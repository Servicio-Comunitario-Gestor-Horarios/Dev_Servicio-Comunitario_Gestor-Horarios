#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QTest>
#include <QTemporaryDir>
#include <QTime>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioTurnosRecesos.hpp>

/// Tests del servicio de turnos y eventos (RF-1, RF-2) con la compatibilidad
/// transitoria de recesos: `RecesoDTO`/`TurnoDTO.recesos` y
/// `agregarReceso`/`eliminarReceso` se derivan de los eventos.
class TestServicioTurnosEventos : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_dir->filePath("eventos.db")));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioTurnosRecesos(m_dbManager->database()));

        QVERIFY(m_servicio->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        QVERIFY(m_servicio->crearTurno("Tarde", QTime(13, 0), QTime(18, 0), 5).ok);
    }

    void cleanup()
    {
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_dir.reset();
    }

    // ─── Validaciones del alta ──────────────────────────────────────────────

    void agregarEvento_nombreVacio_rechazado()
    {
        auto r = m_servicio->agregarEvento("Mañana", "   ", 2, 10);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("no puede estar vacío"));
    }

    void agregarEvento_turnoInexistente_rechazado()
    {
        auto r = m_servicio->agregarEvento("Nocturno", "Receso", 2, 10);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("No se encontró"));
    }

    void agregarEvento_posicionCero_rechazada()
    {
        auto r = m_servicio->agregarEvento("Mañana", "Receso", 0, 10);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("posición"));
        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_posicionFueraDeRango_rechazada()
    {
        auto r = m_servicio->agregarEvento("Mañana", "Receso", 6, 10);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("posición"));
        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_duracionCero_rechazada()
    {
        auto r = m_servicio->agregarEvento("Mañana", "Receso", 2, 0);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("duración"));
    }

    // ─── Orden dentro de la posición (RF-2) ─────────────────────────────────

    void agregarEvento_asignaOrdenMaximoMasUno()
    {
        auto primero = m_servicio->agregarEvento("Mañana", "Receso", 2, 10);
        auto segundo = m_servicio->agregarEvento("Mañana", "Aviso", 2, 5);

        QVERIFY(primero.ok);
        QVERIFY(segundo.ok);
        QCOMPARE(primero.valor.orden, 1);
        QCOMPARE(segundo.valor.orden, 2);
        QVERIFY(primero.valor.id > 0);
        QVERIFY(segundo.valor.id > 0);
        QVERIFY(primero.valor.id != segundo.valor.id);

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 2);
        QCOMPARE(turno.eventos[0].id, primero.valor.id);
        QCOMPARE(turno.eventos[1].id, segundo.valor.id);
    }

    void agregarEvento_ordenExplicitoRepetido_rechazado()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 2, 10, QTime(), QTime(), 1).ok);

        auto duplicado = m_servicio->agregarEvento("Mañana", "Aviso", 2, 10, QTime(), QTime(), 1);

        QVERIFY(!duplicado.ok);
        QVERIFY(duplicado.mensajeError.contains("orden"));
        QCOMPARE(m_servicio->obtenerTurno("Mañana").valor.eventos.size(), 1);
    }

    void listarEventos_ordenadaPorPosicionYOrden()
    {
        auto enTres = m_servicio->agregarEvento("Mañana", "Acto", 3, 10);
        auto enUno = m_servicio->agregarEvento("Mañana", "Receso", 1, 10);
        auto enUnoDespues = m_servicio->agregarEvento("Mañana", "Aviso", 1, 5);

        QVERIFY(enTres.ok);
        QVERIFY(enUno.ok);
        QVERIFY(enUnoDespues.ok);

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 3);
        QCOMPARE(turno.eventos[0].nombre, QStringLiteral("Receso"));
        QCOMPARE(turno.eventos[0].despuesDeSlot, 1);
        QCOMPARE(turno.eventos[0].orden, 1);
        QCOMPARE(turno.eventos[1].nombre, QStringLiteral("Aviso"));
        QCOMPARE(turno.eventos[1].despuesDeSlot, 1);
        QCOMPARE(turno.eventos[1].orden, 2);
        QCOMPARE(turno.eventos[2].nombre, QStringLiteral("Acto"));
        QCOMPARE(turno.eventos[2].despuesDeSlot, 3);
        QCOMPARE(turno.eventos[2].orden, 1);

        // La lista del listado sigue el mismo orden.
        auto turnos = m_servicio->listarTurnos().valor;
        QCOMPARE(turnos.first().eventos.size(), 3);
        QCOMPARE(turnos.first().eventos[1].nombre, QStringLiteral("Aviso"));
    }

    void eliminarEvento_noRenumera()
    {
        auto uno = m_servicio->agregarEvento("Mañana", "R1", 2, 10);
        auto dos = m_servicio->agregarEvento("Mañana", "R2", 2, 10);
        auto tres = m_servicio->agregarEvento("Mañana", "R3", 2, 10);

        QVERIFY(uno.ok);
        QVERIFY(dos.ok);
        QVERIFY(tres.ok);
        QVERIFY(m_servicio->eliminarEvento(dos.valor.id));

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 2);
        QCOMPARE(turno.eventos[0].nombre, QStringLiteral("R1"));
        QCOMPARE(turno.eventos[0].orden, 1);
        QCOMPARE(turno.eventos[1].nombre, QStringLiteral("R3"));
        QCOMPARE(turno.eventos[1].orden, 3);

        // El alta siguiente usa el máximo + 1, no el recuento.
        auto cuatro = m_servicio->agregarEvento("Mañana", "R4", 2, 10);
        QVERIFY(cuatro.ok);
        QCOMPARE(cuatro.valor.orden, 4);
    }

    // ─── Identidad (RF-2) ───────────────────────────────────────────────────

    void actualizarEvento_porIdentidad_conservaId()
    {
        auto creado = m_servicio->agregarEvento("Mañana", "Receso", 2, 10);
        QVERIFY(creado.ok);

        auto actualizado = m_servicio->actualizarEvento(
            creado.valor.id, "Receso largo", 3, 15, QTime(), QTime(), 1);

        QVERIFY(actualizado.ok);
        QCOMPARE(actualizado.valor.id, creado.valor.id);
        QCOMPARE(actualizado.valor.nombre, QStringLiteral("Receso largo"));
        QCOMPARE(actualizado.valor.despuesDeSlot, 3);
        QCOMPARE(actualizado.valor.duracion, 15);

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 1);
        QCOMPARE(turno.eventos[0].id, creado.valor.id);
        QCOMPARE(turno.eventos[0].nombre, QStringLiteral("Receso largo"));
    }

    void actualizarEvento_noCambiaElTurno()
    {
        auto creado = m_servicio->agregarEvento("Mañana", "Receso", 2, 10);
        QVERIFY(creado.ok);

        // La firma no admite turno: el evento sigue en «Mañana» pese a la edición.
        auto actualizado = m_servicio->actualizarEvento(
            creado.valor.id, "Receso", 2, 12, QTime(), QTime(), 1);
        QVERIFY(actualizado.ok);

        auto manana = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(manana.eventos.size(), 1);
        QCOMPARE(manana.eventos[0].id, creado.valor.id);
        QCOMPARE(manana.eventos[0].duracion, 12);

        auto tarde = m_servicio->obtenerTurno("Tarde").valor;
        QVERIFY(tarde.eventos.isEmpty());
    }

    void actualizarEvento_inexistente_rechazado()
    {
        auto r = m_servicio->actualizarEvento(9999, "Receso", 2, 10, QTime(), QTime(), 1);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("No se encontró"));
    }

    void actualizarEvento_posicionCero_rechazada()
    {
        auto creado = m_servicio->agregarEvento("Mañana", "Receso", 2, 10);
        QVERIFY(creado.ok);

        auto r = m_servicio->actualizarEvento(
            creado.valor.id, "Receso", 0, 10, QTime(), QTime(), 1);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("posición"));
        QCOMPARE(m_servicio->obtenerTurno("Mañana").valor.eventos[0].despuesDeSlot, 2);
    }

    void actualizarEvento_ordenCero_rechazado()
    {
        auto creado = m_servicio->agregarEvento("Mañana", "Receso", 2, 10);
        QVERIFY(creado.ok);

        auto r = m_servicio->actualizarEvento(
            creado.valor.id, "Receso", 2, 10, QTime(), QTime(), 0);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("orden"));
    }

    void actualizarEvento_ordenDuplicadoEnLaPosicion_rechazado()
    {
        auto primero = m_servicio->agregarEvento("Mañana", "R1", 2, 10);
        auto segundo = m_servicio->agregarEvento("Mañana", "R2", 2, 10);
        QVERIFY(primero.ok);
        QVERIFY(segundo.ok);

        auto r = m_servicio->actualizarEvento(
            segundo.valor.id, "R2", 2, 10, QTime(), QTime(), 1);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("orden"));
        QCOMPARE(m_servicio->obtenerTurno("Mañana").valor.eventos[1].orden, 2);
    }

    void eliminarEvento_porIdentidad_borraSoloElIndicado()
    {
        auto uno = m_servicio->agregarEvento("Mañana", "R1", 2, 10);
        auto dos = m_servicio->agregarEvento("Mañana", "R2", 2, 10);
        QVERIFY(uno.ok);
        QVERIFY(dos.ok);

        QVERIFY(m_servicio->eliminarEvento(uno.valor.id));

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 1);
        QCOMPARE(turno.eventos[0].id, dos.valor.id);
        QCOMPARE(turno.eventos[0].nombre, QStringLiteral("R2"));
    }

    void eliminarEvento_inexistente_falso()
    {
        QVERIFY(!m_servicio->eliminarEvento(9999));
        QVERIFY(!m_servicio->eliminarEvento(0));
    }

    void agregarEvento_sinHoras_lasGuardaNulas()
    {
        auto creado = m_servicio->agregarEvento("Mañana", "Receso", 2, 10);
        QVERIFY(creado.ok);
        QVERIFY(!creado.valor.inicio.isValid());
        QVERIFY(!creado.valor.fin.isValid());

        QSqlQuery q(m_dbManager->database());
        q.prepare("SELECT inicio, fin FROM Eventos WHERE id = :id");
        q.bindValue(":id", creado.valor.id);
        QVERIFY(q.exec());
        QVERIFY(q.next());
        QVERIFY(q.value(0).isNull());
        QVERIFY(q.value(1).isNull());
    }

    void eliminarTurno_borraSusEventos()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 2, 10).ok);

        QVERIFY(m_servicio->eliminarTurno("Mañana"));
        QVERIFY(m_servicio->obtenerTurno("Tarde").ok);

        QSqlQuery q(m_dbManager->database());
        q.prepare("SELECT COUNT(*) FROM Eventos WHERE turno = 'Mañana'");
        QVERIFY(q.exec());
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 0);
    }

    // ─── Compatibilidad transitoria: recesos derivados de eventos ───────────

    void compatibilidad_agregarReceso_creaEventoDerivado()
    {
        auto receso = m_servicio->agregarReceso("Mañana", 2, 10);

        QVERIFY(receso.ok);
        QCOMPARE(receso.valor.despuesDeSlot, 2);
        QCOMPARE(receso.valor.duracion, 10);

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 1);
        QCOMPARE(turno.eventos[0].nombre, QStringLiteral("Receso"));
        QCOMPARE(turno.eventos[0].despuesDeSlot, 2);
        QCOMPARE(turno.eventos[0].duracion, 10);
        QCOMPARE(turno.eventos[0].orden, 1);

        // La sección derivada de recesos refleja el evento.
        QCOMPARE(turno.recesos.size(), 1);
        QCOMPARE(turno.recesos[0].despuesDeSlot, 2);
        QCOMPARE(turno.recesos[0].duracion, 10);
    }

    void compatibilidad_eliminarReceso_borraElEvento()
    {
        QVERIFY(m_servicio->agregarReceso("Mañana", 2, 10).ok);
        QVERIFY(m_servicio->agregarEvento("Mañana", "Acto", 4, 20).ok);

        QVERIFY(m_servicio->eliminarReceso("Mañana", 2));

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 1);
        QCOMPARE(turno.eventos[0].nombre, QStringLiteral("Acto"));
        // La vista derivada es una entrada por evento: ya no queda la del receso.
        QCOMPARE(turno.recesos.size(), 1);
        QCOMPARE(turno.recesos[0].duracion, 20);
    }

    void compatibilidad_eliminarReceso_inexistente_falso()
    {
        QVERIFY(!m_servicio->eliminarReceso("Mañana", 2));
    }

    void compatibilidad_agregarReceso_validaIgualQueElEvento()
    {
        auto posicionCero = m_servicio->agregarReceso("Mañana", 0, 10);
        QVERIFY(!posicionCero.ok);
        QVERIFY(posicionCero.mensajeError.contains("posición"));

        auto duracionCero = m_servicio->agregarReceso("Mañana", 2, 0);
        QVERIFY(!duracionCero.ok);
        QVERIFY(duracionCero.mensajeError.contains("duración"));
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioTurnosRecesos> m_servicio;
};

QTEST_MAIN(TestServicioTurnosEventos)
#include "test_servicio_turnos_eventos.moc"
