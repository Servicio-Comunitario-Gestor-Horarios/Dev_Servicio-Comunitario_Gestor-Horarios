#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioMaterias.hpp>
#include <backend/services/ServicioTiposAula.hpp>

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

        m_tipos.reset(new ServicioTiposAula(m_dbManager->database()));
        const auto tipo = m_tipos->crearTipoAula(QStringLiteral("Aula de estudio"));
        QVERIFY(tipo.ok);
        m_idTipoAula = tipo.valor.id;
    }

    void cleanup() {
        m_tipos.reset();
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── Tests ──────────────────────────────────────────────────────────────

    void crearMateria_exitoso() {
        auto resultado = m_servicio->crearMateria("Matemática", "Álgebra, Geometría",
                                                 m_idTipoAula);

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Matemática");
        QCOMPARE(resultado.valor.requisitos, "Álgebra, Geometría");
        QVERIFY(resultado.valor.id > 0);
    }

    void crearMateria_sinRequisitos() {
        auto resultado = m_servicio->crearMateria("Física", QString(), m_idTipoAula);

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, "Física");
        QVERIFY(resultado.valor.requisitos.isEmpty());
    }

    void crearMateria_sinTipo_falla() {
        auto resultado = m_servicio->crearMateria("Matemática");

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("tipo de aula"));
        QVERIFY(m_servicio->listarMaterias().valor.isEmpty());
    }

    void crearMateria_tipoInexistente_falla() {
        auto resultado = m_servicio->crearMateria("Matemática", QString(), 9999);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No existe el tipo de aula"));
        QVERIFY(m_servicio->listarMaterias().valor.isEmpty());
    }

    void crearMateria_catalogoVacio_falla() {
        // Base nueva sin tipos: el catálogo vacío impide crear materias.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        DatabaseManager db;
        QVERIFY(db.initialize(dir.filePath("sin_tipos.db")));

        ServicioMaterias servicio(db.database());

        auto sinTipo = servicio.crearMateria("Matemática");
        QVERIFY(!sinTipo.ok);
        QVERIFY(sinTipo.mensajeError.contains("No hay tipos de aula registrados"));

        auto tipoInexistente = servicio.crearMateria("Matemática", QString(), 1);
        QVERIFY(!tipoInexistente.ok);
        QVERIFY(servicio.listarMaterias().valor.isEmpty());

        db.close();
    }

    void crearMateria_exponeElTipo() {
        const auto creada = m_servicio->crearMateria("Matemática", QString(), m_idTipoAula);
        QVERIFY(creada.ok);

        QCOMPARE(creada.valor.idTipoAula, m_idTipoAula);
        QCOMPARE(creada.valor.tipoAula, QString("Aula de estudio"));

        const auto obtenida = m_servicio->obtenerMateria(creada.valor.id);
        QVERIFY(obtenida.ok);
        QCOMPARE(obtenida.valor.idTipoAula, m_idTipoAula);
        QCOMPARE(obtenida.valor.tipoAula, QString("Aula de estudio"));

        const auto lista = m_servicio->listarMaterias();
        QVERIFY(lista.ok);
        QCOMPARE(lista.valor.size(), 1);
        QCOMPARE(lista.valor[0].idTipoAula, m_idTipoAula);
        QCOMPARE(lista.valor[0].tipoAula, QString("Aula de estudio"));
    }

    void crearMateria_nombreVacio_falla() {
        auto resultado = m_servicio->crearMateria("", QString(), m_idTipoAula);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("no puede estar vacío"));
    }

    void crearMateria_nombreDuplicado_falla() {
        auto r1 = m_servicio->crearMateria("Química", QString(), m_idTipoAula);
        QVERIFY(r1.ok);

        auto r2 = m_servicio->crearMateria("Química", QString(), m_idTipoAula);
        QVERIFY(!r2.ok);
        QVERIFY(r2.mensajeError.contains("Ya existe"));
    }

    void obtenerMateria_exitoso() {
        auto creada = m_servicio->crearMateria("Biología", "Química", m_idTipoAula);
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
        m_servicio->crearMateria("Zootecnia", QString(), m_idTipoAula);
        m_servicio->crearMateria("Agronomía", QString(), m_idTipoAula);
        m_servicio->crearMateria("Botánica", QString(), m_idTipoAula);

        auto lista = m_servicio->listarMaterias().valor;
        QCOMPARE(lista.size(), 3);

        // Ordenado por nombre
        QCOMPARE(lista[0].nombre, "Agronomía");
        QCOMPARE(lista[1].nombre, "Botánica");
        QCOMPARE(lista[2].nombre, "Zootecnia");
    }

    void actualizarMateria_exitoso() {
        auto creada = m_servicio->crearMateria("Matemática Antigua", QString(), m_idTipoAula);
        QVERIFY(creada.ok);

        auto actualizada = m_servicio->actualizarMateria(
            creada.valor.id, "Matemática Moderna", "Álgebra, Cálculo", m_idTipoAula
        );

        QVERIFY(actualizada.ok);
        QCOMPARE(actualizada.valor.nombre, "Matemática Moderna");
        QCOMPARE(actualizada.valor.requisitos, "Álgebra, Cálculo");
    }

    void actualizarMateria_sinTipo_falla() {
        auto creada = m_servicio->crearMateria("Matemática Antigua", QString(), m_idTipoAula);
        QVERIFY(creada.ok);

        auto actualizada = m_servicio->actualizarMateria(creada.valor.id,
                                                        "Matemática Moderna");

        QVERIFY(!actualizada.ok);
        QVERIFY(actualizada.mensajeError.contains("tipo de aula"));

        // No se conserva el tipo anterior de forma implícita: la materia sigue igual.
        auto sinCambios = m_servicio->obtenerMateria(creada.valor.id);
        QVERIFY(sinCambios.ok);
        QCOMPARE(sinCambios.valor.nombre, "Matemática Antigua");
        QCOMPARE(sinCambios.valor.idTipoAula, m_idTipoAula);
    }

    void actualizarMateria_cambiaTipo() {
        auto creada = m_servicio->crearMateria("Física", QString(), m_idTipoAula);
        QVERIFY(creada.ok);

        const auto otro = m_tipos->crearTipoAula(QStringLiteral("Laboratorio"));
        QVERIFY(otro.ok);

        auto actualizada = m_servicio->actualizarMateria(creada.valor.id, "Física", QString(),
                                                         otro.valor.id);

        QVERIFY(actualizada.ok);
        QCOMPARE(actualizada.valor.idTipoAula, otro.valor.id);
        QCOMPARE(actualizada.valor.tipoAula, QString("Laboratorio"));
    }

    void actualizarMateria_inexistente_falla() {
        auto resultado = m_servicio->actualizarMateria(9999, "Materia", QString(), m_idTipoAula);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void eliminarMateria_exitoso() {
        auto creada = m_servicio->crearMateria("Materia a Eliminar", QString(), m_idTipoAula);
        QVERIFY(creada.ok);

        bool eliminado = m_servicio->eliminarMateria(creada.valor.id);
        QVERIFY(eliminado);

        auto obtenida = m_servicio->obtenerMateria(creada.valor.id);
        QVERIFY(!obtenida.ok);
    }

    void eliminarMateria_conPlanAsociado_falla() {
        auto materia = m_servicio->crearMateria("Materia Plan", QString(), m_idTipoAula);
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

    void obtenerMateria_idCero_falla() {
        auto resultado = m_servicio->obtenerMateria(0);
        QVERIFY(!resultado.ok);
    }

    void obtenerMateria_idNegativo_falla() {
        auto resultado = m_servicio->obtenerMateria(-3);
        QVERIFY(!resultado.ok);
    }

    void eliminarMateria_idCero_falla() {
        bool eliminado = m_servicio->eliminarMateria(0);
        QVERIFY(!eliminado);
    }

    void eliminarMateria_idNegativo_falla() {
        bool eliminado = m_servicio->eliminarMateria(-1);
        QVERIFY(!eliminado);
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioMaterias> m_servicio;
    std::unique_ptr<ServicioTiposAula> m_tipos;
    int m_idTipoAula = -1;
};

QTEST_MAIN(TestServicioMaterias)
#include "test_servicio_materias.moc"
