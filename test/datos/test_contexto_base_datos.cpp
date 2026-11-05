#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <datos/ContextoBaseDatos.hpp>
#include <backend/database/apertura_base_datos.hpp>
#include <backend/database/version_esquema.hpp>
#include <backend/services/NucleoDatos.hpp>

using AperturaBaseDatos::Estado;

namespace {

/// Id de la fila del tipo general que deja el esquema en una base nueva (RF-1).
int idTipoGeneral(QSqlDatabase& db)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id FROM TiposAula WHERE clave = 'aula de estudio'"));
    if (!query.exec() || !query.next())
        return -1;
    return query.value(0).toInt();
}

} // namespace

/// Tests del componente que encapsula el ciclo de vida de la base del cliente.
/// Verifica que el contexto abre/migra/respalda, construye el núcleo, aísla sus
/// conexiones y las libera al destruirse (RAII), sin depender de la interfaz.
class TestContextoBaseDatos : public QObject
{
    Q_OBJECT

private:
    int m_contadorConexiones = 0;

    /// Abre una conexión temporal con nombre único (para preparar/inspeccionar
    /// archivos de base sin tocar el contexto bajo prueba).
    QSqlDatabase abrirConexionTemporal(const QString& ruta, int version)
    {
        const QString nombre =
            QStringLiteral("test_contexto_aux_%1").arg(++m_contadorConexiones);
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", nombre);
        db.setDatabaseName(ruta);
        db.open();
        if (version >= 0)
            VersionEsquema::fijarVersionEsquema(db, version);
        return db;
    }

    void cerrarConexionTemporal(QSqlDatabase& db)
    {
        const QString nombre = db.connectionName();
        if (db.isOpen())
            db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(nombre);
    }

    /// Deja en `ruta` una base con la versión de esquema indicada.
    void prepararBaseVersion(const QString& ruta, int version)
    {
        // Base válida con el esquema actual…
        const AperturaBaseDatos::Resultado creada = AperturaBaseDatos::abrir(ruta);
        Q_UNUSED(creada)
        // …y se ajusta la versión para el escenario deseado.
        QSqlDatabase db = abrirConexionTemporal(ruta, version);
        cerrarConexionTemporal(db);
    }

private slots:

    void abrir_inexistente_creaYDejaOperativa()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("nueva.db");
        QVERIFY(!QFile::exists(ruta));

        ContextoBaseDatos contexto;
        const AperturaBaseDatos::Resultado resultado = contexto.abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkCreada);
        QVERIFY(resultado.ok());
        QVERIFY(contexto.abierto());
        QCOMPARE(contexto.ruta(), ruta);
        QVERIFY(QFile::exists(ruta));
    }

    void abrir_inexistente_nucleoOpera()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("crud.db");

        ContextoBaseDatos contexto;
        QVERIFY(contexto.abrir(ruta).ok());

        const int antes = contexto.nucleo().listarMaterias().valor.size();
        const int idGeneral = idTipoGeneral(contexto.conexion());
        QVERIFY(idGeneral > 0);
        const auto alta = contexto.nucleo().crearMateria(QStringLiteral("Matemáticas"),
                                                         QString(), idGeneral);
        QVERIFY(alta.ok);
        QCOMPARE(contexto.nucleo().listarMaterias().valor.size(), antes + 1);
    }

    void abrir_versionActual_abreSinRespaldo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("actual.db");
        prepararBaseVersion(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        ContextoBaseDatos contexto;
        const AperturaBaseDatos::Resultado resultado = contexto.abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkAbierta);
        QVERIFY(resultado.ok());
        QVERIFY(contexto.abierto());
    }

    void abrir_versionPosterior_fallaYNoAbre()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("posterior.db");
        prepararBaseVersion(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL + 1);

        ContextoBaseDatos contexto;
        const AperturaBaseDatos::Resultado resultado = contexto.abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloEsquema);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
        QVERIFY(!contexto.abierto());
    }

    void abrir_versionAusente_falla()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("ausente.db");
        prepararBaseVersion(ruta, 0);

        ContextoBaseDatos contexto;
        const AperturaBaseDatos::Resultado resultado = contexto.abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloEsquema);
        QVERIFY(!resultado.ok());
        QVERIFY(!contexto.abierto());
    }

    void dosInstancias_noColisionanConexion()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ContextoBaseDatos uno;
        ContextoBaseDatos otro;
        QVERIFY(uno.abrir(dir.filePath("uno.db")).ok());
        QVERIFY(otro.abrir(dir.filePath("otro.db")).ok());

        QVERIFY(uno.abierto());
        QVERIFY(otro.abierto());
        QVERIFY(uno.conexion().connectionName() != otro.conexion().connectionName());

        // Cada contexto opera sobre su propia base.
        const int idGeneral = idTipoGeneral(uno.conexion());
        QVERIFY(idGeneral > 0);
        QVERIFY(uno.nucleo().crearMateria(QStringLiteral("SoloUno"), QString(),
                                          idGeneral).ok);
        QCOMPARE(otro.nucleo().listarMaterias().valor.size(), 0);
    }

    void abrir_reintentoSobreAbierto_cierraAnterior()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ContextoBaseDatos contexto;
        QVERIFY(contexto.abrir(dir.filePath("primera.db")).ok());
        const QString primeraConexion = contexto.conexion().connectionName();

        QVERIFY(contexto.abrir(dir.filePath("segunda.db")).ok());
        QVERIFY(contexto.abierto());
        QVERIFY(contexto.ruta().endsWith("segunda.db"));
        // La conexión anterior se liberó y se registró una nueva.
        QVERIFY(contexto.conexion().connectionName() != primeraConexion);

        // La primera base sigue existiendo en disco, ya cerrada.
        QVERIFY(!QSqlDatabase::contains(primeraConexion));
    }

    void destruirYReabrir_mismasRuta()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("reabrir.db");

        {
            ContextoBaseDatos contexto;
            QVERIFY(contexto.abrir(ruta).ok());
            const int idGeneral = idTipoGeneral(contexto.conexion());
            QVERIFY(idGeneral > 0);
            QVERIFY(contexto.nucleo().crearMateria(QStringLiteral("Persistente"), QString(),
                                                   idGeneral).ok);
        } // RAII: cierra y libera la conexión

        ContextoBaseDatos reabierto;
        const AperturaBaseDatos::Resultado resultado = reabierto.abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkAbierta);
        QVERIFY(resultado.ok());
        QCOMPARE(reabierto.nucleo().listarMaterias().valor.size(), 1);
    }

    void rutaPorDefecto_noVacia()
    {
        QVERIFY(!ContextoBaseDatos::rutaPorDefecto().isEmpty());
    }
};

QTEST_MAIN(TestContextoBaseDatos)
#include "test_contexto_base_datos.moc"
