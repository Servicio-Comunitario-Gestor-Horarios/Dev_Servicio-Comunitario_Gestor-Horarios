#include <QCryptographicHash>
#include <QRegularExpression>
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

/// Huella SHA-256 del catálogo real de la base: tablas y su DDL tal y como los
/// registró SQLite, en orden estable y con el espacio normalizado. Cualquier
/// cambio de esquema (tabla, columna, restricción) cambia la huella.
QByteArray huellaEsquema(QSqlDatabase& db)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT name, sql FROM sqlite_master "
                                  "WHERE type='table' AND name NOT LIKE 'sqlite_%' "
                                  "ORDER BY name")))
        return QByteArray();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (query.next())
    {
        QString sql = query.value(1).toString();
        sql.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
        hash.addData((query.value(0).toString() + QLatin1Char('|') + sql.trimmed()
                      + QLatin1Char('\n')).toUtf8());
    }
    return hash.result().toHex();
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

    void esquemaEsperado_explicaElMotivo()
    {
        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Curso_Materia (id INTEGER)")));

        QString motivo;
        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("Curso_Materia"))); // tabla retirada presente

        QVERIFY(q.exec(QStringLiteral("DROP TABLE Curso_Materia")));
        QVERIFY(q.exec(QStringLiteral("DROP TABLE Cursos")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE Cursos (id INTEGER PRIMARY KEY)")));

        QVERIFY(!VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.contains(QStringLiteral("anio")));   // columna que falta
        QVERIFY(motivo.contains(QStringLiteral("aula")));   // columna que falta

        QVERIFY(q.exec(QStringLiteral("DROP TABLE Cursos")));
        QVERIFY(VersionEsquema::crearEsquema(db));
        QVERIFY(VersionEsquema::esquemaEsperado(db, &motivo));
        QVERIFY(motivo.isEmpty()); // catálogo correcto: sin motivo

        cerrarBase(db);
    }

    // ─── huella del esquema ────────────────────────────────────────────────

    void huellaEsquema_noCambiaSinSubirLaMarcaDeVersion()
    {
        // Guardia anti-regresión del incidente real de 2026-10-17: el DDL de
        // `Cursos` cambió (nombre/aula_fija → anio/seccion/turno/aula) sin subir
        // VERSION_ESQUEMA_ACTUAL, y las bases existentes con la marca antigua
        // quedaron indistinguibles por número de versión.
        //
        // SI ESTE TEST FALLA AL CAMBIAR EL ESQUEMA, ACTÚA ASÍ:
        //   1. Sube VERSION_ESQUEMA_ACTUAL en `backend/database/version_esquema.hpp`.
        //   2. Registra la huella nueva en HUELLAS_CONOCIDAS más abajo.
        //   3. Actualiza TABLAS_ESPERADAS/COLUMNAS_CURSOS de `version_esquema.cpp`.
        // Cambiar el DDL sin subir la versión vuelve a romper las bases de los
        // usuarios: la marca ya no las distingue y solo el catálogo las rechaza.
        static const struct { int version; const char* huella; } HUELLAS_CONOCIDAS[] = {
            { 2, "b7684333c36458b0529308f9130e2a3b105c4cd4086b5f0e470a26ca917eb882" },
        };

        QSqlDatabase db = abrirBaseEnMemoria();
        QVERIFY(VersionEsquema::crearEsquema(db));

        const QByteArray huella = huellaEsquema(db);
        QVERIFY(!huella.isEmpty());

        const char* registrada = nullptr;
        for (const auto& entrada : HUELLAS_CONOCIDAS)
            if (entrada.version == VersionEsquema::VERSION_ESQUEMA_ACTUAL)
                registrada = entrada.huella;

        QVERIFY2(registrada,
                 "No hay huella registrada para VERSION_ESQUEMA_ACTUAL en "
                 "HUELLAS_CONOCIDAS (test_version_esquema.cpp): añade la huella del "
                 "esquema recién creado.");

        QCOMPARE(QString::fromLatin1(huella), QString::fromLatin1(registrada));

        // La marca y el catálogo deben viajar juntos: una base recién creada es
        // válida para la apertura con el mismo esquema congelado aquí.
        QVERIFY(VersionEsquema::esquemaEsperado(db));

        cerrarBase(db);
    }
};

QTEST_MAIN(TestVersionEsquema)
#include "test_version_esquema.moc"
