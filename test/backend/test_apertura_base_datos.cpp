#include <QDateTime>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <backend/database/apertura_base_datos.hpp>
#include <backend/database/respaldo.hpp>
#include <backend/database/version_esquema.hpp>

using AperturaBaseDatos::Estado;
using AperturaBaseDatos::Opciones;
using AperturaBaseDatos::Resultado;

/// Tests de la apertura de la base con migración y respaldo obligatorio (RF-1, RF-4).
class TestAperturaBaseDatos : public QObject
{
    Q_OBJECT

private:
    int m_contadorConexiones = 0;

    /// Abre una conexión de prueba a una base en archivo con un nombre único.
    QSqlDatabase abrirBase(const QString& ruta)
    {
        const QString nombre =
            QStringLiteral("test_apertura_%1").arg(++m_contadorConexiones);
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", nombre);
        db.setDatabaseName(ruta);
        db.open();
        return db;
    }

    /// Cierra una conexión y la elimina del registro de Qt.
    void cerrarBase(QSqlDatabase& db)
    {
        const QString nombre = db.connectionName();

        if (db.isOpen())
            db.close();

        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(nombre);
    }

    /// Crea una base con el esquema v1 y fija la versión indicada (0 = ausente).
    void crearBaseVersion(const QString& ruta, int version)
    {
        QSqlDatabase db = abrirBase(ruta);
        VersionEsquema::aplicarHasta(db, 1);
        VersionEsquema::fijarVersionEsquema(db, version);
        cerrarBase(db);
    }

    /// Lee la versión de esquema almacenada en un archivo de base de datos.
    int leerVersion(const QString& ruta)
    {
        QSqlDatabase db = abrirBase(ruta);
        const int version = VersionEsquema::leerVersionEsquema(db);
        cerrarBase(db);
        return version;
    }

    /// Indica si una tabla existe en el archivo de base de datos dado.
    bool tablaExiste(const QString& ruta, const QString& tabla)
    {
        QSqlDatabase db = abrirBase(ruta);
        QSqlQuery query(db);
        query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:nombre");
        query.bindValue(":nombre", tabla);
        const bool existe = query.exec() && query.next();
        cerrarBase(db);
        return existe;
    }

private slots:

    // ─── Base inexistente ───────────────────────────────────────────────────

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
        QVERIFY(tablaExiste(ruta, "Profesores"));
        QVERIFY(tablaExiste(ruta, "Aulas"));
    }

    // ─── Base existente, sin migración ──────────────────────────────────────

    void abrir_versionActual_abreSinRespaldo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("actual.db");
        crearBaseVersion(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkAbierta);
        QVERIFY(resultado.ok());
        // Abrir sin migrar no debe crear respaldo alguno.
        QVERIFY(resultado.rutaRespaldo.isEmpty());
    }

    void abrir_versionAusente_falla()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("ausente.db");
        crearBaseVersion(ruta, 0);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloAusente);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
    }

    void abrir_versionPosterior_falla()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("posterior.db");
        crearBaseVersion(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL + 1);

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloPosterior);
        QVERIFY(!resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
    }

    void abrir_rutaVacia_falla()
    {
        const Resultado resultado = AperturaBaseDatos::abrir(QString());

        QCOMPARE(resultado.estado, Estado::FalloApertura);
        QVERIFY(!resultado.ok());
    }

    // ─── Migración con respaldo obligatorio ─────────────────────────────────

    void migracion_respaldaAntesDeMigrar()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("migrar.db");
        crearBaseVersion(ruta, 1);

        const QDateTime ahora(QDate(2026, 10, 8), QTime(12, 0, 0));
        int llamadasRespaldo = 0;
        QStringList orden;
        QString rutaRespaldoRegistrada;

        Opciones opciones;
        opciones.versionEsperada = 2;
        opciones.ahora = ahora;
        opciones.respaldo = [&](QSqlDatabase& db, const QString& rutaResp,
                                QString* error) -> bool {
            ++llamadasRespaldo;
            orden << QStringLiteral("respaldo");
            rutaRespaldoRegistrada = rutaResp;
            return Respaldo::crearRespaldo(db, rutaResp, error);
        };
        opciones.paso = [&](QSqlDatabase& db, int destino) -> bool {
            orden << QStringLiteral("paso");
            if (destino != 2)
                return false;
            QSqlQuery query(db);
            return query.exec("CREATE TABLE DominioV2 (id INTEGER PRIMARY KEY)");
        };

        const Resultado resultado = AperturaBaseDatos::abrir(ruta, opciones);

        QCOMPARE(resultado.estado, Estado::OkMigrada);
        QVERIFY(resultado.ok());
        QCOMPARE(llamadasRespaldo, 1);
        // El respaldo se pide antes de ejecutar la migración.
        QCOMPARE(orden, QStringList({QStringLiteral("respaldo"), QStringLiteral("paso")}));
        QCOMPARE(resultado.rutaRespaldo, rutaRespaldoRegistrada);
        QCOMPARE(resultado.rutaRespaldo, Respaldo::construirNombreRespaldo(ruta, ahora));
        QVERIFY(QFile::exists(resultado.rutaRespaldo));
        QCOMPARE(leerVersion(ruta), 2);
        QVERIFY(tablaExiste(ruta, "DominioV2"));
    }

    void migracion_sinRespaldo_fallaSinMigrar()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("sin_respaldo.db");
        crearBaseVersion(ruta, 1);

        bool pasoLlamado = false;

        Opciones opciones;
        opciones.versionEsperada = 2;
        opciones.respaldo = [](QSqlDatabase&, const QString&, QString* error) -> bool {
            if (error)
                *error = QStringLiteral("no hay espacio para el respaldo");
            return false;
        };
        opciones.paso = [&](QSqlDatabase&, int) -> bool {
            pasoLlamado = true;
            return true;
        };

        const Resultado resultado = AperturaBaseDatos::abrir(ruta, opciones);

        QCOMPARE(resultado.estado, Estado::FalloRespaldo);
        QVERIFY(!resultado.ok());
        // Sin respaldo no se migra: ni el paso se ejecuta ni la versión cambia.
        QVERIFY(!pasoLlamado);
        QCOMPARE(leerVersion(ruta), 1);
        QVERIFY(!tablaExiste(ruta, "DominioV2"));
    }

    void migracion_fallida_dejaEstadoConocido()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("fallo_migracion.db");
        crearBaseVersion(ruta, 1);

        Opciones opciones;
        opciones.versionEsperada = 2;
        opciones.respaldo = [](QSqlDatabase& db, const QString& rutaResp,
                               QString* error) -> bool {
            return Respaldo::crearRespaldo(db, rutaResp, error);
        };
        opciones.paso = [](QSqlDatabase& db, int destino) -> bool {
            if (destino == 2)
            {
                QSqlQuery query(db);
                query.exec("CREATE TABLE TablaParcial (id INTEGER PRIMARY KEY)");
            }
            return false;
        };

        const Resultado resultado = AperturaBaseDatos::abrir(ruta, opciones);

        QCOMPARE(resultado.estado, Estado::FalloMigracion);
        QVERIFY(!resultado.ok());
        // Estado conocido: la versión previa queda intacta y no hay cambios parciales.
        QCOMPARE(leerVersion(ruta), 1);
        QVERIFY(!tablaExiste(ruta, "TablaParcial"));
        // Se conserva la ruta del respaldo creado antes de migrar.
        QVERIFY(!resultado.rutaRespaldo.isEmpty());
        QVERIFY(QFile::exists(resultado.rutaRespaldo));
    }

    // ─── Progreso de migración (RF-1) ───────────────────────────────────────

    void migracion_notificaProgresoPorPaso()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("progreso.db");
        crearBaseVersion(ruta, 1);

        QVector<int> pasos;
        Opciones opciones;
        opciones.versionEsperada = 2;
        opciones.respaldo = [](QSqlDatabase& db, const QString& rutaResp,
                               QString* error) -> bool {
            return Respaldo::crearRespaldo(db, rutaResp, error);
        };
        opciones.paso = [](QSqlDatabase& db, int destino) -> bool {
            QSqlQuery query(db);
            return query.exec(QStringLiteral("CREATE TABLE DominioV%1 (id INTEGER PRIMARY KEY)")
                                  .arg(destino));
        };
        opciones.progreso = [&pasos](int destino) { pasos.append(destino); };

        const Resultado resultado = AperturaBaseDatos::abrir(ruta, opciones);

        QCOMPARE(resultado.estado, Estado::OkMigrada);
        QVERIFY(resultado.ok());
        QCOMPARE(pasos, QVector<int>({2}));
    }

    void migracion_notificaCadaPaso()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("progreso_multipaso.db");
        crearBaseVersion(ruta, 1);

        QVector<int> pasos;
        Opciones opciones;
        opciones.versionEsperada = 3;
        opciones.respaldo = [](QSqlDatabase& db, const QString& rutaResp,
                               QString* error) -> bool {
            return Respaldo::crearRespaldo(db, rutaResp, error);
        };
        opciones.paso = [](QSqlDatabase& db, int destino) -> bool {
            QSqlQuery query(db);
            return query.exec(QStringLiteral("CREATE TABLE DominioV%1 (id INTEGER PRIMARY KEY)")
                                  .arg(destino));
        };
        opciones.progreso = [&pasos](int destino) { pasos.append(destino); };

        const Resultado resultado = AperturaBaseDatos::abrir(ruta, opciones);

        QCOMPARE(resultado.estado, Estado::OkMigrada);
        QVERIFY(resultado.ok());
        QCOMPARE(pasos, QVector<int>({2, 3}));
    }

    void sinMigracion_noNotificaProgreso()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("sin_migracion.db");
        crearBaseVersion(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        bool notificado = false;
        Opciones opciones;
        opciones.progreso = [&notificado](int) { notificado = true; };

        const Resultado resultado = AperturaBaseDatos::abrir(ruta, opciones);

        QCOMPARE(resultado.estado, Estado::OkAbierta);
        QVERIFY(!notificado);
    }

    // ─── Mensajes de error (RNF-1) ──────────────────────────────────────────

    void abrir_noAbrible_mensajeSinTextoCrudoDeQt()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        // Directorio contenedor inexistente: SQLite no puede crear el archivo.
        const QString ruta = dir.filePath("sub/no_existe/base.db");

        const Resultado resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloApertura);
        QVERIFY(!resultado.detalle.isEmpty());
        QVERIFY(!resultado.detalle.contains("unable to", Qt::CaseInsensitive));
        QVERIFY(!resultado.detalle.contains("no such", Qt::CaseInsensitive));
    }
};

QTEST_MAIN(TestAperturaBaseDatos)
#include "test_apertura_base_datos.moc"
