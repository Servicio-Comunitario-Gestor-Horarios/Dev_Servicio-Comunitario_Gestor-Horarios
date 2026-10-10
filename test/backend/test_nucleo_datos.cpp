#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTest>
#include <QTime>
#include <QTemporaryDir>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/database/version_esquema.hpp>
#include <backend/services/NucleoDatos.hpp>

/// Tests del núcleo de datos: CRUD de dominios y persistencia tras reapertura (RF-2).
class TestNucleoDatos : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<NucleoDatos> m_nucleo;
    QString m_ruta;

    /// Abre una conexión a la base de prueba y construye el núcleo de datos.
    void abrirBase()
    {
        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_ruta));
        QVERIFY(m_dbManager->isInitialized());
        m_nucleo.reset(new NucleoDatos(m_dbManager->database()));
    }

    /// Cierra la conexión para simular el cierre de la aplicación.
    void cerrarBase()
    {
        m_nucleo.reset();
        m_dbManager->close();
        m_dbManager.reset();
    }

    /// Indica si una tabla existe en la base abierta.
    bool tablaExiste(const QString& tabla)
    {
        QSqlQuery query(m_dbManager->database());
        query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:nombre");
        query.bindValue(":nombre", tabla);
        return query.exec() && query.next();
    }

private slots:

    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());
        m_ruta = m_dir->filePath("nucleo.db");
        abrirBase();
    }

    void cleanup()
    {
        cerrarBase();
        m_dir.reset();
    }

    // ─── Esquema v2 ─────────────────────────────────────────────────────────

    void esquemaV2_contieneTablasDeDominio()
    {
        QCOMPARE(VersionEsquema::leerVersionEsquema(m_dbManager->database()),
                 VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        QVERIFY(tablaExiste(QStringLiteral("Cursos")));
        QVERIFY(tablaExiste(QStringLiteral("Curso_Materia")));
        QVERIFY(tablaExiste(QStringLiteral("Turnos")));
        QVERIFY(tablaExiste(QStringLiteral("Recesos")));
    }

    // ─── Docentes ───────────────────────────────────────────────────────────

    void docentes_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com", "555");
        QVERIFY(creado.ok);
        QCOMPARE(m_nucleo->listarDocentes().size(), 1);

        auto actualizado =
            m_nucleo->actualizarDocente("P1", "Ana Gómez", "ana@test.com", "777");
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarDocentes().first().nombre, QStringLiteral("Ana Gómez"));

        QVERIFY(m_nucleo->eliminarDocente("P1"));
        QVERIFY(m_nucleo->listarDocentes().isEmpty());
    }

    // ─── Aulas ──────────────────────────────────────────────────────────────

    void aulas_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearAula("Aula 101", 30, "Edificio A", "Piso 1");
        QVERIFY(creado.ok);
        QCOMPARE(m_nucleo->listarAulas().size(), 1);

        auto actualizado = m_nucleo->actualizarAula(creado.valor.id, "Aula 101", 45,
                                                    "Edificio B", "Piso 2");
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarAulas().first().capacidad, 45);

        QVERIFY(m_nucleo->eliminarAula(creado.valor.id));
        QVERIFY(m_nucleo->listarAulas().isEmpty());
    }

    // ─── Materias ───────────────────────────────────────────────────────────

    void materias_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearMateria("Matemática");
        QVERIFY(creado.ok);
        QCOMPARE(m_nucleo->listarMaterias().size(), 1);

        auto actualizado = m_nucleo->actualizarMateria(creado.valor.id, "Matemática II");
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarMaterias().first().nombre, QStringLiteral("Matemática II"));

        QVERIFY(m_nucleo->eliminarMateria(creado.valor.id));
        QVERIFY(m_nucleo->listarMaterias().isEmpty());
    }

    // ─── Planes de estudio ──────────────────────────────────────────────────

    void planes_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearPlan("IS-2026", "Ingeniería de Sistemas");
        QVERIFY(creado.ok);
        QCOMPARE(m_nucleo->listarPlanes().size(), 1);

        auto actualizado =
            m_nucleo->actualizarPlan("IS-2026", "Ingeniería de Sistemas v2", "Descripción");
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarPlanes().first().nombre,
                 QStringLiteral("Ingeniería de Sistemas v2"));

        QVERIFY(m_nucleo->eliminarPlan("IS-2026"));
        QVERIFY(m_nucleo->listarPlanes().isEmpty());
    }

    // ─── Cursos ─────────────────────────────────────────────────────────────

    void cursos_altaModificacionBaja()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería de Sistemas").ok);
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        auto materia = m_nucleo->crearMateria("Programación");
        QVERIFY(materia.ok);

        auto creado = m_nucleo->crearCurso("1º A", "Mañana", -1, 30, "IS-2026");
        QVERIFY(creado.ok);
        QCOMPARE(creado.valor.nombre, QStringLiteral("1º A"));
        QCOMPARE(creado.valor.turno, QStringLiteral("Mañana"));
        QCOMPARE(creado.valor.numEstudiantes, 30);
        QCOMPARE(creado.valor.codigoPlan, QStringLiteral("IS-2026"));

        auto asignada = m_nucleo->asignarMateriaACurso(creado.valor.id, materia.valor.id, 4);
        QVERIFY(asignada.ok);
        QCOMPARE(asignada.valor.horasSemanales, 4);

        auto lista = m_nucleo->listarCursos();
        QCOMPARE(lista.size(), 1);
        QCOMPARE(lista.first().materias.size(), 1);
        QCOMPARE(lista.first().materias.first().idMateria, materia.valor.id);

        auto actualizado =
            m_nucleo->actualizarCurso(creado.valor.id, "1º B", "Mañana", -1, 35, "IS-2026");
        QVERIFY(actualizado.ok);
        QCOMPARE(actualizado.valor.nombre, QStringLiteral("1º B"));
        QCOMPARE(actualizado.valor.numEstudiantes, 35);

        QVERIFY(m_nucleo->eliminarCurso(creado.valor.id));
        QVERIFY(m_nucleo->listarCursos().isEmpty());
    }

    // ─── Turnos y recesos ───────────────────────────────────────────────────

    void turnosRecesos_altaModificacionBaja()
    {
        auto turno = m_nucleo->crearTurno("Tarde", QTime(13, 0), QTime(18, 0), 5);
        QVERIFY(turno.ok);
        QCOMPARE(turno.valor.nombre, QStringLiteral("Tarde"));
        QCOMPARE(turno.valor.numSlots, 5);

        auto receso = m_nucleo->agregarReceso("Tarde", 2, 15, QTime(15, 0), QTime(15, 15));
        QVERIFY(receso.ok);
        QCOMPARE(receso.valor.despuesDeSlot, 2);
        QCOMPARE(receso.valor.duracion, 15);

        auto lista = m_nucleo->listarTurnos();
        QCOMPARE(lista.size(), 1);
        QCOMPARE(lista.first().recesos.size(), 1);
        QCOMPARE(lista.first().recesos.first().duracion, 15);

        auto actualizado = m_nucleo->actualizarTurno("Tarde", QTime(14, 0), QTime(19, 0), 6);
        QVERIFY(actualizado.ok);
        QCOMPARE(actualizado.valor.numSlots, 6);

        QVERIFY(m_nucleo->eliminarReceso("Tarde", 2));
        QCOMPARE(m_nucleo->listarTurnos().first().recesos.size(), 0);

        QVERIFY(m_nucleo->eliminarTurno("Tarde"));
        QVERIFY(m_nucleo->listarTurnos().isEmpty());
    }

    // ─── Persistencia tras reapertura ───────────────────────────────────────

    void persistencia_todosLosDominios_trasReapertura()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería de Sistemas").ok);
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        QVERIFY(m_nucleo->agregarReceso("Mañana", 2, 10).ok);
        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_nucleo->crearAula("Aula 1", 30, "Edificio A").ok);
        auto materia = m_nucleo->crearMateria("Matemática");
        QVERIFY(materia.ok);
        auto curso = m_nucleo->crearCurso("1º A", "Mañana", -1, 25, "IS-2026");
        QVERIFY(curso.ok);
        QVERIFY(m_nucleo->asignarMateriaACurso(curso.valor.id, materia.valor.id, 3).ok);

        // Cerrar y reabrir la base: todo debe seguir ahí.
        cerrarBase();
        abrirBase();

        QCOMPARE(m_nucleo->listarDocentes().size(), 1);
        QCOMPARE(m_nucleo->listarAulas().size(), 1);
        QCOMPARE(m_nucleo->listarMaterias().size(), 1);
        QCOMPARE(m_nucleo->listarPlanes().size(), 1);
        QCOMPARE(m_nucleo->listarCursos().size(), 1);
        QCOMPARE(m_nucleo->listarTurnos().size(), 1);
        QCOMPARE(m_nucleo->listarTurnos().first().recesos.size(), 1);

        auto cursos = m_nucleo->listarCursos();
        QCOMPARE(cursos.first().nombre, QStringLiteral("1º A"));
        QCOMPARE(cursos.first().codigoPlan, QStringLiteral("IS-2026"));
        QCOMPARE(cursos.first().materias.size(), 1);
        QCOMPARE(cursos.first().materias.first().horasSemanales, 3);

        // Una baja también persiste tras reapertura.
        QVERIFY(m_nucleo->eliminarDocente("P1"));
        cerrarBase();
        abrirBase();
        QVERIFY(m_nucleo->listarDocentes().isEmpty());
    }
};

QTEST_MAIN(TestNucleoDatos)
#include "test_nucleo_datos.moc"
