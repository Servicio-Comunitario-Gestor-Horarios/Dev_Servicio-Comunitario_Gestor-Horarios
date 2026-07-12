#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioComposicionPlan.hpp>
#include <backend/services/ServicioPlanesEstudio.hpp>
#include <backend/services/ServicioMaterias.hpp>
#include <backend/services/ServicioCargaHoraria.hpp>

class TestServicioComposicionPlan : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QString dbPath = m_tempDir->filePath("test_composicion.db");
        QVERIFY(m_dbManager->initialize(dbPath));
        QVERIFY(m_dbManager->isInitialized());

        m_servicioComposicion.reset(new ServicioComposicionPlan(m_dbManager->database()));
        m_servicioPlanes.reset(new ServicioPlanesEstudio(m_dbManager->database()));
        m_servicioMaterias.reset(new ServicioMaterias(m_dbManager->database()));
        m_servicioCarga.reset(new ServicioCargaHoraria(m_dbManager->database()));

        // Crear datos de prueba completos
        m_servicioPlanes->crearPlan("PLAN-COMP", "Plan de Composición", "Plan para pruebas");

        auto mat1 = m_servicioMaterias->crearMateria("Matemática", "Álgebra, Geometría");
        auto mat2 = m_servicioMaterias->crearMateria("Física", "Matemática");
        auto mat3 = m_servicioMaterias->crearMateria("Química", "Física");

        m_mateId = mat1.valor.id;
        m_fisicaId = mat2.valor.id;
        m_quimicaId = mat3.valor.id;

        m_servicioCarga->asignarCarga("PLAN-COMP", m_mateId, 1, 4);
        m_servicioCarga->asignarCarga("PLAN-COMP", m_fisicaId, 2, 3);
        m_servicioCarga->asignarCarga("PLAN-COMP", m_quimicaId, 3, 5);
    }

    void cleanup() {
        m_servicioCarga.reset();
        m_servicioMaterias.reset();
        m_servicioPlanes.reset();
        m_servicioComposicion.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Tests ──────────────────────────────────────────────────────────────

    void componerPlanCompleto_exitoso() {
        auto resultado = m_servicioComposicion->componerPlanCompleto("PLAN-COMP");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Plan de Composición");
        QCOMPARE(resultado.valor.materias.size(), 3);

        // Verificar MateriaEnPlan 1
        const auto& mate1 = resultado.valor.materias[0];
        QCOMPARE(mate1.curso, 1);
        QCOMPARE(mate1.horas, 4);
        QCOMPARE(mate1.materia.nombre, "Matemática");
        QCOMPARE(mate1.materia.horas_semanales, 4);
        QCOMPARE(mate1.materia.requerimientos.size(), 2);
        QCOMPARE(mate1.materia.requerimientos[0], "Álgebra");
        QCOMPARE(mate1.materia.requerimientos[1], "Geometría");

        // Verificar MateriaEnPlan 2
        const auto& mate2 = resultado.valor.materias[1];
        QCOMPARE(mate2.curso, 2);
        QCOMPARE(mate2.horas, 3);
        QCOMPARE(mate2.materia.nombre, "Física");
        QCOMPARE(mate2.materia.requerimientos.size(), 1);
        QCOMPARE(mate2.materia.requerimientos[0], "Matemática");

        // Verificar MateriaEnPlan 3
        const auto& mate3 = resultado.valor.materias[2];
        QCOMPARE(mate3.curso, 3);
        QCOMPARE(mate3.horas, 5);
        QCOMPARE(mate3.materia.nombre, "Química");
        QCOMPARE(mate3.materia.requerimientos.size(), 1);
        QCOMPARE(mate3.materia.requerimientos[0], "Física");
    }

    void componerPlanCompleto_codigoVacio_falla() {
        auto resultado = m_servicioComposicion->componerPlanCompleto("");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("Código de plan inválido"));
    }

    void componerPlanCompleto_planInexistente_falla() {
        auto resultado = m_servicioComposicion->componerPlanCompleto("PLAN-999");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void obtenerMateriasConCarga_exitoso() {
        auto materias = m_servicioComposicion->obtenerMateriasConCarga("PLAN-COMP");

        QCOMPARE(materias.size(), 3);

        QCOMPARE(materias[0].curso, 1);
        QCOMPARE(materias[0].materia.nombre, "Matemática");
        QCOMPARE(materias[0].horas, 4);

        QCOMPARE(materias[1].curso, 2);
        QCOMPARE(materias[1].materia.nombre, "Física");
        QCOMPARE(materias[1].horas, 3);

        QCOMPARE(materias[2].curso, 3);
        QCOMPARE(materias[2].materia.nombre, "Química");
        QCOMPARE(materias[2].horas, 5);
    }

    void obtenerMateriasConCarga_planVacio() {
        auto materias = m_servicioComposicion->obtenerMateriasConCarga("");
        QVERIFY(materias.isEmpty());
    }

    void obtenerMateriasConCarga_planInexistente() {
        auto materias = m_servicioComposicion->obtenerMateriasConCarga("PLAN-999");
        QVERIFY(materias.isEmpty());
    }

    void listarPlanesDisponibles() {
        // Crear un plan adicional
        m_servicioPlanes->crearPlan("PLAN-EXTRA", "Plan Extra");

        auto planes = m_servicioComposicion->listarPlanesDisponibles();

        // Debe tener al menos "PLAN-COMP" y "PLAN-EXTRA"
        QVERIFY(planes.size() >= 2);

        bool foundComp = false;
        bool foundExtra = false;
        for (const auto& plan : planes) {
            if (plan.first == "PLAN-COMP") {
                foundComp = true;
                QCOMPARE(plan.second, "Plan de Composición");
            }
            if (plan.first == "PLAN-EXTRA") {
                foundExtra = true;
                QCOMPARE(plan.second, "Plan Extra");
            }
        }

        QVERIFY(foundComp);
        QVERIFY(foundExtra);
    }

    void listarPlanesDisponibles_vacio() {
        // No debería estar vacío porque tenemos PLAN-COMP
        auto planes = m_servicioComposicion->listarPlanesDisponibles();
        QVERIFY(!planes.isEmpty());
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioComposicionPlan> m_servicioComposicion;
    std::unique_ptr<ServicioPlanesEstudio> m_servicioPlanes;
    std::unique_ptr<ServicioMaterias> m_servicioMaterias;
    std::unique_ptr<ServicioCargaHoraria> m_servicioCarga;

    int m_mateId = -1;
    int m_fisicaId = -1;
    int m_quimicaId = -1;
};

QTEST_MAIN(TestServicioComposicionPlan)
#include "test_composicion_plan.moc"