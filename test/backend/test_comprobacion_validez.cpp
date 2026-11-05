#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#include <memory>

#include <backend/database/apertura_base_datos.hpp>
#include <backend/database/comprobacion_validez.hpp>
#include <backend/database/version_esquema.hpp>

/// Pruebas de la comprobación única de validez (016 RF-1): reutiliza la guardia de la 015,
/// decide la causa sin duplicar la comprobación y no escribe. Sin reloj y sin datos de dominio.
namespace {

/// `QSqlDatabase::open()` necesita una aplicación Qt viva para cargar el driver SQLite.
class EntornoQt : public ::testing::Environment
{
public:
    void SetUp() override
    {
        static int argc = 1;
        static char nombre[] = "test_comprobacion_validez";
        static char* argv[] = {nombre, nullptr};
        m_app = std::make_unique<QCoreApplication>(argc, argv);
    }

    void TearDown() override { m_app.reset(); }

private:
    std::unique_ptr<QCoreApplication> m_app;
};

::testing::Environment* const entornoQt =
    ::testing::AddGlobalTestEnvironment(new EntornoQt());

int& contadorConexiones()
{
    static int contador = 0;
    return contador;
}

QString nombreConexion(const QString& etiqueta)
{
    return QStringLiteral("test_comprobacion_%1_%2")
        .arg(etiqueta)
        .arg(++contadorConexiones());
}

/// Conexión temporal con nombre único; se libera al destruirse.
class ConexionTemporal
{
public:
    ConexionTemporal(const QString& ruta, const QString& etiqueta)
        : m_nombre(nombreConexion(etiqueta))
    {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_nombre);
        m_db.setDatabaseName(ruta);
        m_db.open();
    }

    ~ConexionTemporal()
    {
        if (m_db.isOpen())
            m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_nombre);
    }

    QSqlDatabase& db() { return m_db; }

private:
    QString m_nombre;
    QSqlDatabase m_db;
};

/// Catálogo de objetos tal y como lo registra SQLite (tipo, nombre y SQL), en orden estable.
QStringList instantaneaCatalogo(QSqlDatabase& db)
{
    QStringList filas;
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT type, name, COALESCE(sql, '') FROM sqlite_master "
                                   "ORDER BY type, name")))
        return {QStringLiteral("__ERROR__")};

    while (query.next())
        filas << QStringLiteral("%1|%2|%3").arg(query.value(0).toString(),
                                                query.value(1).toString(),
                                                query.value(2).toString());
    return filas;
}

/// P10 de la 015: quita los dos disparadores para poder mutar la fila del general.
bool quitarProtecciones(QSqlDatabase& db)
{
    QSqlQuery query(db);
    return query.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar"))
           && query.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_renombrar"));
}

bool crearEsquemaValido(QSqlDatabase& db)
{
    return VersionEsquema::crearEsquema(db);
}

} // namespace

// ─── comprobar ──────────────────────────────────────────────────────────────

TEST(TestComprobacionValidez, baseValida_esValida)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("valida"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());

    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Valida);
    EXPECT_TRUE(comprobacion.valida());
    EXPECT_TRUE(comprobacion.mensaje.isEmpty());
}

TEST(TestComprobacionValidez, sinTipoGeneral_esEsquemaYNombraLaFila)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("sin_fila"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));
    ASSERT_TRUE(quitarProtecciones(conexion.db()));

    QSqlQuery borrado(conexion.db());
    ASSERT_TRUE(borrado.exec(QStringLiteral("DELETE FROM TiposAula")));

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());

    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Esquema);
    EXPECT_FALSE(comprobacion.valida());
    EXPECT_NE(comprobacion.mensaje.indexOf(QStringLiteral("aula de estudio")), -1)
        << comprobacion.mensaje.toStdString();
    EXPECT_NE(comprobacion.mensaje.indexOf(QStringLiteral("esquema esperado")), -1)
        << comprobacion.mensaje.toStdString();
}

TEST(TestComprobacionValidez, sinIndice_esEsquemaYNombraElObjeto)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("sin_indice"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    QSqlQuery indice(conexion.db());
    ASSERT_TRUE(indice.exec(QStringLiteral("DROP INDEX TiposAula_clave")));

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());

    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Esquema);
    EXPECT_NE(comprobacion.mensaje.indexOf(QStringLiteral("TiposAula_clave")), -1)
        << comprobacion.mensaje.toStdString();
}

TEST(TestComprobacionValidez, sinDisparadorDeBorrado_esEsquemaYNombraElObjeto)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("sin_borrado"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    QSqlQuery disparador(conexion.db());
    ASSERT_TRUE(disparador.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar")));

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());

    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Esquema);
    EXPECT_NE(comprobacion.mensaje.indexOf(
                  QStringLiteral("TiposAula_general_no_borrar")), -1)
        << comprobacion.mensaje.toStdString();
}

TEST(TestComprobacionValidez, sinDisparadorDeRenombrado_esEsquemaYNombraElObjeto)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("sin_renombrado"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    QSqlQuery disparador(conexion.db());
    ASSERT_TRUE(disparador.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_renombrar")));

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());

    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Esquema);
    EXPECT_NE(comprobacion.mensaje.indexOf(
                  QStringLiteral("TiposAula_general_no_renombrar")), -1)
        << comprobacion.mensaje.toStdString();
}

TEST(TestComprobacionValidez, tablaRetirada_esEsquema)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("retirada"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    QSqlQuery retirada(conexion.db());
    ASSERT_TRUE(retirada.exec(QStringLiteral("CREATE TABLE Curso_Materia (id INTEGER)")));

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());

    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Esquema);
    EXPECT_NE(comprobacion.mensaje.indexOf(QStringLiteral("Curso_Materia")), -1)
        << comprobacion.mensaje.toStdString();
}

TEST(TestComprobacionValidez, versionAnterior_esVersionConAcciones)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("version"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));
    ASSERT_TRUE(VersionEsquema::fijarVersionEsquema(conexion.db(), 5));

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());

    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Version);
    EXPECT_FALSE(comprobacion.valida());
    EXPECT_TRUE(comprobacion.mensaje.contains(QStringLiteral("crear una base nueva"),
                                              Qt::CaseInsensitive));
    EXPECT_NE(comprobacion.mensaje.indexOf(
                  QStringLiteral("restaurar un respaldo de la versión anterior volverá a "
                                 "fallar")), -1)
        << comprobacion.mensaje.toStdString();
    EXPECT_EQ(comprobacion.mensaje, validez_base::mensajeDeVersion(5, 6));
}

// ─── solo lectura y transacción ─────────────────────────────────────────────

TEST(TestComprobacionValidez, noModificaElCatalogo)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("sin_escritura"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    const QStringList antes = instantaneaCatalogo(conexion.db());
    const int versionAntes = VersionEsquema::leerVersionEsquema(conexion.db());

    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());
    EXPECT_TRUE(comprobacion.valida());

    EXPECT_EQ(instantaneaCatalogo(conexion.db()), antes);
    EXPECT_EQ(VersionEsquema::leerVersionEsquema(conexion.db()), versionAntes);
}

TEST(TestComprobacionValidez, noDejaTransaccionAbierta)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("transaccion_cierra"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    EXPECT_TRUE(validez_base::comprobar(conexion.db()).valida());

    // Si la comprobación hubiera dejado su transacción abierta, esta fallaría.
    EXPECT_TRUE(conexion.db().transaction());
    EXPECT_TRUE(conexion.db().commit());
}

TEST(TestComprobacionValidez, funcionaDentroDeUnaTransaccion)
{
    ConexionTemporal conexion(QStringLiteral(":memory:"), QStringLiteral("transaccion_usa"));
    ASSERT_TRUE(conexion.db().isOpen());
    ASSERT_TRUE(crearEsquemaValido(conexion.db()));

    // P5: con una transacción abierta, la comprobación usa esa instantánea y no la cierra.
    ASSERT_TRUE(conexion.db().transaction());
    EXPECT_TRUE(validez_base::comprobar(conexion.db()).valida());
    EXPECT_TRUE(conexion.db().commit());
}

// ─── coincidencia con la apertura ──────────────────────────────────────────

TEST(TestComprobacionValidez, aperturaYComprobacionCoinciden)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString ruta = dir.filePath(QStringLiteral("base.db"));

    // Base buena: la apertura abre y la comprobación es válida.
    const AperturaBaseDatos::Resultado creada = AperturaBaseDatos::abrir(ruta);
    EXPECT_EQ(creada.estado, AperturaBaseDatos::Estado::OkCreada);
    {
        ConexionTemporal conexion(ruta, QStringLiteral("coincide_buena"));
        ASSERT_TRUE(conexion.db().isOpen());
        EXPECT_TRUE(validez_base::comprobar(conexion.db()).valida());
    }

    // Mismo defecto (falta el disparador de borrado) en las dos vías.
    {
        ConexionTemporal conexion(ruta, QStringLiteral("coincide_rota"));
        ASSERT_TRUE(conexion.db().isOpen());
        QSqlQuery disparador(conexion.db());
        ASSERT_TRUE(disparador.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar")));
    }

    const AperturaBaseDatos::Resultado abierta = AperturaBaseDatos::abrir(ruta);
    EXPECT_EQ(abierta.estado, AperturaBaseDatos::Estado::FalloEsquema);
    EXPECT_FALSE(abierta.ok());

    ConexionTemporal conexion(ruta, QStringLiteral("coincide_rota_check"));
    ASSERT_TRUE(conexion.db().isOpen());
    const validez_base::Comprobacion comprobacion = validez_base::comprobar(conexion.db());
    EXPECT_EQ(comprobacion.causa, validez_base::Causa::Esquema);
    EXPECT_FALSE(comprobacion.valida());
}

// ─── mensajes puros ─────────────────────────────────────────────────────────

TEST(TestComprobacionValidez, mensajeDeVersion_incluyeVersionesYCausaYAccion)
{
    const QString mensaje = validez_base::mensajeDeVersion(5, 6);

    EXPECT_NE(mensaje.indexOf(QStringLiteral("5")), -1) << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("6")), -1) << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("No se migra ni se repara")), -1)
        << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("crear una base nueva")), -1)
        << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("volverá a fallar")), -1)
        << mensaje.toStdString();
}

TEST(TestComprobacionValidez, mensajeDeEsquema_incluyeMotivoYAccion)
{
    const QString mensaje = validez_base::mensajeDeEsquema(
        6, QStringLiteral("faltan los objetos «índice TiposAula_clave»"));

    EXPECT_NE(mensaje.indexOf(QStringLiteral("esquema esperado")), -1)
        << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("TiposAula_clave")), -1)
        << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("respaldo de la versión vigente")), -1)
        << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("base nueva")), -1) << mensaje.toStdString();
}
