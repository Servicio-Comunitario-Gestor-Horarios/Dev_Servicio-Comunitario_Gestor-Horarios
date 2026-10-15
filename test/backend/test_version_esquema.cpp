#include <QTest>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include <backend/database/version_esquema.hpp>

using VersionEsquema::EstadoApertura;

namespace {

bool tablaExiste(QSqlDatabase& db, const QString& tabla)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT name FROM sqlite_master "
                                 "WHERE type='table' AND name=:nombre"));
    query.bindValue(QStringLiteral(":nombre"), tabla);
    return query.exec() && query.next();
}

} // namespace

/// Tests de la decisión de apertura y de la creación del esquema único (RF-6).
class TestVersionEsquema : public QObject
{
    Q_OBJECT

private:
    QString m_nombreConexion;

    QSqlDatabase abrirBaseEnMemoria()
    {
        m_nombreConexion = QStringLiteral("test_version_esquema");
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_nombreConexion);
        db.setDatabaseName(QStringLiteral(":memory:"));
        db.open();
        return db;
    }

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
        QCOMPARE(VersionEsquema::decidirApertura(false, 0, 2), EstadoApertura::Crear);
    }

    void decidirApertura_versionIgual_debeAbrir()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 2, 2), EstadoApertura::Abrir);
    }

    void decidirApertura_versionAusente_debeFallarEsquema()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 0, 2), EstadoApertura::FalloEsquema);
    }

    void decidirApertura_versionAnterior_debeFallarEsquema()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 1, 2), EstadoApertura::FalloEsquema);
    }

    void decidirApertura_versionPosterior_debeFallarEsquema()
    {
        QCOMPARE(VersionEsquema::decidirApertura(true, 3, 2), EstadoApertura::FalloEsquema);
    }

    // ─── crearEsquema ───────────────────────────────────────────────────────

    void crearEsquema_creaCatalogoYFijaVersion()
    {
        QSqlDatabase db = abrirBaseEnMemoria();

        QCOMPARE(VersionEsquema::leerVersionEsquema(db), 0);

        QVERIFY(VersionEsquema::crearEsquema(db));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), VersionEsquema::VERSION_ESQUEMA_ACTUAL);
        QVERIFY(tablaExiste(db, QStringLiteral("Aulas")));
        QVERIFY(tablaExiste(db, QStringLiteral("Profesores")));
        QVERIFY(tablaExiste(db, QStringLiteral("Cursos")));
        QVERIFY(!tablaExiste(db, QStringLiteral("Curso_Materia")));

        // Idempotente: repetir no falla ni cambia la versión.
        QVERIFY(VersionEsquema::crearEsquema(db));
        QCOMPARE(VersionEsquema::leerVersionEsquema(db), VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        cerrarBase(db);
    }

    // ─── esquemaEsperado ────────────────────────────────────────────────────

    void esquemaEsperado_aceptaEsquemaCorrecto()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(VersionEsquema::esquemaEsperado(db));
        cerrarBase(db);
    }

    void esquemaEsperado_rechazaTablaRetirada()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Curso_Materia (id INTEGER)")));

        QVERIFY(!VersionEsquema::esquemaEsperado(db));
        cerrarBase(db);
    }

    void esquemaEsperado_rechazaCursosSinColumnaAula()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("DROP TABLE Cursos")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Cursos ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "anio INTEGER NOT NULL,"
                                      "seccion TEXT NOT NULL,"
                                      "turno TEXT,"
                                      "fecha_creacion TEXT,"
                                      "fecha_modificacion TEXT,"
                                      "UNIQUE(anio, seccion))")));

        QVERIFY(!VersionEsquema::esquemaEsperado(db)); // falta la columna `aula`
        cerrarBase(db);
    }
};

QTEST_MAIN(TestVersionEsquema)
#include "test_version_esquema.moc"
