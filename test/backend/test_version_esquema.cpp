#include <QTest>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include <backend/database/version_esquema.hpp>

using VersionEsquema::EstadoApertura;

namespace {

/// Devuelve true si `tabla` existe en el esquema de la conexión dada.
bool tablaExiste(QSqlDatabase& db, const QString& tabla)
{
    QSqlQuery query(db);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:nombre");
    query.bindValue(":nombre", tabla);

    if (!query.exec())
        return false;

    return query.next();
}

} // namespace

/// Tests de la decisión de apertura y del mecanismo de migraciones versionadas (RF-1).
class TestVersionEsquema : public QObject
{
    Q_OBJECT

private:
    QString m_nombreConexion;

    /// Abre una conexión SQLite en memoria con un nombre único por test.
    QSqlDatabase abrirBaseEnMemoria()
    {
        m_nombreConexion = QStringLiteral("test_version_esquema");
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_nombreConexion);
        db.setDatabaseName(":memory:");
        db.open();
        return db;
    }

    /// Cierra y elimina la conexión en memoria de este test.
    void cerrarBase(QSqlDatabase& db)
    {
        if (db.isOpen())
            db.close();

        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_nombreConexion);
    }

private slots:

    // ─── decidirApertura ────────────────────────────────────────────────────

    void decidirApertura_inexistente_debeCrear()
    {
        QCOMPARE(VersionEsquema::decidirApertura(false, 0, 1), EstadoApertura::Crear);
    }

    void decidirApertura_versionAusente_debeFallar()
    {
        // Versión almacenada ausente/no interpretable (SQLite devuelve 0).
        QCOMPARE(VersionEsquema::decidirApertura(true, 0, 1), EstadoApertura::FalloAusente);
    }

    void decidirApertura_versionAnterior_debeMigrar()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 1, 3), EstadoApertura::Migrar);
    }

    void decidirApertura_versionIgual_debeAbrir()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 3, 3), EstadoApertura::Abrir);
    }

    void decidirApertura_versionPosterior_debeFallar()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 4, 3), EstadoApertura::FalloPosterior);
    }

    // ─── aplicarHasta ───────────────────────────────────────────────────────

    void aplicarHasta_esquemaReal_fijaVersionActual()
    {
        QSqlDatabase db = abrirBaseEnMemoria();

        QCOMPARE(VersionEsquema::leerVersionEsquema(db), 0);

        QVERIFY(VersionEsquema::aplicarHasta(db, VersionEsquema::VERSION_ESQUEMA_ACTUAL));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        QVERIFY(tablaExiste(db, "Aulas"));
        QVERIFY(tablaExiste(db, "Profesores"));

        // Repetir la migración ya aplicada no vuelve a actuar (idempotente).
        QVERIFY(VersionEsquema::aplicarHasta(db, VersionEsquema::VERSION_ESQUEMA_ACTUAL));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        cerrarBase(db);
    }

    void aplicarHasta_v1aV2_pasoExitoso_fijaVersionDestino()
    {
        QSqlDatabase db = abrirBaseEnMemoria();

        const auto paso = [](QSqlDatabase& d, int destino) -> bool {
            QSqlQuery query(d);
            if (destino == 1)
                return query.exec("CREATE TABLE Base (id INTEGER PRIMARY KEY)");
            if (destino == 2)
                return query.exec("CREATE TABLE Extra (id INTEGER PRIMARY KEY)");
            return false;
        };

        QVERIFY(VersionEsquema::aplicarHasta(db, 2, paso));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), 2);
        QVERIFY(tablaExiste(db, "Base"));
        QVERIFY(tablaExiste(db, "Extra"));

        cerrarBase(db);
    }

    void aplicarHasta_v1aV2_falloSimulado_dejaVersionPreviaIntacta()
    {
        QSqlDatabase db = abrirBaseEnMemoria();

        // Primero aplicamos la versión 1 real.
        QVERIFY(VersionEsquema::aplicarHasta(db, 1));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), 1);

        // La migración a v2 crea una tabla y después falla: debe revertirse por completo.
        const auto pasoFallido = [](QSqlDatabase& d, int destino) -> bool {
            if (destino == 2)
            {
                QSqlQuery query(d);
                query.exec("CREATE TABLE TablaParcial (id INTEGER PRIMARY KEY)");
            }
            return false;
        };

        QVERIFY(!VersionEsquema::aplicarHasta(db, 2, pasoFallido));

        // La versión previa queda intacta y no hay cambios parciales.
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), 1);
        QVERIFY(!tablaExiste(db, "TablaParcial"));

        cerrarBase(db);
    }
};

QTEST_MAIN(TestVersionEsquema)
#include "test_version_esquema.moc"
