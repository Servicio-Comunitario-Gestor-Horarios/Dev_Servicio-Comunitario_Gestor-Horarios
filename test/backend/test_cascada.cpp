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
        QCOMPARE(materia.size(), 3);

        bool apuntaAProfesor = false;
        bool apuntaACurso = false;
        bool apuntaAPlan = false;
        for (const RelacionCascada& rel : materia) {
            if (rel.tabla == QStringLiteral("Profesor_Materia")) apuntaAProfesor = true;
            if (rel.tabla == QStringLiteral("Curso_Materia"))    apuntaACurso = true;
            if (rel.tabla == QStringLiteral("PlanEstudio_Materia")) apuntaAPlan = true;
        }
        QVERIFY(apuntaAProfesor);
        QVERIFY(apuntaACurso);
        QVERIFY(apuntaAPlan);

        QCOMPARE(dependenciasDe(QStringLiteral("aula")).size(), 1);
        QCOMPARE(dependenciasDe(QStringLiteral("curso")).size(), 1);
        QCOMPARE(dependenciasDe(QStringLiteral("turno")).size(), 2);
        QCOMPARE(dependenciasDe(QStringLiteral("plan")).size(), 2);
        QCOMPARE(dependenciasDe(QStringLiteral("profesor")).size(), 2);

        QVERIFY(dependenciasDe(QStringLiteral("inexistente")).isEmpty());
        QVERIFY(!dominioValido(QStringLiteral("inexistente")));
        QVERIFY(dominioValido(QStringLiteral("materia")));
    }

    // ─── Consulta `dependientesDe` ──────────────────────────────────────────

    void dependientesDe_plan_listaMateriasYCursos()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Matemática");
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);

        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20, "IS-2026");
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 3).ok);

        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("plan"), QStringLiteral("IS-2026"));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("plan_materia")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso")), 1);
        // La cascada es recursiva: el curso dependiente arrastra su materia.
        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso_materia")), 1);
    }

    void dependientesDe_materia_listaPlanProfesorYCurso()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Física");
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 2, 3).ok);

        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_profesor->asignarMateria("P1", materia.valor.id));

        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20, "IS-2026");
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 3).ok);

        const auto dependencias = m_nucleo->dependientesDe(
            QStringLiteral("materia"), QString::number(materia.valor.id));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("plan_materia")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("profesor_materia")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso_materia")), 1);
    }

    void dependientesDe_aula_listaCursosConAulaFija()
    {
        auto aula = m_nucleo->crearAula("Aula 101", 30, "Edificio A");
        QVERIFY(aula.ok);
        auto curso = m_nucleo->crearCurso("1º A", QString(), aula.valor.id, 20);
        QVERIFY(curso.ok);

        const auto dependencias = m_nucleo->dependientesDe(
            QStringLiteral("aula"), QString::number(aula.valor.id));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso")), 1);
    }

    void dependientesDe_curso_listaCursoMateria()
    {
        auto materia = m_nucleo->crearMateria("Química");
        QVERIFY(materia.ok);
        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20);
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 2).ok);

        const auto dependencias = m_nucleo->dependientesDe(
            QStringLiteral("curso"), QString::number(curso.valor.id));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("curso_materia")), 1);
    }

    void dependientesDe_profesor_listaRelaciones()
    {
        auto materia = m_nucleo->crearMateria("Historia");
        QVERIFY(materia.ok);
        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_profesor->asignarMateria("P1", materia.valor.id));

        FranjaHoraria franja;
        franja.dia = 1;
        franja.inicio = QTime(8, 0);
        franja.fin = QTime(10, 0);
        QVERIFY(m_profesor->agregarDisponibilidad("P1", franja).ok);

        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("profesor"), QStringLiteral("P1"));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("profesor_materia")), 1);
        QCOMPARE(contarDominio(dependencias, QStringLiteral("disponibilidad")), 1);
    }

    void dependientesDe_turno_listaRecesos()
    {
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        QVERIFY(m_nucleo->agregarReceso("Mañana", 2, 10).ok);

        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("turno"), QStringLiteral("Mañana"));

        QCOMPARE(contarDominio(dependencias, QStringLiteral("receso")), 1);
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
        auto materia = m_nucleo->crearMateria("Matemática");
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);
        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20, "IS-2026");
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 3).ok);

        const int planesAntes = contar(QStringLiteral("PlanEstudio"));
        const int planMateriaAntes = contar(QStringLiteral("PlanEstudio_Materia"));
        const int cursosAntes = contar(QStringLiteral("Cursos"));
        const int cursoMateriaAntes = contar(QStringLiteral("Curso_Materia"));
        const int materiasAntes = contar(QStringLiteral("Materias"));

        // El frontend pide las dependencias para pedir confirmación; si el
        // usuario cancela, no debe llamarse a `eliminarConCascada`.
        const auto dependencias =
            m_nucleo->dependientesDe(QStringLiteral("plan"), QStringLiteral("IS-2026"));
        QVERIFY(!dependencias.isEmpty());

        QCOMPARE(contar(QStringLiteral("PlanEstudio")), planesAntes);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), planMateriaAntes);
        QCOMPARE(contar(QStringLiteral("Cursos")), cursosAntes);
        QCOMPARE(contar(QStringLiteral("Curso_Materia")), cursoMateriaAntes);
        QCOMPARE(contar(QStringLiteral("Materias")), materiasAntes);
    }

    // ─── Borrado en cascada ─────────────────────────────────────────────────

    void eliminarConCascada_plan_borraVinculosYCursos()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Matemática");
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);
        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20, "IS-2026");
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 3).ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("plan"), QStringLiteral("IS-2026")));

        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), 0);
        QCOMPARE(contar(QStringLiteral("Cursos")), 0);
        QCOMPARE(contar(QStringLiteral("Curso_Materia")), 0);
        // La materia en sí no es un dependiente del plan: se conserva.
        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 1);
    }

    void eliminarConCascada_materia_borraSusVinculos()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería").ok);
        auto materia = m_nucleo->crearMateria("Física");
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 2, 3).ok);

        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_profesor->asignarMateria("P1", materia.valor.id));

        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20, "IS-2026");
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 3).ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("materia"),
                                             QString::number(materia.valor.id)));

        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("PlanEstudio_Materia")), 0);
        QCOMPARE(contar(QStringLiteral("Profesor_Materia")), 0);
        QCOMPARE(contar(QStringLiteral("Curso_Materia")), 0);
        // El plan, el docente y el curso se conservan.
        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 1);
        QCOMPARE(m_nucleo->listarDocentes().valor.size(), 1);
        QCOMPARE(m_nucleo->listarCursos().valor.size(), 1);
    }

    void eliminarConCascada_curso_borraCursoMateria()
    {
        auto materia = m_nucleo->crearMateria("Química");
        QVERIFY(materia.ok);
        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20);
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 2).ok);

        QVERIFY(m_nucleo->eliminarConCascada(QStringLiteral("curso"),
                                             QString::number(curso.valor.id)));

        QCOMPARE(m_nucleo->listarCursos().valor.size(), 0);
        QCOMPARE(contar(QStringLiteral("Curso_Materia")), 0);
        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 1);
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
        auto materia = m_nucleo->crearMateria("Matemática");
        QVERIFY(materia.ok);
        QVERIFY(m_carga->asignarCarga("IS-2026", materia.valor.id, 1, 4).ok);

        auto curso = m_nucleo->crearCurso("1º A", QString(), -1, 20, "IS-2026");
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 3).ok);

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
        QCOMPARE(contar(QStringLiteral("Curso_Materia")), 1);

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
