#include <QTest>
#include <QTemporaryDir>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioPlanesEstudio.hpp>

class TestServicioPlanesEstudio : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QString dbPath = m_tempDir->filePath("test_planes.db");
        QVERIFY(m_dbManager->initialize(dbPath));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioPlanesEstudio(m_dbManager->database()));
    }

    void cleanup() {
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Tests ──────────────────────────────────────────────────────────────

    void crearPlan_exitoso() {
        auto resultado = m_servicio->crearPlan("PLAN-001", "Plan de Ciencias", "Ciencias Naturales");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.codigo, "PLAN-001");
        QCOMPARE(resultado.valor.nombre, "Plan de Ciencias");
        QCOMPARE(resultado.valor.descripcion, "Ciencias Naturales");
    }

    void crearPlan_sinDescripcion() {
        auto resultado = m_servicio->crearPlan("PLAN-002", "Plan de Letras");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.codigo, "PLAN-002");
        QCOMPARE(resultado.valor.nombre, "Plan de Letras");
        QVERIFY(resultado.valor.descripcion.isEmpty());
    }

    void crearPlan_codigoVacio_falla() {
        auto resultado = m_servicio->crearPlan("", "Plan");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede estar vacío"));
    }

    void crearPlan_nombreVacio_falla() {
        auto resultado = m_servicio->crearPlan("PLAN-003", "");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede estar vacío"));
    }

    void crearPlan_codigoDuplicado_falla() {
        auto r1 = m_servicio->crearPlan("PLAN-004", "Plan A");
        QVERIFY(r1.ok);

        auto r2 = m_servicio->crearPlan("PLAN-004", "Plan B");
        QVERIFY(!r2.ok);
        QVERIFY(r2.mensajeError.contains("Ya existe un plan"));
    }

    void obtenerPlan_exitoso() {
        auto creado = m_servicio->crearPlan("PLAN-005", "Plan Test", "Descripción Test");
        QVERIFY(creado.ok);

        auto obtenido = m_servicio->obtenerPlan("PLAN-005");
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.codigo, "PLAN-005");
        QCOMPARE(obtenido.valor.nombre, "Plan Test");
        QCOMPARE(obtenido.valor.descripcion, "Descripción Test");
    }

    void obtenerPlan_inexistente_falla() {
        auto resultado = m_servicio->obtenerPlan("PLAN-999");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void listarPlanes() {
        m_servicio->crearPlan("Z-PLAN", "Plan Z");
        m_servicio->crearPlan("A-PLAN", "Plan A");
        m_servicio->crearPlan("M-PLAN", "Plan M");

        auto lista = m_servicio->listarPlanes();
        QCOMPARE(lista.size(), 3);

        // Ordenado por código
        QCOMPARE(lista[0].codigo, "A-PLAN");
        QCOMPARE(lista[1].codigo, "M-PLAN");
        QCOMPARE(lista[2].codigo, "Z-PLAN");
    }

    void actualizarPlan_exitoso() {
        auto creado = m_servicio->crearPlan("PLAN-006", "Plan Original");
        QVERIFY(creado.ok);

        auto actualizado = m_servicio->actualizarPlan(
            "PLAN-006", "Plan Actualizado", "Nueva descripción"
        );

        QVERIFY(actualizado.ok);
        QCOMPARE(actualizado.valor.nombre, "Plan Actualizado");
        QCOMPARE(actualizado.valor.descripcion, "Nueva descripción");
    }

    void actualizarPlan_inexistente_falla() {
        auto resultado = m_servicio->actualizarPlan("PLAN-999", "Plan");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void eliminarPlan_exitoso() {
        auto creado = m_servicio->crearPlan("PLAN-007", "Plan a Eliminar");
        QVERIFY(creado.ok);

        bool eliminado = m_servicio->eliminarPlan("PLAN-007");
        QVERIFY(eliminado);

        auto obtenido = m_servicio->obtenerPlan("PLAN-007");
        QVERIFY(!obtenido.ok);
    }

    void eliminarPlan_conMaterias_falla() {
        auto plan = m_servicio->crearPlan("PLAN-008", "Plan con Materias");
        QVERIFY(plan.ok);

        // Crear una materia y asociarla al plan
        QSqlQuery query(m_dbManager->database());
        query.prepare("INSERT INTO Materias (nombre) VALUES ('Materia Test')");
        QVERIFY(query.exec());
        int materiaId = query.lastInsertId().toInt();

        query.prepare(
            "INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas) "
            "VALUES ('PLAN-008', :id, 1, 4)"
        );
        query.bindValue(":id", materiaId);
        QVERIFY(query.exec());

        bool eliminado = m_servicio->eliminarPlan("PLAN-008");
        QVERIFY(!eliminado); // No debe eliminar porque tiene materias asociadas
    }

    void eliminarPlan_inexistente_falla() {
        bool eliminado = m_servicio->eliminarPlan("PLAN-999");
        QVERIFY(!eliminado);
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioPlanesEstudio> m_servicio;
};

QTEST_MAIN(TestServicioPlanesEstudio)
#include "test_servicio_planes_estudio.moc"