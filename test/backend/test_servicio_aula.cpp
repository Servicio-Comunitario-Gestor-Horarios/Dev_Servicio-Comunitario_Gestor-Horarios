#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioAula.hpp>

class TestServicioAula : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QString dbPath = m_tempDir->filePath("test_aulas.db");
        QVERIFY(m_dbManager->initialize(dbPath));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioAula(m_dbManager->database()));
    }

    void cleanup() {
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Tests ──────────────────────────────────────────────────────────────

    void crearAula_exitoso() {
        auto resultado = m_servicio->crearAula("Aula 101", 30, "Edificio A", "Piso 1");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Aula 101");
        QCOMPARE(resultado.valor.capacidad, 30);
        QCOMPARE(resultado.valor.edificio, "Edificio A");
        QCOMPARE(resultado.valor.piso, "Piso 1");
        QVERIFY(resultado.valor.id > 0);
    }

    void crearAula_nombreVacio_falla() {
        auto resultado = m_servicio->crearAula("", 30);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede estar vacío"));
    }

    void crearAula_capacidadCero_falla() {
        auto resultado = m_servicio->crearAula("Aula 101", 0);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("al menos"));
    }

    void crearAula_capacidadExcesiva_falla() {
        auto resultado = m_servicio->crearAula("Aula 101", 600);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede exceder"));
    }

    void crearAula_nombreDuplicado_falla() {
        auto r1 = m_servicio->crearAula("Aula Duplicada", 25);
        QVERIFY(r1.ok);

        auto r2 = m_servicio->crearAula("Aula Duplicada", 30);
        QVERIFY(!r2.ok);
        QVERIFY(r2.mensajeError.contains("Ya existe"));
    }

    void obtenerAula_exitoso() {
        auto creada = m_servicio->crearAula("Aula 202", 35, "Edificio B");
        QVERIFY(creada.ok);

        auto obtenida = m_servicio->obtenerAula(creada.valor.id);
        QVERIFY(obtenida.ok);
        QCOMPARE(obtenida.valor.nombre, "Aula 202");
        QCOMPARE(obtenida.valor.capacidad, 35);
        QCOMPARE(obtenida.valor.edificio, "Edificio B");
    }

    void obtenerAula_inexistente_falla() {
        auto resultado = m_servicio->obtenerAula(9999);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void listarAulas_vacio() {
        auto lista = m_servicio->listarAulas();
        QVERIFY(lista.isEmpty());
    }

    void listarAulas_conDatos() {
        m_servicio->crearAula("Aula Z", 20);
        m_servicio->crearAula("Aula A", 25);
        m_servicio->crearAula("Aula M", 30);

        auto lista = m_servicio->listarAulas();
        QCOMPARE(lista.size(), 3);

        // Debe estar ordenado por nombre
        QCOMPARE(lista[0].nombre, "Aula A");
        QCOMPARE(lista[1].nombre, "Aula M");
        QCOMPARE(lista[2].nombre, "Aula Z");
    }

    void actualizarAula_exitoso() {
        auto creada = m_servicio->crearAula("Aula Original", 20);
        QVERIFY(creada.ok);

        auto actualizada = m_servicio->actualizarAula(
            creada.valor.id, "Aula Renovada", 45, "Edificio C", "Piso 3"
        );

        QVERIFY(actualizada.ok);
        QCOMPARE(actualizada.valor.nombre, "Aula Renovada");
        QCOMPARE(actualizada.valor.capacidad, 45);
        QCOMPARE(actualizada.valor.edificio, "Edificio C");
        QCOMPARE(actualizada.valor.piso, "Piso 3");
    }

    void actualizarAula_inexistente_falla() {
        auto resultado = m_servicio->actualizarAula(9999, "Aula", 20);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void eliminarAula_exitoso() {
        auto creada = m_servicio->crearAula("Aula a Eliminar", 20);
        QVERIFY(creada.ok);

        bool eliminado = m_servicio->eliminarAula(creada.valor.id);
        QVERIFY(eliminado);

        auto obtenida = m_servicio->obtenerAula(creada.valor.id);
        QVERIFY(!obtenida.ok);
    }

    void eliminarAula_inexistente_falla() {
        bool eliminado = m_servicio->eliminarAula(9999);
        QVERIFY(!eliminado);
    }

    void obtenerTodasParaSolver() {
        m_servicio->crearAula("Aula 1", 20, "Edif A");
        m_servicio->crearAula("Aula 2", 30, "Edif B", "Piso 2");

        auto aulas = m_servicio->obtenerTodasParaSolver();

        QCOMPARE(aulas.size(), 2);
        QCOMPARE(aulas[0].nombre, "Aula 1");
        QCOMPARE(aulas[0].capacidad, 20);
        QCOMPARE(aulas[1].nombre, "Aula 2");
        QCOMPARE(aulas[1].capacidad, 30);
    }

    void obtenerAulaParaSolver() {
        auto creada = m_servicio->crearAula("Aula Solver", 25, "Edif C");
        QVERIFY(creada.ok);

        auto resultado = m_servicio->obtenerAulaParaSolver(creada.valor.id);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Aula Solver");
        QCOMPARE(resultado.valor.capacidad, 25);
    }

    void obtenerAula_idCero_falla() {
        auto resultado = m_servicio->obtenerAula(0);
        QVERIFY(!resultado.ok);
    }

    void obtenerAula_idNegativo_falla() {
        auto resultado = m_servicio->obtenerAula(-5);
        QVERIFY(!resultado.ok);
    }

    void eliminarAula_idCero_falla() {
        bool eliminado = m_servicio->eliminarAula(0);
        QVERIFY(!eliminado);
    }

    void eliminarAula_idNegativo_falla() {
        bool eliminado = m_servicio->eliminarAula(-1);
        QVERIFY(!eliminado);
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioAula> m_servicio;
};

QTEST_MAIN(TestServicioAula)
#include "test_servicio_aula.moc"
