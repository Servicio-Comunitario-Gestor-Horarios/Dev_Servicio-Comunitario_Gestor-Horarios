/**
 * @file test_contrato_interfaz.cpp
 * @brief Fija el contrato que consume el frontend (RF-3, RF-4, RNF-4).
 *
 * No prueba la lógica por dentro: fija la semántica observable de lo que
 * exponemos y que la interfaz (otro equipo) da por supuesta según
 * `docs/interfaz-frontend.md`, sección «Base de datos local»:
 *   - `Resultado<T>::exito()` / `error()` (valor, mensaje y `codigoError`);
 *   - `AperturaBaseDatos::Resultado::ok()`, `detalle` y `rutaRespaldo`;
 *   - `estadoPendienteDe` para los tres estados por registro.
 *
 * Si este test cambia, hay que actualizar también la documentación viva del
 * contrato para el frontend.
 */

#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QSqlDatabase>
#include <QString>
#include <QTime>
#include <QVector>
#include <QTest>
#include <QTemporaryDir>

#include <backend/database/apertura_base_datos.hpp>
#include <backend/database/respaldo.hpp>
#include <backend/database/version_esquema.hpp>
#include <backend/resultado.hpp>
#include <backend/services/GestorPendientes.hpp>

using AperturaBaseDatos::Estado;
using AperturaBaseDatos::Opciones;
using ResultadoApertura = AperturaBaseDatos::Resultado;

class TestContratoInterfaz : public QObject
{
    Q_OBJECT

private:
    int m_contadorConexiones = 0;

    /// Abre una conexión de prueba con un nombre único (para preparar bases).
    QSqlDatabase abrirConexion(const QString& ruta)
    {
        const QString nombre =
            QStringLiteral("test_contrato_%1").arg(++m_contadorConexiones);
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombre);
        db.setDatabaseName(ruta);
        db.open();
        return db;
    }

    /// Cierra y elimina del registro de Qt la conexión dada.
    void cerrarConexion(QSqlDatabase& db)
    {
        const QString nombre = db.connectionName();

        if (db.isOpen())
            db.close();

        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(nombre);
    }

    /// Crea una base con el esquema v1 y fija la versión indicada.
    void crearBaseVersion(const QString& ruta, int version)
    {
        QSqlDatabase db = abrirConexion(ruta);
        VersionEsquema::aplicarHasta(db, 1);
        VersionEsquema::fijarVersionEsquema(db, version);
        cerrarConexion(db);
    }

private slots:

    // ─── Resultado<T> ──────────────────────────────────────────────────────

    void resultado_exito_exponeValorYNoMarcaError()
    {
        const Resultado<int> resultado = Resultado<int>::exito(42);

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor, 42);
        QVERIFY(resultado.mensajeError.isEmpty());
        QCOMPARE(resultado.codigoError, 0);
    }

    void resultado_error_exponeMensajeYCodigoGenericoPorDefecto()
    {
        const Resultado<int> resultado =
            Resultado<int>::error(QStringLiteral("no se pudo guardar el aula"));

        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.mensajeError, QStringLiteral("no se pudo guardar el aula"));
        // El frontend puede tratar cualquier código < 0 como error genérico.
        QCOMPARE(resultado.codigoError, -1);
    }

    void resultado_error_aceptaCodigoExplicito()
    {
        const Resultado<int> resultado =
            Resultado<int>::error(QStringLiteral("registro inexistente"), -2);

        QVERIFY(!resultado.ok);
        QCOMPARE(resultado.codigoError, -2);
        QCOMPARE(resultado.mensajeError, QStringLiteral("registro inexistente"));
    }

    // ─── AperturaBaseDatos::Resultado ──────────────────────────────────────

    void apertura_okSoloParaLosEstadosDeExito()
    {
        // Contrato: `ok()` es true solo con la base operativa.
        const QVector<Estado> exitos = {
            Estado::OkCreada, Estado::OkAbierta, Estado::OkMigrada};
        const QVector<Estado> fallos = {
            Estado::FalloAusente, Estado::FalloPosterior, Estado::FalloApertura,
            Estado::FalloRespaldo, Estado::FalloMigracion};

        for (const Estado estado : exitos) {
            ResultadoApertura resultado;
            resultado.estado = estado;
            QVERIFY2(resultado.ok(), "un estado de éxito debe devolver ok() == true");
        }

        for (const Estado estado : fallos) {
            ResultadoApertura resultado;
            resultado.estado = estado;
            QVERIFY2(!resultado.ok(), "un estado de fallo debe devolver ok() == false");
        }
    }

    void apertura_inexistente_okConDetalleYSinRespaldo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("nueva.db");

        const ResultadoApertura resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkCreada);
        QVERIFY(resultado.ok());
        QVERIFY(!resultado.detalle.isEmpty());
        // Crear no migra: nunca hay respaldo asociado.
        QVERIFY(resultado.rutaRespaldo.isEmpty());
    }

    void apertura_versionActual_okSinRespaldo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("actual.db");
        crearBaseVersion(ruta, VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        const ResultadoApertura resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::OkAbierta);
        QVERIFY(resultado.ok());
        QVERIFY(resultado.rutaRespaldo.isEmpty());
    }

    void apertura_migracion_exitoExponeRutaRespaldo()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("migrar.db");
        crearBaseVersion(ruta, 1);

        const QDateTime ahora(QDate(2026, 10, 9), QTime(12, 0, 0));

        Opciones opciones;
        opciones.ahora = ahora;

        const ResultadoApertura resultado = AperturaBaseDatos::abrir(ruta, opciones);

        QCOMPARE(resultado.estado, Estado::OkMigrada);
        QVERIFY(resultado.ok());
        // El frontend puede ofrecer restaurar el respaldo usando esta ruta.
        QVERIFY(!resultado.rutaRespaldo.isEmpty());
        QVERIFY(QFile::exists(resultado.rutaRespaldo));
        QCOMPARE(resultado.rutaRespaldo,
                 Respaldo::construirNombreRespaldo(ruta, ahora));
    }

    void apertura_versionAusente_falloConDetalle()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("ausente.db");
        crearBaseVersion(ruta, 0);

        const ResultadoApertura resultado = AperturaBaseDatos::abrir(ruta);

        QCOMPARE(resultado.estado, Estado::FalloAusente);
        QVERIFY(!resultado.ok());
        // El diálogo de fallo (RF-4) necesita un motivo legible en español.
        QVERIFY(!resultado.detalle.isEmpty());
    }

    // ─── Estado por registro ───────────────────────────────────────────────

    void estadoPendiente_registroGuardado_esGuardado()
    {
        const OperacionPendiente guardado;

        QCOMPARE(static_cast<int>(estadoPendienteDe(guardado)),
                 static_cast<int>(EstadoPendiente::Guardado));
    }

    void estadoPendiente_altaOModificacion_esPendienteGuardar()
    {
        OperacionPendiente alta;
        alta.pendiente = true;
        alta.tipo = TipoOperacion::Alta;
        QCOMPARE(static_cast<int>(estadoPendienteDe(alta)),
                 static_cast<int>(EstadoPendiente::PendienteGuardar));

        OperacionPendiente modificacion;
        modificacion.pendiente = true;
        modificacion.tipo = TipoOperacion::Modificacion;
        QCOMPARE(static_cast<int>(estadoPendienteDe(modificacion)),
                 static_cast<int>(EstadoPendiente::PendienteGuardar));
    }

    void estadoPendiente_baja_esPendienteEliminar()
    {
        OperacionPendiente baja;
        baja.pendiente = true;
        baja.tipo = TipoOperacion::Baja;
        QCOMPARE(static_cast<int>(estadoPendienteDe(baja)),
                 static_cast<int>(EstadoPendiente::PendienteEliminar));
    }
};

QTEST_MAIN(TestContratoInterfaz)
#include "test_contrato_interfaz.moc"
