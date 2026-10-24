#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioAula.hpp>
#include <backend/services/ServicioMaterias.hpp>
#include <backend/services/ServicioTiposAula.hpp>

/// Tests del catálogo de tipos de aula (RF-5): clave normalizada almacenada,
/// unicidad ASCII y acentuada, renombrado que propaga por referencia y borrado
/// rechazado mientras haya aulas o materias que lo usen.
class TestServicioTiposAula : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_tempDir->filePath("test_tipos.db")));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioTiposAula(m_dbManager->database()));
    }

    void cleanup() {
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_tempDir.reset();
    }

    // ─── claveTipo ──────────────────────────────────────────────────────────

    void claveTipo_normalizaRecorteYPlegado()
    {
        QCOMPARE(ServicioTiposAula::claveTipo(QStringLiteral("  Laboratorio  ")),
                 QStringLiteral("laboratorio"));
        QCOMPARE(ServicioTiposAula::claveTipo(QStringLiteral("FÍSICA")),
                 QStringLiteral("física"));
        QCOMPARE(ServicioTiposAula::claveTipo(QStringLiteral("   ")), QString());
        QCOMPARE(ServicioTiposAula::claveTipo(QString()), QString());
    }

    // ─── crearTipoAula ──────────────────────────────────────────────────────

    void crearTipoAula_exitoso()
    {
        const auto resultado = m_servicio->crearTipoAula(QStringLiteral("  Laboratorio  "));

        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.id > 0);
        // El nombre visible se conserva (solo recortado); la clave se pliega.
        QCOMPARE(resultado.valor.nombre, QStringLiteral("Laboratorio"));
        QCOMPARE(claveGuardada(resultado.valor.id), QStringLiteral("laboratorio"));
    }

    void crearTipoAula_acentuada_guardaClavePlegada()
    {
        const auto resultado = m_servicio->crearTipoAula(QStringLiteral("Física"));

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.nombre, QStringLiteral("Física"));
        QCOMPARE(claveGuardada(resultado.valor.id), QStringLiteral("física"));
    }

    void crearTipoAula_nombreVacio_falla()
    {
        const auto vacio = m_servicio->crearTipoAula(QString());
        QVERIFY(!vacio.ok);
        QVERIFY(vacio.mensajeError.contains(QStringLiteral("no puede estar vacío")));

        const auto espacios = m_servicio->crearTipoAula(QStringLiteral("   "));
        QVERIFY(!espacios.ok);
        QVERIFY(espacios.mensajeError.contains(QStringLiteral("no puede estar vacío")));

        QCOMPARE(contarTipos(), 0);
    }

    void crearTipoAula_duplicadoExacto_falla()
    {
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Laboratorio")).ok);

        const auto duplicado = m_servicio->crearTipoAula(QStringLiteral("Laboratorio"));

        QVERIFY(!duplicado.ok);
        QVERIFY(duplicado.mensajeError.contains(QStringLiteral("Ya existe")));
        QCOMPARE(contarTipos(), 1);
    }

    void crearTipoAula_duplicadoSoloMayusculas_falla()
    {
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Laboratorio")).ok);

        const auto duplicado = m_servicio->crearTipoAula(QStringLiteral("laboratorio"));

        QVERIFY(!duplicado.ok);
        QVERIFY(duplicado.mensajeError.contains(QStringLiteral("Ya existe")));
        QCOMPARE(contarTipos(), 1);
    }

    void crearTipoAula_duplicadoAcentuado_falla()
    {
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Física")).ok);

        const auto duplicado = m_servicio->crearTipoAula(QStringLiteral("FÍSICA"));

        QVERIFY(!duplicado.ok);
        QVERIFY(duplicado.mensajeError.contains(QStringLiteral("Ya existe")));
        QCOMPARE(contarTipos(), 1);
    }

    void crearTipoAula_duplicadoConEspaciosExtremos_falla()
    {
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Taller")).ok);

        const auto duplicado = m_servicio->crearTipoAula(QStringLiteral("  taller  "));

        QVERIFY(!duplicado.ok);
        QVERIFY(duplicado.mensajeError.contains(QStringLiteral("Ya existe")));
        QCOMPARE(contarTipos(), 1);
    }

    void crearTipoAula_claveDirectaSinNormalizar_frena()
    {
        // Escritura directa en la tabla (sin pasar por el servicio): la clave
        // guardada no está normalizada, pero la comparación en memoria sí.
        QSqlQuery directa(m_dbManager->database());
        directa.prepare(QStringLiteral(
            "INSERT INTO TiposAula (nombre, clave) VALUES ('Taller', '  Taller ')"));
        QVERIFY(directa.exec());

        const auto duplicado = m_servicio->crearTipoAula(QStringLiteral("taller"));

        QVERIFY(!duplicado.ok);
        QVERIFY(duplicado.mensajeError.contains(QStringLiteral("Ya existe")));
        QCOMPARE(contarTipos(), 1);
    }

    // ─── listar / obtener ───────────────────────────────────────────────────

    void listarTiposAula_vacio()
    {
        const auto lista = m_servicio->listarTiposAula();

        QVERIFY(lista.ok);
        QVERIFY(lista.valor.isEmpty());
    }

    void listarTiposAula_ordenadoPorNombre()
    {
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Taller")).ok);
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Aula de estudio")).ok);
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Laboratorio")).ok);

        const auto lista = m_servicio->listarTiposAula();

        QVERIFY(lista.ok);
        QCOMPARE(lista.valor.size(), 3);
        QCOMPARE(lista.valor[0].nombre, QStringLiteral("Aula de estudio"));
        QCOMPARE(lista.valor[1].nombre, QStringLiteral("Laboratorio"));
        QCOMPARE(lista.valor[2].nombre, QStringLiteral("Taller"));
    }

    void obtenerTipoAula_exitoso()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Laboratorio"));
        QVERIFY(creado.ok);

        const auto obtenido = m_servicio->obtenerTipoAula(creado.valor.id);

        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.id, creado.valor.id);
        QCOMPARE(obtenido.valor.nombre, QStringLiteral("Laboratorio"));
    }

    void obtenerTipoAula_inexistente_falla()
    {
        const auto resultado = m_servicio->obtenerTipoAula(9999);

        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("No se encontró")));
    }

    void obtenerTipoAula_idInvalido_falla()
    {
        QVERIFY(!m_servicio->obtenerTipoAula(0).ok);
        QVERIFY(!m_servicio->obtenerTipoAula(-3).ok);
    }

    // ─── renombrarTipoAula ──────────────────────────────────────────────────

    void renombrarTipoAula_exitoso()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Laboratorio"));
        QVERIFY(creado.ok);

        const auto renombrado = m_servicio->renombrarTipoAula(
            creado.valor.id, QStringLiteral("  Laboratorio de Física  "));

        QVERIFY(renombrado.ok);
        QCOMPARE(renombrado.valor.id, creado.valor.id);
        QCOMPARE(renombrado.valor.nombre, QStringLiteral("Laboratorio de Física"));
        QCOMPARE(claveGuardada(creado.valor.id), QStringLiteral("laboratorio de física"));
        QCOMPARE(contarTipos(), 1);
    }

    void renombrarTipoAula_soloMayusculas_noDuplica()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Laboratorio"));
        QVERIFY(creado.ok);

        const auto renombrado = m_servicio->renombrarTipoAula(
            creado.valor.id, QStringLiteral("LABORATORIO"));

        QVERIFY(renombrado.ok);
        QCOMPARE(renombrado.valor.id, creado.valor.id);
        QCOMPARE(renombrado.valor.nombre, QStringLiteral("LABORATORIO"));
        QCOMPARE(claveGuardada(creado.valor.id), QStringLiteral("laboratorio"));
        QCOMPARE(contarTipos(), 1);
    }

    void renombrarTipoAula_aOtraClaveExistente_falla()
    {
        const auto laboratorio = m_servicio->crearTipoAula(QStringLiteral("Laboratorio"));
        QVERIFY(laboratorio.ok);
        QVERIFY(m_servicio->crearTipoAula(QStringLiteral("Física")).ok);

        const auto renombrado = m_servicio->renombrarTipoAula(
            laboratorio.valor.id, QStringLiteral("FÍSICA"));

        QVERIFY(!renombrado.ok);
        QVERIFY(renombrado.mensajeError.contains(QStringLiteral("Ya existe")));

        // No se tocó nada: el nombre original sigue publicado.
        const auto original = m_servicio->obtenerTipoAula(laboratorio.valor.id);
        QVERIFY(original.ok);
        QCOMPARE(original.valor.nombre, QStringLiteral("Laboratorio"));
        QCOMPARE(contarTipos(), 2);
    }

    void renombrarTipoAula_nombreVacio_falla()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Laboratorio"));
        QVERIFY(creado.ok);

        const auto renombrado = m_servicio->renombrarTipoAula(creado.valor.id,
                                                             QStringLiteral("   "));

        QVERIFY(!renombrado.ok);
        QVERIFY(renombrado.mensajeError.contains(QStringLiteral("no puede estar vacío")));
        QCOMPARE(contarTipos(), 1);
    }

    void renombrarTipoAula_inexistente_falla()
    {
        const auto renombrado = m_servicio->renombrarTipoAula(9999, QStringLiteral("Taller"));

        QVERIFY(!renombrado.ok);
        QVERIFY(renombrado.mensajeError.contains(QStringLiteral("No se encontró")));
    }

    void renombrarTipoAula_idInvalido_falla()
    {
        QVERIFY(!m_servicio->renombrarTipoAula(0, QStringLiteral("Taller")).ok);
        QVERIFY(!m_servicio->renombrarTipoAula(-1, QStringLiteral("Taller")).ok);
    }

    void renombrarTipoAula_propagaAAulasYMaterias_sinReescribir()
    {
        const auto tipo = m_servicio->crearTipoAula(QStringLiteral("Aula de estudio"));
        QVERIFY(tipo.ok);

        ServicioAula aulas(m_dbManager->database());
        ServicioMaterias materias(m_dbManager->database());

        const auto aula = aulas.crearAula(QStringLiteral("Aula 101"), 30,
                                          QStringLiteral("Edificio A"),
                                          QStringLiteral("Piso 1"), tipo.valor.id);
        QVERIFY(aula.ok);
        const auto materia = materias.crearMateria(QStringLiteral("Matemática"),
                                                   QStringLiteral("Álgebra"),
                                                   tipo.valor.id);
        QVERIFY(materia.ok);

        QVERIFY(m_servicio->renombrarTipoAula(tipo.valor.id,
                                              QStringLiteral("Aula común")).ok);

        const auto aulaDespues = aulas.obtenerAula(aula.valor.id);
        QVERIFY(aulaDespues.ok);
        QCOMPARE(aulaDespues.valor.idTipoAula, tipo.valor.id);
        QCOMPARE(aulaDespues.valor.tipoAula, QStringLiteral("Aula común"));
        // Los datos propios del aula no se reescriben.
        QCOMPARE(aulaDespues.valor.nombre, QStringLiteral("Aula 101"));
        QCOMPARE(aulaDespues.valor.capacidad, 30);
        QCOMPARE(aulaDespues.valor.edificio, QStringLiteral("Edificio A"));
        QCOMPARE(aulaDespues.valor.piso, QStringLiteral("Piso 1"));

        const auto materiaDespues = materias.obtenerMateria(materia.valor.id);
        QVERIFY(materiaDespues.ok);
        QCOMPARE(materiaDespues.valor.idTipoAula, tipo.valor.id);
        QCOMPARE(materiaDespues.valor.tipoAula, QStringLiteral("Aula común"));
        QCOMPARE(materiaDespues.valor.nombre, QStringLiteral("Matemática"));
        QCOMPARE(materiaDespues.valor.requisitos, QStringLiteral("Álgebra"));

        QCOMPARE(nombreDeLaFilaAula(aula.valor.id), QStringLiteral("Aula 101"));
        QCOMPARE(nombreDeLaFilaMateria(materia.valor.id), QStringLiteral("Matemática"));
    }

    // ─── eliminarTipoAula ───────────────────────────────────────────────────

    void eliminarTipoAula_libre_exitoso()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Taller"));
        QVERIFY(creado.ok);

        const auto eliminado = m_servicio->eliminarTipoAula(creado.valor.id);

        QVERIFY(eliminado.ok);
        QVERIFY(eliminado.valor);
        QVERIFY(!m_servicio->obtenerTipoAula(creado.valor.id).ok);
        QCOMPARE(contarTipos(), 0);
    }

    void eliminarTipoAula_enUsoPorAula_falla()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Aula de estudio"));
        QVERIFY(creado.ok);

        ServicioAula aulas(m_dbManager->database());
        QVERIFY(aulas.crearAula(QStringLiteral("Aula 101"), 30, QStringLiteral("Edificio A"),
                                QString(), creado.valor.id).ok);

        const auto eliminado = m_servicio->eliminarTipoAula(creado.valor.id);

        QVERIFY(!eliminado.ok);
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("en uso")));
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("Aula 101")));
        QCOMPARE(contarTipos(), 1);
    }

    void eliminarTipoAula_enUsoPorMateria_falla()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Laboratorio"));
        QVERIFY(creado.ok);

        ServicioMaterias materias(m_dbManager->database());
        QVERIFY(materias.crearMateria(QStringLiteral("Física"), QString(), creado.valor.id).ok);

        const auto eliminado = m_servicio->eliminarTipoAula(creado.valor.id);

        QVERIFY(!eliminado.ok);
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("en uso")));
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("Física")));
        QCOMPARE(contarTipos(), 1);
    }

    void eliminarTipoAula_enUsoPorAulaYMateria_listaAmbos()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Aula de estudio"));
        QVERIFY(creado.ok);

        ServicioAula aulas(m_dbManager->database());
        ServicioMaterias materias(m_dbManager->database());
        QVERIFY(aulas.crearAula(QStringLiteral("Aula 101"), 30, QString(), QString(),
                                creado.valor.id).ok);
        QVERIFY(materias.crearMateria(QStringLiteral("Matemática"), QString(),
                                      creado.valor.id).ok);

        const auto eliminado = m_servicio->eliminarTipoAula(creado.valor.id);

        QVERIFY(!eliminado.ok);
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("aulas")));
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("Aula 101")));
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("materias")));
        QVERIFY(eliminado.mensajeError.contains(QStringLiteral("Matemática")));
        QCOMPARE(contarTipos(), 1);
    }

    void eliminarTipoAula_trasBorrarDependientes_exitoso()
    {
        const auto creado = m_servicio->crearTipoAula(QStringLiteral("Taller"));
        QVERIFY(creado.ok);

        ServicioAula aulas(m_dbManager->database());
        const auto aula = aulas.crearAula(QStringLiteral("Taller 1"), 20, QString(), QString(),
                                          creado.valor.id);
        QVERIFY(aula.ok);
        QVERIFY(aulas.eliminarAula(aula.valor.id));

        QVERIFY(m_servicio->eliminarTipoAula(creado.valor.id).ok);
        QCOMPARE(contarTipos(), 0);
    }

    void eliminarTipoAula_inexistente_falla()
    {
        QVERIFY(!m_servicio->eliminarTipoAula(9999).ok);
    }

    void eliminarTipoAula_idInvalido_falla()
    {
        QVERIFY(!m_servicio->eliminarTipoAula(0).ok);
        QVERIFY(!m_servicio->eliminarTipoAula(-2).ok);
    }

private:
    QString claveGuardada(int id) const
    {
        QSqlQuery query(m_dbManager->database());
        query.prepare(QStringLiteral("SELECT clave FROM TiposAula WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), id);
        if (!query.exec() || !query.next()) {
            return QString();
        }
        return query.value(0).toString();
    }

    int contarTipos() const
    {
        QSqlQuery query(m_dbManager->database());
        if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM TiposAula")) || !query.next()) {
            return -1;
        }
        return query.value(0).toInt();
    }

    QString nombreDeLaFilaAula(int id) const
    {
        QSqlQuery query(m_dbManager->database());
        query.prepare(QStringLiteral("SELECT nombre FROM Aulas WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), id);
        if (!query.exec() || !query.next()) {
            return QString();
        }
        return query.value(0).toString();
    }

    QString nombreDeLaFilaMateria(int id) const
    {
        QSqlQuery query(m_dbManager->database());
        query.prepare(QStringLiteral("SELECT nombre FROM Materias WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), id);
        if (!query.exec() || !query.next()) {
            return QString();
        }
        return query.value(0).toString();
    }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioTiposAula> m_servicio;
};

QTEST_MAIN(TestServicioTiposAula)
#include "test_servicio_tipos_aula.moc"
