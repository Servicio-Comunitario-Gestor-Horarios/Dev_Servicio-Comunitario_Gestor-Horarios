#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTest>
#include <QTime>
#include <QTemporaryDir>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/NucleoDatos.hpp>
#include <backend/services/ServicioCargaHoraria.hpp>
#include <backend/services/ServicioProfesor.hpp>
#include <backend/services/cascada.hpp>

/// Tests de la eliminación en cascada atómica (RF-2).
///
/// Comprueba tres cosas: `dependenciasDe`/`dependientesDe` listan los registros
/// dependientes; el borrado en cascada es atómico (un fallo no deja nada
/// aplicado); y calcular las dependencias (la "cancelación" del diálogo del
/// frontend) no toca los datos.
class TestCascada : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<NucleoDatos> m_nucleo;
    std::unique_ptr<ServicioCargaHoraria> m_carga;
    std::unique_ptr<ServicioProfesor> m_profesor;
    QString m_ruta;
    int m_idTipoAula = -1;

    /// Abre una conexión a la base de prueba y construye el núcleo de datos.
    void abrirBase()
    {
        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_ruta));
        QVERIFY(m_dbManager->isInitialized());

        QSqlDatabase& db = m_dbManager->database();
        m_nucleo.reset(new NucleoDatos(db));
        m_carga.reset(new ServicioCargaHoraria(db));
        m_profesor.reset(new ServicioProfesor(db));
    }

    /// Cierra la conexión para liberar la base de prueba.
    void cerrarBase()
    {
        m_profesor.reset();
        m_carga.reset();
        m_nucleo.reset();
        m_dbManager->close();
        m_dbManager.reset();
    }

    /// Ejecuta una sentencia SQL suelta (para preparar datos o triggers).
    bool ejecutar(const QString& sql)
    {
        QSqlQuery query(m_dbManager->database());
        if (!query.exec(sql)) {
            qWarning() << "SQL de prueba falló:" << query.lastError().text();
            return false;
        }
        return true;
    }

    /// Cuenta las filas de una tabla, opcionalmente con un filtro.
    int contar(const QString& tabla, const QString& where = QString())
    {
        QString sql = QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabla);
        if (!where.isEmpty()) {
            sql += QStringLiteral(" WHERE %1").arg(where);
        }

        QSqlQuery query(m_dbManager->database());
        if (!query.exec(sql) || !query.next()) {
            qWarning() << "No se pudo contar" << tabla << ":" << query.lastError().text();
            return -1;
        }
        return query.value(0).toInt();
    }

    /// Cuenta cuántas dependencias pertenecen a un dominio lógico.
    static int contarDominio(const QVector<Dependencia>& dependencias, const QString& dominio)
    {
        int total = 0;
        for (const Dependencia& d : dependencias) {
            if (d.dominio == dominio) {
                ++total;
            }
        }
        return total;
    }

private slots:

    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());
        m_ruta = m_dir->filePath("cascada.db");
        abrirBase();

        const auto tipo = m_nucleo->crearTipoAula(QStringLiteral("Aula de estudio"));
        QVERIFY2(tipo.ok, qPrintable(tipo.mensajeError));
        m_idTipoAula = tipo.valor.id;
    }

    void cleanup()
    {
        cerrarBase();
        m_dir.reset();
    }

    // ─── Función pura `dependenciasDe` ──────────────────────────────────────

    void dependenciasDe_describeRelacionesPorDominio()
    {
        const auto materia = dependenciasDe(QStringLiteral("materia"));
        QCOMPARE(materia.size(), 2);

        bool apuntaAProfesor = false;
        bool apuntaACurso = false;
        bool apuntaAPlan = false;
        for (const RelacionCascada& rel : materia) {
            if (rel.tabla == QStringLiteral("Profesor_Materia")) apuntaAProfesor = true;
            if (rel.tabla == QStringLiteral("Curso_Materia"))    apuntaACurso = true;
            if (rel.tabla == QStringLiteral("PlanEstudio_Materia")) apuntaAPlan = true;
        }
        QVERIFY(apuntaAProfesor);
        QVERIFY(!apuntaACurso);   // Curso_Materia se retiro (RF-5)
        QVERIFY(apuntaAPlan);

        QCOMPARE(dependenciasDe(QStringLiteral("aula")).size(), 1);
        QCOMPARE(dependenciasDe(QStringLiteral("curso")).size(), 0); // ya no arrastra nada
        QCOMPARE(dependenciasDe(QStringLiteral("turno")).size(), 2);
        QCOMPARE(dependenciasDe(QStringLiteral("plan")).size(), 1);  // sin plan -> cursos
        QCOMPARE(dependenciasDe(QStringLiteral("profesor")).size(), 3);

        QVERIFY(dependenciasDe(QStringLiteral("inexistente")).isEmpty());
        QVERIFY(!dominioValido(QStringLiteral("inexistente")));
        QVERIFY(dominioValido(QStringLiteral("materia")));
    }

    // ─── Consulta `dependientesDe` ──────────────────────────────────────────

    void dependientesDe_plan_listaMateriasYCursos()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Matemática", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);

        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("plan"), QStringLiteral("IS-2026"));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("plan_materia")), 1);
        // El curso ya no depende del plan (RF-5).
        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso")), 0);
    }

    void dependientesDe_materia_listaPlanProfesorYCurso()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Física", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 2, 3).ok);

        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_profesor->asignarMateria("P1", materia.valor.id));

        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        const auto dependencias = m_nucleo->dependientesDe(
            QStringLiteral("materia"), QString::number(materia.valor.id));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("plan_materia")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("profesor_materia")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso_materia")), 0);
    }

    void dependientesDe_aula_listaCursosConAulaFija()
    {
        auto aula = m_nucleo->crearAula("Aula 101", 30, "Edificio A", QString(), m_idTipoAula);
        QVERIFY(aula.ok);
        auto curso = m_nucleo->crearCurso(1, "A", QString(), aula.valor.id);
        QVERIFY(curso.ok);

        const auto dependencias = m_nucleo->dependientesDe(
            QStringLiteral("aula"), QString::number(aula.valor.id));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso")), 1);
    }

    void dependientesDe_curso_noTieneDependientes()
    {
        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        const auto dependencias = m_nucleo->dependientesDe(
            QStringLiteral("curso"), QString::number(curso.valor.id));

        QVERIFY(dependencias.isEmpty());
    }

    void dependientesDe_profesor_listaRelaciones()
    {
        auto materia = m_nucleo->crearMateria("Historia", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_profesor->asignarMateria("P1", materia.valor.id));

        FranjaHoraria franja;
        franja.dia = 1;
        franja.inicio = QTime(8, 0);
        franja.fin = QTime(10, 0);
        QVERIFY(m_profesor->agregarDisponibilidad("P1", franja).ok);

        FranjaHoraria preferida;
        preferida.dia = 2;
        preferida.inicio = QTime(14, 0);
        preferida.fin = QTime(16, 0);
        QVERIFY(m_nucleo->agregarPreferencia("P1", preferida,
                                             SentidoPreferencia::Evitada).ok);

        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("profesor"), QStringLiteral("P1"));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("profesor_materia")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("disponibilidad")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("preferencia")), 1);

        // La preferencia se lista con etiqueta legible (sentido, día y rango).
        bool etiquetaLegible = false;
        for (const Dependencia& d : dependencias) {
            if (d.dominio != QStringLiteral("preferencia"))
                continue;
            etiquetaLegible = d.descripcion.contains(QStringLiteral("Preferencia horaria")) &&
                              d.descripcion.contains(QStringLiteral("evitada")) &&
                              d.descripcion.contains(QStringLiteral("14:00-16:00"));
        }
        QVERIFY(etiquetaLegible);
    }

    void dependientesDe_turno_listaEventos()
    {
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        QVERIFY(m_nucleo->agregarEvento("Mañana", "Receso", 2, 10).ok);

        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("turno"), QStringLiteral("Mañana"));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("evento")), 1);
        // La dependencia lleva la etiqueta del evento (su nombre).
        QVERIFY(dependencias.first().descripcion.contains(QStringLiteral("Receso")));
    }

    void dependientesDe_dominioDesconocido_estaVacio()
    {
        QVERIFY(m_nucleo->dependientesDe(QStringLiteral("inexistente"), QStringLiteral("1"))
                    .isEmpty());
    }

    // ─── Cancelación: calcular dependencias no toca datos ───────────────────

    void calcularDependencias_noModificaDatos()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Matemática", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);
        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        const int planesAntes = contar(QStringLiteral("PlanEstudio"));
        const int planMateriaAntes = contar(QStringLiteral("PlanEstudio_Materia"));
        const int cursosAntes = contar(QStringLiteral("Cursos"));
        const int materiasAntes = contar(QStringLiteral("Materias"));

        // El frontend pide las dependencias para pedir confirmación; si el
        // usuario cancela, no debe llamarse a `eliminarConCascada`.
        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("plan"), QStringLiteral("IS-2026"));
        QVERIFY(!dependencias.isEmpty());

        QCOMPARE(contar(QStringLiteral("PlanEstudio")), planesAntes);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), planMateriaAntes);
        QCOMPARE(contar(QStringLiteral("Cursos")), cursosAntes);
        QCOMPARE(contar(QStringLiteral("Materias")), materiasAntes);
    }

    // ─── Borrado en cascada ─────────────────────────────────────────────────

    void eliminarConCascada_plan_borraVinculosYCursos()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Matemática", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);
        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("plan"), QStringLiteral("IS-2026")));

        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), 0);
        // El curso ya no depende del plan: se conserva (RF-5).
        QCOMPARE(contar(QStringLiteral("Cursos")), 1);
        // La materia en sí no es un dependiente del plan: se conserva.
        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 1);
    }

    void eliminarConCascada_materia_borraSusVinculos()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Física", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 2, 3).ok);

        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_profesor->asignarMateria("P1", materia.valor.id));

        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("materia"),
                                             QString::number(materia.valor.id)));

        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), 0);
        QCOMPARE(contar(QStringLiteral("Profesor_Materia")), 0);
        // El plan, el docente y el curso se conservan.
        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 1);
        QCOMPARE(m_nucleo->listarDocentes().valor.size(), 1);
        QCOMPARE(m_nucleo->listarCursos().valor.size(), 1);
    }

    void eliminarConCascada_curso_borraElCurso()
    {
        auto materia = m_nucleo->crearMateria("Química", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("curso"),
                                             QString::number(curso.valor.id)));

        QCOMPARE(m_nucleo->listarCursos().valor.size(), 0);
        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 1);
    }

    // ─── Anulación de referencia (aula/turno -> cursos) ─────────────────────

    void eliminarConCascada_aula_anulaReferenciaDeCursos()
    {
        auto aula = m_nucleo->crearAula("Aula 101", 30, QString(), QString(), m_idTipoAula);
        QVERIFY(aula.ok);
        QVERIFY(m_nucleo->crearCurso(1, "A", QString(), aula.valor.id).ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("aula"),
                                             QString::number(aula.valor.id)));

        // El aula desaparece; el curso permanece, sin aula.
        QCOMPARE(m_nucleo->listarAulas().valor.size(), 0);
        const auto cursos = m_nucleo->listarCursos().valor;
        QCOMPARE(cursos.size(), 1);
        QCOMPARE(cursos.first().aula, -1);
    }

    void eliminarConCascada_turno_anulaReferenciaDeCursos()
    {
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        QVERIFY(m_nucleo->crearCurso(1, "A", QStringLiteral("Mañana")).ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("turno"), QStringLiteral("Mañana")));

        QCOMPARE(m_nucleo->listarTurnos().valor.size(), 0);
        const auto cursos = m_nucleo->listarCursos().valor;
        QCOMPARE(cursos.size(), 1);
        QVERIFY(cursos.first().turno.isEmpty());
    }

    void eliminarConCascada_profesor_borraDisponibilidadYPreferencias()
    {
        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);

        FranjaHoraria disponibilidad;
        disponibilidad.dia = 1;
        disponibilidad.inicio = QTime(8, 0);
        disponibilidad.fin = QTime(10, 0);
        QVERIFY(m_profesor->agregarDisponibilidad("P1", disponibilidad).ok);

        FranjaHoraria preferida;
        preferida.dia = 2;
        preferida.inicio = QTime(14, 0);
        preferida.fin = QTime(16, 0);
        QVERIFY(m_nucleo->agregarPreferencia("P1", preferida,
                                             SentidoPreferencia::Preferida).ok);

        QCOMPARE(contar(QStringLiteral("Disponibilidad_Profesor")), 1);
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 1);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("profesor"),
                                             QStringLiteral("P1")));

        QCOMPARE(m_nucleo->listarDocentes().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("Disponibilidad_Profesor")), 0);
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    /// Un fallo al borrar el docente revierte también sus hijas (RNF-4).
    void eliminarConCascada_profesor_atomico_falloDejaTodoIntacto()
    {
        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);

        FranjaHoraria franja;
        franja.dia = 1;
        franja.inicio = QTime(8, 0);
        franja.fin = QTime(10, 0);
        QVERIFY(m_profesor->agregarDisponibilidad("P1", franja).ok);
        QVERIFY(m_nucleo->agregarPreferencia("P1", franja,
                                             SentidoPreferencia::Preferida).ok);

        // El trigger aborta al borrar el docente, después de borrar sus hijas.
        QVERIFY(ejecutar(QStringLiteral(
            "CREATE TRIGGER fallo_cascada BEFORE DELETE ON Profesores "
            "BEGIN SELECT RAISE(ABORT, 'fallo simulado en cascada'); END;")));

        QVERIFY(!m_nucleo->eliminarConCascada(QStringLiteral("profesor"),
                                              QStringLiteral("P1")));

        // Nada aplicado: docente, disponibilidad y preferencia siguen ahí.
        QCOMPARE(m_nucleo->listarDocentes().valor.size(), 1);
        QCOMPARE(contar(QStringLiteral("Disponibilidad_Profesor")), 1);
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 1);

        // Retirado el trigger, la cascada vuelve a funcionar.
        QVERIFY(ejecutar(QStringLiteral("DROP TRIGGER fallo_cascada")));
        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("profesor"),
                                             QStringLiteral("P1")));
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    void eliminarConCascada_turno_borraSusEventos()
    {
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        QVERIFY(m_nucleo->agregarEvento("Mañana", "Receso", 2, 10).ok);
        QVERIFY(m_nucleo->agregarEvento("Mañana", "Acto cívico", 2, 5).ok);
        QCOMPARE(contar(QStringLiteral("Eventos")), 2);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("turno"), QStringLiteral("Mañana")));

        QCOMPARE(m_nucleo->listarTurnos().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("Eventos")), 0);
    }

    void eliminarConCascada_registroInexistente_falla()
    {
        QVERIFY(!m_nucleo->eliminarConCascada(QStringLiteral("materia"),
                                              QStringLiteral("9999")));
        QVERIFY(!m_nucleo->eliminarConCascada(QStringLiteral("inexistente"),
                                              QStringLiteral("1")));
    }

    // ─── Atomicidad: un fallo a mitad no deja nada aplicado ─────────────────

    void eliminarConCascada_atomico_falloDejaTodoIntacto()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Matemática", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);

        auto curso = m_nucleo->crearCurso(1, "A");
        QVERIFY(curso.ok);

        // Falla simulada: al borrar el plan (después de borrar sus dependientes)
        // el trigger aborta la transacción entera.
        QVERIFY(ejecutar(QStringLiteral(
            "CREATE TRIGGER fallo_cascada BEFORE DELETE ON PlanEstudio "
            "BEGIN SELECT RAISE(ABORT, 'fallo simulado en cascada'); END;")));

        QVERIFY(!m_nucleo->eliminarConCascada(QStringLiteral("plan"),
                                              QStringLiteral("IS-2026")));

        // Nada aplicado: el plan, el vínculo y el curso siguen ahí.
        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 1);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), 1);
        QCOMPARE(m_nucleo->listarCursos().valor.size(), 1);

        // Retirado el trigger, la cascada vuelve a funcionar.
        QVERIFY(ejecutar(QStringLiteral("DROP TRIGGER fallo_cascada")));
        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("plan"),
                                             QStringLiteral("IS-2026")));
        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), 0);
    }
};

QTEST_MAIN(TestCascada)
#include "test_cascada.moc"
