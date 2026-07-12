#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioCargaHoraria.hpp>
#include <backend/services/ServicioPlanesEstudio.hpp>
#include <backend/services/ServicioMaterias.hpp>

class TestServicioCargaHoraria : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QString dbPath = m_tempDir->filePath("test_carga.db");
        QVERIFY(m_dbManager->initialize(dbPath));
        QVERIFY(m_dbManager->isInitialized());

        m_servicioCarga.reset(new ServicioCargaHoraria(m_dbManager->database()));

        // Crear datos base para pruebas
        m_servicioPlanes.reset(new ServicioPlanesEstudio(m_dbManager->database()));
        m_servicioPlanes->crearPlan("PLAN-TEST", "Plan de Prueba");

        m_servicioMaterias.reset(new ServicioMaterias(m_dbManager->database()));
        m_servicioMaterias->crearMateria("Matemática");
        m_servicioMaterias->crearMateria("Física");
        m_servicioMaterias->crearMateria("Química");
    }

    void cleanup() {
        m_servicioCarga.reset();
        m_servicioMaterias.reset();
        m_servicioPlanes.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Tests ──────────────────────────────────────────────────────────────

    void asignarCarga_exitoso() {
        auto resultado = m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 4);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.codigoPlanEstudio, "PLAN-TEST");
        QCOMPARE(resultado.valor.idMateria, 1);
        QCOMPARE(resultado.valor.curso, 1);
        QCOMPARE(resultado.valor.horas, 4);
    }

    void asignarCarga_planInexistente_falla() {
        auto resultado = m_servicioCarga->asignarCarga("PLAN-999", 1, 1, 4);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no existe"));
    }

    void asignarCarga_materiaInexistente_falla() {
        auto resultado = m_servicioCarga->asignarCarga("PLAN-TEST", 999, 1, 4);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no existe"));
    }

    void asignarCarga_cursoInvalido_falla() {
        auto resultado = m_servicioCarga->asignarCarga("PLAN-TEST", 1, 0, 4);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("curso debe estar entre"));

        auto resultado2 = m_servicioCarga->asignarCarga("PLAN-TEST", 1, 7, 4);
        QVERIFY(!resultado2.ok);
        QVERIFY(resultado2.mensajeError.contains("curso debe estar entre"));
    }

    void asignarCarga_horasCero_falla() {
        auto resultado = m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 0);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("mayores a 0"));
    }

    void asignarCarga_duplicado_falla() {
        auto r1 = m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 4);
        QVERIFY(r1.ok);

        auto r2 = m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 3);
        QVERIFY(!r2.ok);
        QVERIFY(r2.mensajeError.contains("ya está asignada"));
    }

    void asignarCarga_excedeLimite_falla() {
        // Asignar varias materias para exceder el límite de 40 horas
        auto r1 = m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 20);
        QVERIFY(r1.ok);
        auto r2 = m_servicioCarga->asignarCarga("PLAN-TEST", 2, 1, 20);
        QVERIFY(r2.ok);

        // Intentar agregar una tercera con 5 horas (total 45 > 40)
        auto r3 = m_servicioCarga->asignarCarga("PLAN-TEST", 3, 1, 5);
        QVERIFY(!r3.ok);
        QVERIFY(r3.mensajeError.contains("excederían el límite"));
    }

    void obtenerCarga_exitoso() {
        m_servicioCarga->asignarCarga("PLAN-TEST", 1, 2, 5);

        auto obtenida = m_servicioCarga->obtenerCarga(1, "PLAN-TEST");
        QVERIFY(obtenida.ok);
        QCOMPARE(obtenida.valor.idMateria, 1);
        QCOMPARE(obtenida.valor.curso, 2);
        QCOMPARE(obtenida.valor.horas, 5);
        QCOMPARE(obtenida.valor.nombreMateria, "Matemática");
    }

    void obtenerCarga_inexistente_falla() {
        auto resultado = m_servicioCarga->obtenerCarga(999, "PLAN-TEST");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void listarCargaPorPlan() {
        m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 4);
        m_servicioCarga->asignarCarga("PLAN-TEST", 2, 1, 3);
        m_servicioCarga->asignarCarga("PLAN-TEST", 3, 2, 5);

        auto lista = m_servicioCarga->listarCargaPorPlan("PLAN-TEST");
        QCOMPARE(lista.size(), 3);

        // Ordenado por curso y nombre
        QCOMPARE(lista[0].nombreMateria, "Física");
        QCOMPARE(lista[1].nombreMateria, "Matemática");
        QCOMPARE(lista[2].nombreMateria, "Química");
    }

    void listarCargaPorPlan_planSinMaterias() {
        auto lista = m_servicioCarga->listarCargaPorPlan("PLAN-TEST");
        QVERIFY(lista.isEmpty());
    }

    void actualizarCarga_exitoso() {
        m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 4);

        auto actualizada = m_servicioCarga->actualizarCarga(1, "PLAN-TEST", 6);
        QVERIFY(actualizada.ok);
        QCOMPARE(actualizada.valor.horas, 6);
    }

    void actualizarCarga_excedeLimite_falla() {
        m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 20);
        m_servicioCarga->asignarCarga("PLAN-TEST", 2, 1, 20);

        // Intentar actualizar a 5 horas (total 45 > 40)
        auto resultado = m_servicioCarga->actualizarCarga(1, "PLAN-TEST", 25);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("excederían el límite"));
    }

    void actualizarCarga_inexistente_falla() {
        auto resultado = m_servicioCarga->actualizarCarga(999, "PLAN-TEST", 5);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void eliminarCarga_exitoso() {
        m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 4);

        bool eliminado = m_servicioCarga->eliminarCarga(1, "PLAN-TEST");
        QVERIFY(eliminado);

        auto obtenida = m_servicioCarga->obtenerCarga(1, "PLAN-TEST");
        QVERIFY(!obtenida.ok);
    }

    void totalHorasPorAnio() {
        m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 4);
        m_servicioCarga->asignarCarga("PLAN-TEST", 2, 1, 3);
        m_servicioCarga->asignarCarga("PLAN-TEST", 3, 2, 5);

        int totalCurso1 = m_servicioCarga->totalHorasPorAnio("PLAN-TEST", 1);
        QCOMPARE(totalCurso1, 7);

        int totalCurso2 = m_servicioCarga->totalHorasPorAnio("PLAN-TEST", 2);
        QCOMPARE(totalCurso2, 5);

        int totalCurso3 = m_servicioCarga->totalHorasPorAnio("PLAN-TEST", 3);
        QCOMPARE(totalCurso3, 0);
    }

    void excedeLimiteHoras() {
        m_servicioCarga->asignarCarga("PLAN-TEST", 1, 1, 35);

        bool excede1 = m_servicioCarga->excedeLimiteHoras("PLAN-TEST", 1, 0);
        QVERIFY(!excede1);

        bool excede2 = m_servicioCarga->excedeLimiteHoras("PLAN-TEST", 1, 5);
        QVERIFY(!excede2);

        bool excede3 = m_servicioCarga->excedeLimiteHoras("PLAN-TEST", 1, 10);
        QVERIFY(excede3);
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioPlanesEstudio> m_servicioPlanes;
    std::unique_ptr<ServicioMaterias> m_servicioMaterias;
    std::unique_ptr<ServicioCargaHoraria> m_servicioCarga;
};

QTEST_MAIN(TestServicioCargaHoraria)
#include "test_servicio_carga_horaria.moc"