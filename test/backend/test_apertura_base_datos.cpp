#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <backend/database/apertura_base_datos.hpp>
#include <backend/database/version_esquema.hpp>

using AperturaBaseDatos::Estado;
using AperturaBaseDatos::Opciones;
using AperturaBaseDatos::Resultado;

/// Tests de la apertura con esquema único: crear o abrir, o fallar sin migrar (RF-6).
class TestAperturaBaseDatos : public QObject
{
    Q_OBJECT

private:
    int m_contadorConexiones = 0;

    QSqlDatabase abrirBase(const QString& ruta)
    {
        const QString nombre = QStringLiteral("test_apertura_%1").arg(++m_contadorConexiones);
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombre);
        db.setDatabaseName(ruta);
        db.open();
        return db;
    }

    void cerrarBase(QSqlDatabase& db)
    {
        const QString nombre = db.connectionName();
        if (db.isOpen())
            db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(nombre);
    }

    /// Crea una base con el esquema único y, si se indica, fija otra versión.
    void crearBaseConEsquema(const QString& ruta, int version)
    {
        QSqlDatabase db = abrirBase(ruta);
        QVERIFY(VersionEsquema::crearEsquema(db));
        if (version != VersionEsquema::VERSION_ESQUEMA_ACTUAL)
            QVERIFY(VersionEsquema::fijarVersionEsquema(db, version));
        cerrarBase(db);
    }

    int leerVersion(const QString& ruta)
    {
        QSqlDatabase db = abrirBase(ruta);
        const int version = VersionEsquema::leerVersionEsquema(db);
        cerrarBase(db);
        return version;
    }

    bool tablaExiste(const QString& ruta, const QString& tabla)
    {
        QSqlDatabase db = abrirBase(ruta);
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT name FROM sqlite_master "
                                     "WHERE type='table' AND name=:nombre"));
        query.bindValue(QStringLiteral(":nombre"), tabla);
        const bool existe = query.exec() && query.next();
        cerrarBase(db);
        return existe;
    }

    /// Añade una tabla retirada a una base ya creada.
    void crearTablaRetirada(const QString& ruta)
    {
        QSqlDatabase db = abrirBase(ruta);
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("CREATE TABLE Curso_Materia (id INTEGER)")));
        cerrarBase(db);
    }

private slots:

    void abrir_inexistente_creaEsquemaYVersion()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("nueva.db");
        QVERIFY(!QFile::exists(ruta));

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkCreada);
        QVERIFY(resultado.ok());
        QVERIFY(QFile::exists(ruta));
        QCOMPARE(leerVersion(ruta), VersionEsquema::VERSION_ESQUEMA_ACTUAL);
        QVERIFY(tablaExiste(ruta, QStringLiteral("Cursos")));
        QVERIFY(!tablaExiste(ruta, QStringLiteral("Curso_Materia")));
    }

    void abrir_versionYCatalogoCorrectos_abre()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("actual.db");
        crearBaseConEsquema(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkAbierta);
        QVERIFY(resultado.ok());
    }

    void abrir_versionAusente_fallaEsquema()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("ausente.db");
        crearBaseConEsquema(ruta, 0);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloEsquema);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
    }

    void abrir_versionAnterior_fallaEsquema()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("anterior.db");
        crearBaseConEsquema(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL - 1);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloEsquema);
        QVERIFY(!resultado.ok());
    }

    void abrir_versionPosterior_fallaEsquema()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("posterior.db");
        crearBaseConEsquema(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL + 1);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloEsquema);
        QVERIFY(!resultado.ok());
    }

    void abrir_catalogoNoEsperado_fallaEsquema()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("catalogo.db");
        crearBaseConEsquema(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL);
        crearTablaRetirada(ruta); // Curso_Materia sobra

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloEsquema);
        QVERIFY(!resultado.ok());
    }

    void abrir_falloEsquema_noBorraNiMigra()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("conserva.db");
        crearBaseConEsquema(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL - 1);
        crearTablaRetirada(ruta);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloEsquema);
        // No se migra ni se borran datos: la versión y la tabla siguen ahí.
        QCOMPARE(leerVersion(ruta), VersionEsquema::VERSION_ESQUEMA_ACTUAL - 1);
        QVERIFY(tablaExiste(ruta, QStringLiteral("Curso_Materia")));
    }

    void abrir_rutaVacia_fallaApertura()
    {
        const Resultado resultado = AperturaBaseDatos::abrir(QString());
        QCOMPARE(resultado.estado, Estado::FalloApertura);
        QVERIFY(!resultado.ok());
    }

    void abrir_noAbrible_mensajeSinTextoCrudoDeQt()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("sub/no_existe/base.db");

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloApertura);
        QVERIFY(!resultado.detalle.isEmpty());
        QVERIFY(!resultado.detalle.contains(QStringLiteral("unable to"), Qt::CaseInsensitive));
        QVERIFY(!resultado.detalle.contains(QStringLiteral("no such"), Qt::CaseInsensitive));
    }
};

QTEST_MAIN(TestAperturaBaseDatos)
#include "test_apertura_base_datos.moc"
