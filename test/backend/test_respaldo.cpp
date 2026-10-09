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
};

QTEST_MAIN(TestRespaldo)
#include "test_respaldo.moc"
