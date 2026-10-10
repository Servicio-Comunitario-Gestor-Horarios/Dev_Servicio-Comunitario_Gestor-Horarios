#include <QDateTime>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <backend/database/respaldo.hpp>
#include <backend/database/version_esquema.hpp>

/// Tests de creación, validación y restauración de respaldos de la base (RF-1, RF-6).
class TestRespaldo : public QObject
{
    Q_OBJECT

private:
    int m_contadorConexiones = 0;

    /// Abre una conexión de prueba a una base en archivo con un nombre único.
    QSqlDatabase abrirBase(const QString& ruta)
    {
        const QString nombre = QStringLiteral("test_respaldo_%1").arg(++m_contadorConexiones);
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

    /// Crea una base con el esquema actual (versión fijada) y dos profesores de ejemplo.
    void crearBaseConDatos(const QString& ruta)
    {
        QSqlDatabase db = abrirBase(ruta);

        if (!VersionEsquema::aplicarHasta(db, VersionEsquema::VERSION_ESQUEMA_ACTUAL))
            return; // no se crea la base; el test lo detectará al consultar

        {
            QSqlQuery query(db);
            query.exec("INSERT INTO Profesores(id,nombre,email) "
                       "VALUES('P1','Ana','ana@test.com')");
            query.exec("INSERT INTO Profesores(id,nombre,email) "
                       "VALUES('P2','Luis','luis@test.com')");
        }

        cerrarBase(db);
    }

    /// Cuenta las filas de una tabla abriendo una conexión temporal.
    int contarFilas(const QString& ruta, const QString& tabla)
    {
        QSqlDatabase db = abrirBase(ruta);
        int total = -1;

        {
            QSqlQuery query(db);

            if (query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabla)) && query.next())
                total = query.value(0).toInt();
        }

        cerrarBase(db);
        return total;
    }

    /// Lee la versión de esquema almacenada en un archivo de base de datos.
    int leerVersion(const QString& ruta)
    {
        QSqlDatabase db = abrirBase(ruta);
        const int version = VersionEsquema::leerVersionEsquema(db);
        cerrarBase(db);
        return version;
    }

private slots:

    // ─── construirNombreRespaldo ────────────────────────────────────────────

    void construirNombreRespaldo_esDeterminista()
    {
        const QDateTime ahora(QDate(2026, 10, 8), QTime(14, 30, 5));
        const QString ruta = QStringLiteral("/datos/base.db");
        const QString esperado = QStringLiteral("/datos/base.db.respaldo-20261008-143005");

        QCOMPARE(Respaldo::construirNombreRespaldo(ruta, ahora), esperado);

        // Mismo instante → mismo nombre (función pura, sin reloj interno ni E/S).
        QCOMPARE(Respaldo::construirNombreRespaldo(ruta, ahora), esperado);

        // Instantes distintos → nombres distintos.
        const QDateTime otro(QDate(2026, 10, 8), QTime(14, 30, 6));
        QVERIFY(Respaldo::construirNombreRespaldo(ruta, otro) != esperado);
    }

    // ─── crearRespaldo / validarRespaldo ────────────────────────────────────

    void crearRespaldo_creaArchivoValido()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);

        const QString rutaRespaldo =
            Respaldo::construirNombreRespaldo(rutaDb, QDateTime(QDate(2026, 10, 8), QTime(10, 0, 0)));

        QSqlDatabase db = abrirBase(rutaDb);
        QString error;
        QVERIFY2(Respaldo::crearRespaldo(db, rutaRespaldo, &error), qPrintable(error));
        cerrarBase(db);

        QVERIFY(error.isEmpty());
        QVERIFY(QFile::exists(rutaRespaldo));
        QVERIFY2(Respaldo::validarRespaldo(rutaRespaldo, &error), qPrintable(error));
        QVERIFY(error.isEmpty());
    }

    void crearRespaldo_imposible_reportaMotivo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);

        QSqlDatabase db = abrirBase(rutaDb);
        // El directorio contenedor no existe: el respaldo es imposible.
        const QString rutaRespaldo = dir.filePath("no/existe/base.db.respaldo");

        QString error;
        QVERIFY(!Respaldo::crearRespaldo(db, rutaRespaldo, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFile::exists(rutaRespaldo));
        cerrarBase(db);
    }

    void validarRespaldo_archivoCorrupto_seRechaza()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("corrupto.db");

        {
            QFile archivo(ruta);
            QVERIFY(archivo.open(QIODevice::WriteOnly));
            archivo.write("esto no es una base de datos sqlite");
        }

        QString error;
        QVERIFY(!Respaldo::validarRespaldo(ruta, &error));
        QVERIFY(!error.isEmpty());
    }

    void validarRespaldo_versionPosterior_seRechaza()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);

        const QString rutaRespaldo = dir.filePath("posterior.db");

        {
            QSqlDatabase db = abrirBase(rutaDb);
            QVERIFY(VersionEsquema::fijarVersionEsquema(
                db, VersionEsquema::VERSION_ESQUEMA_ACTUAL + 1));

            QString error;
            QVERIFY2(Respaldo::crearRespaldo(db, rutaRespaldo, &error), qPrintable(error));
            cerrarBase(db);
        }

        QString error;
        QVERIFY(!Respaldo::validarRespaldo(rutaRespaldo, &error));
        QVERIFY(!error.isEmpty());
    }

    // ─── restaurarRespaldo ──────────────────────────────────────────────────

    void restaurarRespaldo_reproduceLaBase()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);
        QCOMPARE(contarFilas(rutaDb, "Profesores"), 2);

        const QString rutaRespaldo = dir.filePath("base.db.respaldo");
        {
            QSqlDatabase db = abrirBase(rutaDb);
            QString error;
            QVERIFY2(Respaldo::crearRespaldo(db, rutaRespaldo, &error), qPrintable(error));
            cerrarBase(db);
        }

        // Se pierde la base original.
        QVERIFY(QFile::remove(rutaDb));
        QVERIFY(!QFile::exists(rutaDb));

        QString error;
        QVERIFY2(Respaldo::restaurarRespaldo(rutaRespaldo, rutaDb, &error), qPrintable(error));
        QVERIFY(QFile::exists(rutaDb));

        // La base restaurada reproduce los datos del respaldo.
        QCOMPARE(contarFilas(rutaDb, "Profesores"), 2);
    }

    void restaurarRespaldo_origenCorrupto_noTocaDestino()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString rutaDestino = dir.filePath("destino.db");
        crearBaseConDatos(rutaDestino);

        const QString rutaOrigen = dir.filePath("corrupto.db");
        {
            QFile archivo(rutaOrigen);
            QVERIFY(archivo.open(QIODevice::WriteOnly));
            archivo.write("basura que no es sqlite");
        }

        QString error;
        QVERIFY(!Respaldo::restaurarRespaldo(rutaOrigen, rutaDestino, &error));
        QVERIFY(!error.isEmpty());

        // El destino conserva sus datos intactos.
        QCOMPARE(contarFilas(rutaDestino, "Profesores"), 2);
    }

    // ─── descartarBaseYCrearNueva (RF-4, RF-6) ─────────────────────────────

    void descartarBaseYCrearNueva_respaldaYcreaNueva()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);
        QCOMPARE(contarFilas(rutaDb, "Profesores"), 2);

        const QDateTime ahora(QDate(2026, 10, 10), QTime(9, 15, 0));
        Respaldo::OpcionesDescarte opciones;
        opciones.ahora = ahora;

        const Respaldo::ResultadoDescarte resultado =
            Respaldo::descartarBaseYCrearNueva(rutaDb, opciones);

        QVERIFY(resultado.ok());
        QCOMPARE(resultado.estado, Respaldo::EstadoDescarte::Ok);
        QCOMPARE(resultado.rutaRespaldo, Respaldo::construirNombreRespaldo(rutaDb, ahora));
        QVERIFY(QFile::exists(resultado.rutaRespaldo));

        // El respaldo conserva los datos anteriores.
        QString error;
        QVERIFY2(Respaldo::validarRespaldo(resultado.rutaRespaldo, &error), qPrintable(error));
        QCOMPARE(contarFilas(resultado.rutaRespaldo, "Profesores"), 2);

        // La base nueva tiene el esquema actual y está vacía.
        QVERIFY(QFile::exists(rutaDb));
        QCOMPARE(leerVersion(rutaDb), VersionEsquema::VERSION_ESQUEMA_ACTUAL);
        QCOMPARE(contarFilas(rutaDb, "Profesores"), 0);
    }

    void descartarBaseYCrearNueva_respaldoImposible_noDescarta()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);

        Respaldo::OpcionesDescarte opciones;
        opciones.respaldo = [](QSqlDatabase&, const QString&, QString* error) -> bool {
            if (error)
                *error = QStringLiteral("no hay espacio en disco");
            return false;
        };

        const Respaldo::ResultadoDescarte resultado =
            Respaldo::descartarBaseYCrearNueva(rutaDb, opciones);

        QCOMPARE(resultado.estado, Respaldo::EstadoDescarte::FalloRespaldo);
        QVERIFY(!resultado.ok());
        QVERIFY(resultado.rutaRespaldo.isEmpty());
        QVERIFY(!resultado.detalle.isEmpty());
        // La base anterior queda intacta: no se descartó sin respaldo.
        QVERIFY(QFile::exists(rutaDb));
        QCOMPARE(contarFilas(rutaDb, "Profesores"), 2);
    }

    void descartarBaseYCrearNueva_creacionFalla_conservaRespaldo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);

        const QDateTime ahora(QDate(2026, 10, 10), QTime(9, 30, 0));
        Respaldo::OpcionesDescarte opciones;
        opciones.ahora = ahora;
        opciones.crearNueva = [](const QString&, QString* error) -> bool {
            if (error)
                *error = QStringLiteral("sin permisos de escritura");
            return false;
        };

        const Respaldo::ResultadoDescarte resultado =
            Respaldo::descartarBaseYCrearNueva(rutaDb, opciones);

        QCOMPARE(resultado.estado, Respaldo::EstadoDescarte::FalloCreacion);
        QVERIFY(!resultado.ok());
        // Se descartó la base anterior...
        QVERIFY(!QFile::exists(rutaDb));
        // ...pero el respaldo sigue disponible para restaurar.
        QCOMPARE(resultado.rutaRespaldo, Respaldo::construirNombreRespaldo(rutaDb, ahora));
        QVERIFY(QFile::exists(resultado.rutaRespaldo));
        QString error;
        QVERIFY2(Respaldo::validarRespaldo(resultado.rutaRespaldo, &error), qPrintable(error));
    }

    void descartarBaseYCrearNueva_sinBase_creaNueva()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath("nueva.db");
        QVERIFY(!QFile::exists(rutaDb));

        const Respaldo::ResultadoDescarte resultado =
            Respaldo::descartarBaseYCrearNueva(rutaDb);

        QVERIFY(resultado.ok());
        QVERIFY(resultado.rutaRespaldo.isEmpty());
        QVERIFY(QFile::exists(rutaDb));
        QCOMPARE(leerVersion(rutaDb), VersionEsquema::VERSION_ESQUEMA_ACTUAL);
    }

    // ─── Mensajes de error (RNF-1) ──────────────────────────────────────────

    void respaldo_mensajeErrorSinTextoCrudoDeQt()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString rutaDb = dir.filePath("base.db");
        crearBaseConDatos(rutaDb);

        QSqlDatabase db = abrirBase(rutaDb);
        // Directorio contenedor inexistente: el respaldo falla.
        const QString rutaRespaldo = dir.filePath("no/existe/respaldo.db");
        QString error;
        QVERIFY(!Respaldo::crearRespaldo(db, rutaRespaldo, &error));
        cerrarBase(db);

        // RNF-1: mensaje en español, sin el texto crudo de Qt en inglés.
        QVERIFY(!error.isEmpty());
        QVERIFY(!error.contains("unable to", Qt::CaseInsensitive));
        QVERIFY(!error.contains("no such", Qt::CaseInsensitive));
    }
};

QTEST_MAIN(TestRespaldo)
#include "test_respaldo.moc"
