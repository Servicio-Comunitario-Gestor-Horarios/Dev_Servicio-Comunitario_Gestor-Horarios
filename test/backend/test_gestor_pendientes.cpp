#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTest>
#include <QTemporaryDir>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/GestorPendientes.hpp>
#include <backend/services/NucleoDatos.hpp>

/// Tests de cambios pendientes y reintento (RF-3, RNF-3, RNF-4).
///
/// Comprueba la semántica del estado por registro (guardado / pendiente de
/// guardar / pendiente de eliminar), que un fallo de escritura queda pendiente y
/// no se descarta («sin perder de pantalla»), y que el reintento lo persiste.
class TestGestorPendientes : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<NucleoDatos> m_nucleo;
    QString m_ruta;

    /// Abre una conexión a la base de prueba y construye el núcleo de datos.
    void abrirBase()
    {
        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_ruta));
        QVERIFY(m_dbManager->isInitialized());
        m_nucleo.reset(new NucleoDatos(m_dbManager->database()));
    }

    /// Cierra la conexión para liberar la base de prueba.
    void cerrarBase()
    {
        m_nucleo.reset();
        m_dbManager->close();
        m_dbManager.reset();
    }

    /// Ejecuta una sentencia SQL suelta (para simular fallos de escritura).
    bool ejecutar(const QString& sql)
    {
        QSqlQuery query(m_dbManager->database());
        if (!query.exec(sql)) {
            qWarning() << "SQL de prueba falló:" << query.lastError().text();
            return false;
        }
        return true;
    }

private slots:

    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());
        m_ruta = m_dir->filePath("pendientes.db");
        abrirBase();
    }

    void cleanup()
    {
        cerrarBase();
        m_dir.reset();
    }

    // ─── Función pura `estadoPendienteDe` ───────────────────────────────────

    void estadoPendienteDe_registroGuardado()
    {
        // Una operación que no quedó pendiente representa un registro guardado.
        const OperacionPendiente guardado;
        QCOMPARE(static_cast<int>(estadoPendienteDe(guardado)),
                 static_cast<int>(EstadoPendiente::Guardado));
    }

    void estadoPendienteDe_altaYModificacion_sonPendienteGuardar()
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

    void estadoPendienteDe_baja_esPendienteEliminar()
    {
        OperacionPendiente baja;
        baja.pendiente = true;
        baja.tipo = TipoOperacion::Baja;
        QCOMPARE(static_cast<int>(estadoPendienteDe(baja)),
                 static_cast<int>(EstadoPendiente::PendienteEliminar));
    }

    // ─── GestorPendientes ────────────────────────────────────────────────────

    void gestor_sinPendientes_noHayCambios()
    {
        GestorPendientes gestor;
        QVERIFY(!gestor.hayPendientes());
        QCOMPARE(gestor.pendientes().size(), 0);
        QCOMPARE(static_cast<int>(gestor.estadoDe(QStringLiteral("aula"),
                                                  QStringLiteral("Aula 101"))),
                 static_cast<int>(EstadoPendiente::Guardado));
    }

    void gestor_registraPendienteYExponeEstado()
    {
        GestorPendientes gestor;

        OperacionPendiente operacion;
        operacion.dominio = QStringLiteral("aula");
        operacion.registroId = QStringLiteral("Aula 101");
        operacion.tipo = TipoOperacion::Alta;
        operacion.descripcion = QStringLiteral("Pendiente de guardar: Aula 101");

        const qint64 id = gestor.registrar(operacion);
        QVERIFY(id > 0);
        QVERIFY(gestor.hayPendientes());
        QCOMPARE(gestor.pendientes().size(), 1);
        QCOMPARE(gestor.pendientes().first().id, id);
        QVERIFY(gestor.pendientes().first().pendiente);

        QCOMPARE(static_cast<int>(gestor.estadoDe(QStringLiteral("aula"),
                                                  QStringLiteral("Aula 101"))),
                 static_cast<int>(EstadoPendiente::PendienteGuardar));
    }

    void gestor_reintentoConExito_quitaElPendiente()
    {
        GestorPendientes gestor;
        bool seIntento = false;

        OperacionPendiente operacion;
        operacion.dominio = QStringLiteral("aula");
        operacion.registroId = QStringLiteral("Aula 101");
        operacion.tipo = TipoOperacion::Alta;
        operacion.accion = [&seIntento]() {
            seIntento = true;
            return true;
        };
        const qint64 id = gestor.registrar(operacion);

        const auto resultado = gestor.reintentar(id);
        QVERIFY(resultado.ok);
        QVERIFY(seIntento);
        QVERIFY(!gestor.hayPendientes());
        QCOMPARE(gestor.pendientes().size(), 0);
    }

    void gestor_reintentoSinExito_conservaElPendiente()
    {
        GestorPendientes gestor;

        OperacionPendiente operacion;
        operacion.dominio = QStringLiteral("aula");
        operacion.registroId = QStringLiteral("Aula 101");
        operacion.tipo = TipoOperacion::Alta;
        operacion.accion = []() { return false; };
        const qint64 id = gestor.registrar(operacion);

        const auto resultado = gestor.reintentar(id);
        QVERIFY(!resultado.ok);
        // «Sin perder de pantalla»: el pendiente sigue disponible para reintento.
        QVERIFY(gestor.hayPendientes());
        QCOMPARE(gestor.pendientes().size(), 1);
        QCOMPARE(gestor.pendientes().first().id, id);
    }

    void gestor_reintentoIdDesconocido_falla()
    {
        GestorPendientes gestor;
        QVERIFY(!gestor.reintentar(99).ok);
    }

    // ─── Integración con NucleoDatos ────────────────────────────────────────

    void nucleo_falloDeGuardado_registraPendienteYReintentoGuarda()
    {
        QVERIFY(ejecutar(QStringLiteral(
            "CREATE TRIGGER fallo_insert_aulas BEFORE INSERT ON Aulas "
            "BEGIN SELECT RAISE(ABORT, 'fallo simulado de escritura'); END;")));

        const auto creado = m_nucleo->crearAula(QStringLiteral("Aula 101"), 30,
                                                QStringLiteral("Edificio A"));
        QVERIFY(!creado.ok);
        QVERIFY(m_nucleo->listarAulas().valor.isEmpty());

        QVERIFY(m_nucleo->hayPendientes());
        const auto pendientes = m_nucleo->gestorPendientes().pendientes();
        QCOMPARE(pendientes.size(), 1);
        QCOMPARE(pendientes.first().dominio, QStringLiteral("aula"));
        QCOMPARE(static_cast<int>(pendientes.first().tipo),
                 static_cast<int>(TipoOperacion::Alta));
        QCOMPARE(static_cast<int>(estadoPendienteDe(pendientes.first())),
                 static_cast<int>(EstadoPendiente::PendienteGuardar));

        const qint64 id = pendientes.first().id;

        // Retirado el fallo, el reintento persiste el registro.
        QVERIFY(ejecutar(QStringLiteral("DROP TRIGGER fallo_insert_aulas")));
        const auto reintento = m_nucleo->reintentarPendiente(id);
        QVERIFY(reintento.ok);
        QVERIFY(!m_nucleo->hayPendientes());
        QCOMPARE(m_nucleo->listarAulas().valor.size(), 1);
        QCOMPARE(m_nucleo->listarAulas().valor.first().nombre, QStringLiteral("Aula 101"));
    }

    void nucleo_falloDeBaja_registraPendienteEliminarYReintentoBorra()
    {
        const auto aula = m_nucleo->crearAula(QStringLiteral("Aula 202"), 25);
        QVERIFY(aula.ok);

        QVERIFY(ejecutar(QStringLiteral(
            "CREATE TRIGGER fallo_delete_aulas BEFORE DELETE ON Aulas "
            "BEGIN SELECT RAISE(ABORT, 'permiso denegado'); END;")));

        QVERIFY(!m_nucleo->eliminarAula(aula.valor.id));
        // La baja no aplicada se conserva en la base (y en pantalla, en el frontend).
        QCOMPARE(m_nucleo->listarAulas().valor.size(), 1);
        QVERIFY(m_nucleo->hayPendientes());

        const auto pendientes = m_nucleo->gestorPendientes().pendientes();
        QCOMPARE(pendientes.size(), 1);
        QCOMPARE(static_cast<int>(pendientes.first().tipo),
                 static_cast<int>(TipoOperacion::Baja));
        QCOMPARE(static_cast<int>(estadoPendienteDe(pendientes.first())),
                 static_cast<int>(EstadoPendiente::PendienteEliminar));

        QVERIFY(ejecutar(QStringLiteral("DROP TRIGGER fallo_delete_aulas")));
        const auto reintento = m_nucleo->reintentarPendiente(pendientes.first().id);
        QVERIFY(reintento.ok);
        QVERIFY(m_nucleo->listarAulas().valor.isEmpty());
        QVERIFY(!m_nucleo->hayPendientes());
    }

    void nucleo_reintentoSinExito_conservaElPendiente()
    {
        QVERIFY(ejecutar(QStringLiteral(
            "CREATE TRIGGER fallo_insert_aulas BEFORE INSERT ON Aulas "
            "BEGIN SELECT RAISE(ABORT, 'fallo simulado de escritura'); END;")));

        QVERIFY(!m_nucleo->crearAula(QStringLiteral("Aula 303"), 10).ok);
        const qint64 id = m_nucleo->gestorPendientes().pendientes().first().id;

        // El reintento vuelve a fallar: el cambio pendiente se conserva.
        const auto reintento = m_nucleo->reintentarPendiente(id);
        QVERIFY(!reintento.ok);
        QVERIFY(m_nucleo->hayPendientes());
        QCOMPARE(m_nucleo->gestorPendientes().pendientes().size(), 1);
    }
};

QTEST_MAIN(TestGestorPendientes)
#include "test_gestor_pendientes.moc"
