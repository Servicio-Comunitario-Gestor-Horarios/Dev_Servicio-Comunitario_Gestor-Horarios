#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioMaterias.hpp>

class TestServicioMaterias : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QString dbPath = m_tempDir->filePath("test_materias.db");
        QVERIFY(m_dbManager->initialize(dbPath));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioMaterias(m_dbManager->database()));
    }

    void cleanup() {
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Tests ──────────────────────────────────────────────────────────────

    void crearMateria_exitoso() {
        auto resultado = m_servicio->crearMateria("Matemática", "Álgebra, Geometría");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Matemática");
        QCOMPARE(resultado.valor.requisitos, "Álgebra, Geometría");
        QVERIFY(resultado.valor.id > 0);
    }

    void crearMateria_sinRequisitos() {
        auto resultado = m_servicio->crearMateria("Física");

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Física");
        QVERIFY(resultado.valor.requisitos.isEmpty());
    }

    void crearMateria_nombreVacio_falla() {
        auto resultado = m_servicio->crearMateria("");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede estar vacío"));
    }

    void crearMateria_nombreDuplicado_falla() {
        auto r1 = m_servicio->crearMateria("Química");
        QVERIFY(r1.ok);

        auto r2 = m_servicio->crearMateria("Química");
        QVERIFY(!r2.ok);
        QVERIFY(r2.mensajeError.contains("Ya existe"));
    }

    void obtenerMateria_exitoso() {
        auto creada = m_servicio->crearMateria("Biología", "Química");
        QVERIFY(creada.ok);

        auto obtenida = m_servicio->obtenerMateria(creada.valor.id);
        QVERIFY(obtenida.ok);
        QCOMPARE(obtenida.valor.nombre, "Biología");
        QCOMPARE(obtenida.valor.requisitos, "Química");
    }

    void obtenerMateria_inexistente_falla() {
        auto resultado = m_servicio->obtenerMateria(9999);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void listarMaterias() {
        m_servicio->crearMateria("Zootecnia");
        m_servicio->crearMateria("Agronomía");
        m_servicio->crearMateria("Botánica");

        auto lista = m_servicio->listarMaterias();
        QCOMPARE(lista.size(), 3);

        // Ordenado por nombre
        QCOMPARE(lista[0].nombre, "Agronomía");
        QCOMPARE(lista[1].nombre, "Botánica");
        QCOMPARE(lista[2].nombre, "Zootecnia");
    }

    void actualizarMateria_exitoso() {
        auto creada = m_servicio->crearMateria("Matemática Antigua");
        QVERIFY(creada.ok);

        auto actualizada = m_servicio->actualizarMateria(
            creada.valor.id, "Matemática Moderna", "Álgebra, Cálculo"
        );

        QVERIFY(actualizada.ok);
        QCOMPARE(actualizada.valor.nombre, "Matemática Moderna");
        QCOMPARE(actualizada.valor.requisitos, "Álgebra, Cálculo");
    }

    void actualizarMateria_inexistente_falla() {
        auto resultado = m_servicio->actualizarMateria(9999, "Materia");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void eliminarMateria_exitoso() {
        auto creada = m_servicio->crearMateria("Materia a Eliminar");
        QVERIFY(creada.ok);

        bool eliminado = m_servicio->eliminarMateria(creada.valor.id);
        QVERIFY(eliminado);

        auto obtenida = m_servicio->obtenerMateria(creada.valor.id);
        QVERIFY(!obtenida.ok);
    }

    void eliminarMateria_conPlanAsociado_falla() {
        auto materia = m_servicio->crearMateria("Materia Plan");
        QVERIFY(materia.ok);

        // Crear un plan y asociar la materia
        QSqlQuery query(m_dbManager->database());
        query.prepare("INSERT INTO PlanEstudio (codigo, nombre) VALUES ('PLAN001', 'Plan Test')");
        QVERIFY(query.exec());

        query.prepare(
            "INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas) "
            "VALUES ('PLAN001', :id, 1, 4)"
        );
        query.bindValue(":id", materia.valor.id);
        QVERIFY(query.exec());

        bool eliminado = m_servicio->eliminarMateria(materia.valor.id);
        QVERIFY(!eliminado); // No debe eliminar porque está asociada
    }

    void toMateria_conRequisitos() {
        MateriaDTO dto;
        dto.id = 1;
        dto.nombre = "Matemática";
        dto.requisitos = "Álgebra, Geometría, Cálculo";

        auto materia = dto.toMateria(5);

        QCOMPARE(materia.nombre, "Matemática");
        QCOMPARE(materia.horas_semanales, 5);
        QCOMPARE(materia.requerimientos.size(), 3);
        QCOMPARE(materia.requerimientos[0], "Álgebra");
        QCOMPARE(materia.requerimientos[1], "Geometría");
        QCOMPARE(materia.requerimientos[2], "Cálculo");
    }

    void toMateria_sinRequisitos() {
        MateriaDTO dto;
        dto.id = 1;
        dto.nombre = "Física";
        dto.requisitos = "";

        auto materia = dto.toMateria(4);

        QCOMPARE(materia.nombre, "Física");
        QCOMPARE(materia.horas_semanales, 4);
        QVERIFY(materia.requerimientos.isEmpty());
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioMaterias> m_servicio;
};

QTEST_MAIN(TestServicioMaterias)
#include "test_servicio_materias.moc"