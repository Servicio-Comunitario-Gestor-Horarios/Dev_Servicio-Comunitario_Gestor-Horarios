#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioConfiguracionHoraria.hpp>
#include <backend/services/duracion_horaria.hpp>

/// Tests del servicio de la configuración global de la hora académica (RF-1,
/// RF-2): valor efectivo con su origen, rango 30–90 y upsert de la fila única.
/// La base se crea con el esquema v6, que ya deja `(1, 35)`.
class TestServicioConfiguracionHoraria : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioConfiguracionHoraria> m_servicio;

    /// Borra la fila para simular una base con esquema y sin valor guardado.
    void borrarFila()
    {
        QSqlQuery query(m_dbManager->database());
        QVERIFY2(query.exec(QStringLiteral("DELETE FROM ConfiguracionHoraria")),
                 qPrintable(query.lastError().text()));
    }

    /// Número de filas de configuración: nunca puede pasar de una (RF-1).
    int filas() const
    {
        QSqlQuery query(m_dbManager->database());
        if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM ConfiguracionHoraria"))
            || !query.next())
            return -1;
        return query.value(0).toInt();
    }

    /// P10 de la 015: quita los dos disparadores y borra la fila del tipo general.
    void invalidarSinTipoGeneral()
    {
        QSqlQuery query(m_dbManager->database());
        QVERIFY2(query.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar")),
                 qPrintable(query.lastError().text()));
        QVERIFY2(query.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_renombrar")),
                 qPrintable(query.lastError().text()));
        QVERIFY2(query.exec(QStringLiteral("DELETE FROM TiposAula")),
                 qPrintable(query.lastError().text()));
    }

    /// Quita solo el disparador de borrado: la base deja de ser válida.
    void invalidarSinDisparadorDeBorrado()
    {
        QSqlQuery query(m_dbManager->database());
        QVERIFY2(query.exec(QStringLiteral("DROP TRIGGER TiposAula_general_no_borrar")),
                 qPrintable(query.lastError().text()));
    }

    /// El rechazo de la base lleva la causa y el sufijo de la operación (RNF-1).
    void esperarRechazoDeBase(const Resultado<ConfiguracionHoraria>& resultado)
    {
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("esquema esperado")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(
                     QStringLiteral("No se guardó la configuración de la hora académica.")),
                 qPrintable(resultado.mensajeError));
    }

    /// El mensaje identifica el rango, el valor y la acción disponible (RNF-1).
    void esperarRechazoConRangoYAccion(const Resultado<ConfiguracionHoraria>& resultado,
                                       int duracion)
    {
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("30")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("90")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QString::number(duracion)),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(
                     QStringLiteral("Ajuste la duración a un valor del rango")),
                 qPrintable(resultado.mensajeError));
    }

private slots:

    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_dir->filePath(QStringLiteral("configuracion.db"))));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioConfiguracionHoraria(m_dbManager->database()));
    }

    void cleanup()
    {
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_dir.reset();
    }

    // ─── consultar ──────────────────────────────────────────────────────────

    void consultar_sinFila_devuelve35PorDefectoSinError()
    {
        borrarFila();

        const auto resultado = m_servicio->consultar();

        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
        QCOMPARE(resultado.valor.duracionMinutos, duracion_horaria::POR_DEFECTO);
        QVERIFY(resultado.valor.porDefecto);
    }

    void consultar_conFila_devuelveElValorGuardadoYNoEsDefecto()
    {
        // La creación del esquema deja `(1, 35)`: hay fila aunque el valor coincida.
        const auto inicial = m_servicio->consultar();

        QVERIFY2(inicial.ok, qPrintable(inicial.mensajeError));
        QCOMPARE(inicial.valor.duracionMinutos, 35);
        QVERIFY(!inicial.valor.porDefecto);

        QVERIFY(m_servicio->guardar(45).ok);

        const auto guardada = m_servicio->consultar();
        QVERIFY2(guardada.ok, qPrintable(guardada.mensajeError));
        QCOMPARE(guardada.valor.duracionMinutos, 45);
        QVERIFY(!guardada.valor.porDefecto);
    }

    // ─── guardar ────────────────────────────────────────────────────────────

    void guardar_valido_haceUpsertDeUnaSolaFila()
    {
        QVERIFY(m_servicio->guardar(45).ok);
        QCOMPARE(filas(), 1);
        QCOMPARE(m_servicio->consultar().valor.duracionMinutos, 45);

        // Volver a guardar actualiza la misma fila (no duplica) y acepta los extremos.
        QVERIFY(m_servicio->guardar(30).ok);
        QVERIFY(m_servicio->guardar(90).ok);
        QCOMPARE(filas(), 1);
        QCOMPARE(m_servicio->consultar().valor.duracionMinutos, 90);
    }

    void guardar_invalido_fallaConRangoYConservaElAnterior()
    {
        QVERIFY(m_servicio->guardar(45).ok);

        esperarRechazoConRangoYAccion(m_servicio->guardar(29), 29);
        esperarRechazoConRangoYAccion(m_servicio->guardar(91), 91);
        esperarRechazoConRangoYAccion(m_servicio->guardar(5), 5);

        const auto conservada = m_servicio->consultar();
        QVERIFY2(conservada.ok, qPrintable(conservada.mensajeError));
        QCOMPARE(conservada.valor.duracionMinutos, 45);
        QVERIFY(!conservada.valor.porDefecto);
        QCOMPARE(filas(), 1);
    }

    void guardar_baseSinElTipoGeneral_rechazaSinEscribir()
    {
        QVERIFY(m_servicio->guardar(45).ok);
        invalidarSinTipoGeneral();

        const auto resultado = m_servicio->guardar(40);

        esperarRechazoDeBase(resultado);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("aula de estudio")),
                 qPrintable(resultado.mensajeError));

        // No se escribió: el valor anterior y el número de filas se conservan.
        QCOMPARE(filas(), 1);
        const auto conservada = m_servicio->consultar();
        QVERIFY2(conservada.ok, qPrintable(conservada.mensajeError));
        QCOMPARE(conservada.valor.duracionMinutos, 45);
        QVERIFY(!conservada.valor.porDefecto);
    }

    void guardar_baseSinDisparador_rechazaSinEscribir()
    {
        invalidarSinDisparadorDeBorrado();

        const auto resultado = m_servicio->guardar(40);

        esperarRechazoDeBase(resultado);
        QVERIFY2(resultado.mensajeError.contains(
                     QStringLiteral("TiposAula_general_no_borrar")),
                 qPrintable(resultado.mensajeError));

        QCOMPARE(filas(), 1);
        const auto conservada = m_servicio->consultar();
        QVERIFY2(conservada.ok, qPrintable(conservada.mensajeError));
        QCOMPARE(conservada.valor.duracionMinutos, 35);
        QVERIFY(!conservada.valor.porDefecto);
    }

    void guardar_baseInvalidaTienePrioridadSobreElRango()
    {
        invalidarSinDisparadorDeBorrado();

        // 29 está fuera de rango, pero el fallo estructural se informa primero.
        const auto resultado = m_servicio->guardar(29);

        esperarRechazoDeBase(resultado);
        QVERIFY2(resultado.mensajeError.contains(
                     QStringLiteral("TiposAula_general_no_borrar")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(!resultado.mensajeError.contains(
                     QStringLiteral("Ajuste la duración a un valor del rango")),
                 qPrintable(resultado.mensajeError));
    }

    void consultar_conBaseInvalida_sigueLeyendo()
    {
        QVERIFY(m_servicio->guardar(45).ok);
        invalidarSinDisparadorDeBorrado();

        // La comprobación es solo al guardar: consultar sigue devolviendo el valor.
        const auto leida = m_servicio->consultar();

        QVERIFY2(leida.ok, qPrintable(leida.mensajeError));
        QCOMPARE(leida.valor.duracionMinutos, 45);
        QVERIFY(!leida.valor.porDefecto);
    }

    void guardar_sinFila_creaLaFila()
    {
        borrarFila();

        const auto resultado = m_servicio->guardar(40);

        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
        QCOMPARE(resultado.valor.duracionMinutos, 40);
        QVERIFY(!resultado.valor.porDefecto);
        QCOMPARE(filas(), 1);

        const auto leida = m_servicio->consultar();
        QVERIFY2(leida.ok, qPrintable(leida.mensajeError));
        QCOMPARE(leida.valor.duracionMinutos, 40);
        QVERIFY(!leida.valor.porDefecto);
    }

    void guardar_invalidoSinFila_noEscribeNiCrea()
    {
        borrarFila();

        esperarRechazoConRangoYAccion(m_servicio->guardar(29), 29);

        QCOMPARE(filas(), 0);

        const auto leida = m_servicio->consultar();
        QVERIFY2(leida.ok, qPrintable(leida.mensajeError));
        QCOMPARE(leida.valor.duracionMinutos, duracion_horaria::POR_DEFECTO);
        QVERIFY(leida.valor.porDefecto);
    }
};

QTEST_MAIN(TestServicioConfiguracionHoraria)
#include "test_servicio_configuracion_horaria.moc"
