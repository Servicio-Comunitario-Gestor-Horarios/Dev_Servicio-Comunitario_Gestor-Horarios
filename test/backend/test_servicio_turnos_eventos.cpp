#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QTest>
#include <QTemporaryDir>
#include <QTime>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioConfiguracionHoraria.hpp>
#include <backend/services/ServicioTurnosEventos.hpp>
#include <backend/services/calculo_eventos.hpp>

/// Tests del servicio de turnos y eventos (RF-1, RF-2): los eventos son la
/// única representación del turno (sin capa de compatibilidad de recesos).
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

        m_servicio.reset(new ServicioTurnosEventos(m_dbManager->database()));

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

    /// Fija la configuración global de la hora académica de la base de prueba.
    void fijarDuracion(int duracionMinutos)
    {
        ServicioConfiguracionHoraria configuracion(m_dbManager->database());
        const auto resultado = configuracion.guardar(duracionMinutos);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
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
        QVERIFY(r.mensajeError.contains("1 y 5"));
        QVERIFY(r.mensajeError.contains("se recibió 0"));
        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_posicionFueraDeRango_rechazada()
    {
        auto r = m_servicio->agregarEvento("Mañana", "Receso", 6, 10);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("posición"));
        QVERIFY(r.mensajeError.contains("1 y 5"));
        QVERIFY(r.mensajeError.contains("se recibió 6"));
        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_duracionCero_rechazada()
    {
        auto r = m_servicio->agregarEvento("Mañana", "Receso", 2, 0);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("duración"));
        QVERIFY(r.mensajeError.contains("se recibió 0 min"));
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
        QVERIFY(duplicado.mensajeError.contains("orden 1"));
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
        QVERIFY(r.mensajeError.contains("se recibió 0"));
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
        QVERIFY(r.mensajeError.contains("se recibió 0"));
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
        QVERIFY(r.mensajeError.contains("orden 1"));
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

    // ─── Coherencia temporal del evento (RF-3, RF-4) ────────────────────────
    //
    // Turno de partida «Mañana»: 07:00–12:00, 5 franjas. La franja dura la hora
    // académica configurada (35 min por defecto): la ventana de la posición 1 va
    // de 07:35 a 07:45 con un evento de 10 min y la de la posición 2, de 08:10 a
    // 08:20. El sobrante de la ventana no se reparte.

    void agregarEvento_parDeHorasIncompleto_rechazado()
    {
        auto soloInicio = m_servicio->agregarEvento("Mañana", "Receso", 2, 10,
                                                    QTime(8, 30), QTime());
        QVERIFY(!soloInicio.ok);
        QVERIFY(soloInicio.mensajeError.contains("juntas"));

        auto soloFin = m_servicio->agregarEvento("Mañana", "Receso", 2, 10,
                                                 QTime(), QTime(8, 40));
        QVERIFY(!soloFin.ok);
        QVERIFY(soloFin.mensajeError.contains("juntas"));

        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_finNoPosteriorAlInicio_rechazado()
    {
        auto iguales = m_servicio->agregarEvento("Mañana", "Receso", 2, 10,
                                                 QTime(9, 0), QTime(9, 0));
        QVERIFY(!iguales.ok);
        QVERIFY(iguales.mensajeError.contains("posterior"));

        auto invertidas = m_servicio->agregarEvento("Mañana", "Receso", 2, 10,
                                                    QTime(9, 10), QTime(9, 0));
        QVERIFY(!invertidas.ok);
        QVERIFY(invertidas.mensajeError.contains("posterior"));

        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_duracionNoCoincideConLasHoras_rechazada()
    {
        auto r = m_servicio->agregarEvento("Mañana", "Receso", 1, 10,
                                           QTime(7, 35), QTime(7, 50));

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("no coincide"));
        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_enLaVentanaDeSuPosicion_aceptado()
    {
        // Posición 1 de 07:00–12:00 con D = 35: la franja acaba a las 07:35.
        auto r = m_servicio->agregarEvento("Mañana", "Receso", 1, 10,
                                           QTime(7, 35), QTime(7, 45));

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor.inicio, QTime(7, 35));
        QCOMPARE(r.valor.fin, QTime(7, 45));
    }

    void agregarEvento_fueraDeLaVentanaDeSuPosicion_rechazado()
    {
        auto antes = m_servicio->agregarEvento("Mañana", "Receso", 1, 10,
                                               QTime(7, 30), QTime(7, 40));
        QVERIFY(!antes.ok);
        QVERIFY(antes.mensajeError.contains("ventana"));

        auto despues = m_servicio->agregarEvento("Mañana", "Receso", 1, 10,
                                                 QTime(7, 45), QTime(7, 55));
        QVERIFY(!despues.ok);
        QVERIFY(despues.mensajeError.contains("ventana"));

        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void agregarEvento_solapeEnLaMismaPosicion_rechazado()
    {
        // Con dos eventos de 10 min la ventana de la posición 1 pasa a
        // 07:35–07:55: el segundo cabe pero pisa al primero.
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 1, 10,
                                          QTime(7, 35), QTime(7, 45)).ok);

        auto solapado = m_servicio->agregarEvento("Mañana", "Aviso", 1, 10,
                                                  QTime(7, 40), QTime(7, 50));

        QVERIFY(!solapado.ok);
        QVERIFY(solapado.mensajeError.contains("solapan"));
        QCOMPARE(m_servicio->obtenerTurno("Mañana").valor.eventos.size(), 1);
    }

    void agregarEvento_cronologiaInvertidaRespectoALaPosicion_rechazada()
    {
        // Un evento que empieza antes que el de una posición anterior queda
        // fuera de su propia ventana: las ventanas son disjuntas y ordenadas.
        QVERIFY(m_servicio->agregarEvento("Mañana", "Primero", 2, 10,
                                          QTime(8, 10), QTime(8, 20)).ok);

        auto invertido = m_servicio->agregarEvento("Mañana", "Segundo", 3, 10,
                                                   QTime(8, 0), QTime(8, 10));

        QVERIFY(!invertido.ok);
        QVERIFY(invertido.mensajeError.contains("ventana"));
        QCOMPARE(m_servicio->obtenerTurno("Mañana").valor.eventos.size(), 1);
    }

    void agregarEvento_eventosNoCabenEnElTurno_rechazado()
    {
        // 5 × 35 + 300 = 475 > 300 (déficit 175) y 5 × 35 + 299 = 474 > 300.
        auto ocupaTodo = m_servicio->agregarEvento("Mañana", "Acto", 1, 300);
        QVERIFY(!ocupaTodo.ok);
        QVERIFY2(ocupaTodo.mensajeError.contains("déficit"), qPrintable(ocupaTodo.mensajeError));

        auto dejaSinReparto = m_servicio->agregarEvento("Mañana", "Acto", 1, 299);
        QVERIFY(!dejaSinReparto.ok);
        QVERIFY2(dejaSinReparto.mensajeError.contains("déficit"),
                 qPrintable(dejaSinReparto.mensajeError));

        QVERIFY(m_servicio->obtenerTurno("Mañana").valor.eventos.isEmpty());
    }

    void actualizarEvento_solapeConOtroEvento_rechazado()
    {
        auto receso = m_servicio->agregarEvento("Mañana", "Receso", 1, 10,
                                                QTime(7, 35), QTime(7, 45));
        auto aviso = m_servicio->agregarEvento("Mañana", "Aviso", 2, 10);
        QVERIFY(receso.ok);
        QVERIFY(aviso.ok);

        // El aviso se mueve a la posición 1 con horas que pisan el receso.
        auto r = m_servicio->actualizarEvento(aviso.valor.id, "Aviso", 1, 10,
                                              QTime(7, 40), QTime(7, 50), 2);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("solapan"));

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.eventos.size(), 2);
        QCOMPARE(turno.eventos[1].despuesDeSlot, 2);
        QVERIFY(!turno.eventos[1].inicio.isValid());
    }

    // ─── Reducción de franjas y ventana del turno (RF-4) ───────────────────

    void eventosFueraDeRango_listaAfectados()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 2, 10).ok);
        QVERIFY(m_servicio->agregarEvento("Mañana", "Acto cívico", 4, 10).ok);

        auto afectados = m_servicio->eventosFueraDeRango("Mañana", 3);
        QVERIFY(afectados.ok);
        QCOMPARE(afectados.valor.size(), 1);
        QCOMPARE(afectados.valor.first().nombre, QStringLiteral("Acto cívico"));
        QCOMPARE(afectados.valor.first().despuesDeSlot, 4);

        auto ninguno = m_servicio->eventosFueraDeRango("Mañana", 5);
        QVERIFY(ninguno.ok);
        QVERIFY(ninguno.valor.isEmpty());

        auto inexistente = m_servicio->eventosFueraDeRango("Nocturno", 3);
        QVERIFY(!inexistente.ok);
        QVERIFY(inexistente.mensajeError.contains("No se encontró"));
    }

    void actualizarTurno_reducirSinConfirmar_rechazaYListaAfectados()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 2, 10).ok);
        QVERIFY(m_servicio->agregarEvento("Mañana", "Acto cívico", 4, 10).ok);

        auto r = m_servicio->actualizarTurno("Mañana", QTime(7, 0), QTime(12, 0), 1);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("Mañana"));
        QVERIFY(r.mensajeError.contains("Receso"));
        QVERIFY(r.mensajeError.contains("Acto cívico"));

        // Sin confirmación no se toca nada.
        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.numSlots, 5);
        QCOMPARE(turno.eventos.size(), 2);
    }

    void actualizarTurno_reducirConConfirmacion_borraAfectados()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 2, 10).ok);
        QVERIFY(m_servicio->agregarEvento("Mañana", "Acto cívico", 4, 10).ok);

        auto r = m_servicio->actualizarTurno("Mañana", QTime(7, 0), QTime(12, 0), 3,
                                             true);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor.numSlots, 3);
        QCOMPARE(r.valor.eventos.size(), 1);
        QCOMPARE(r.valor.eventos.first().nombre, QStringLiteral("Receso"));

        QSqlQuery q(m_dbManager->database());
        q.prepare("SELECT COUNT(*) FROM Eventos WHERE turno = 'Mañana'");
        QVERIFY(q.exec());
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    }

    void actualizarTurno_ventanaQueDejaFueraUnEvento_rechazada()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 1, 10,
                                          QTime(7, 35), QTime(7, 45)).ok);

        // Con la campana 08:00–12:00 la ventana de la posición 1 pasa a
        // 08:35–08:45: el receso de 07:35–07:45 ya no cabe.
        auto r = m_servicio->actualizarTurno("Mañana", QTime(8, 0), QTime(12, 0), 5);

        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains("Receso"));

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.inicio, QTime(7, 0));
        QCOMPARE(turno.eventos.size(), 1);
        QCOMPARE(turno.eventos.first().inicio, QTime(7, 35));
    }

    void actualizarTurno_ventanaQueNoDaEncaje_rechazada()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 2, 10).ok);
        QVERIFY(m_servicio->agregarEvento("Mañana", "Aviso", 4, 10).ok);

        // 5 × 35 + 20 = 195 min > 20 de ventana: el mensaje es el de `errorDeEncaje`.
        auto r = m_servicio->actualizarTurno("Mañana", QTime(7, 0), QTime(7, 20), 5);

        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains("Mañana"), qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains("déficit"), qPrintable(r.mensajeError));

        auto turno = m_servicio->obtenerTurno("Mañana").valor;
        QCOMPARE(turno.fin, QTime(12, 0));
        QCOMPARE(turno.eventos.size(), 2);
    }

    void actualizarTurno_sinAfectados_conservaEventos()
    {
        QVERIFY(m_servicio->agregarEvento("Mañana", "Receso", 2, 10).ok);

        auto r = m_servicio->actualizarTurno("Mañana", QTime(7, 0), QTime(13, 0), 6);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor.numSlots, 6);
        QCOMPARE(r.valor.fin, QTime(13, 0));
        QCOMPARE(r.valor.eventos.size(), 1);
    }

    // ─── Encaje con la hora académica configurada (RF-4, T5) ────────────────
    //
    // La D efectiva sale de `ConfiguracionHoraria` (35 por defecto). La
    // invariante es `franjas × D + eventos ≤ ventana`; si falla, no se escribe.

    void crearTurno_ochoFranjasYVentana290_aceptaConDuracion35()
    {
        // 8 × 35 = 280 ≤ 290: cabe sin eventos.
        auto r = m_servicio->crearTurno("Largo", QTime(7, 0), QTime(11, 50), 8);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor.numSlots, 8);
        QCOMPARE(r.valor.inicio, QTime(7, 0));
        QCOMPARE(r.valor.fin, QTime(11, 50));
    }

    void crearTurno_cincoFranjasEnCincoHoras_sigueAceptandose()
    {
        // 5 × 35 = 175 ≤ 300: el turno de 5 franjas en 5 h se sigue creando.
        auto r = m_servicio->crearTurno("Corto", QTime(7, 0), QTime(12, 0), 5);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor.numSlots, 5);
    }

    void crearTurno_conDuracion45YVentana290_rechazaConDeficit70()
    {
        fijarDuracion(45);

        // 8 × 45 = 360 > 290: faltan 70.
        auto r = m_servicio->crearTurno("Ajustado", QTime(7, 0), QTime(11, 50), 8);

        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains("Ajustado"), qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains("45"), qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains("déficit 70"), qPrintable(r.mensajeError));

        // No se escribió nada.
        QVERIFY(!m_servicio->obtenerTurno("Ajustado").ok);

        // El mensaje es el de `errorDeEncaje`: una sola definición.
        TurnoDTO esperado;
        esperado.nombre   = QStringLiteral("Ajustado");
        esperado.inicio   = QTime(7, 0);
        esperado.fin      = QTime(11, 50);
        esperado.numSlots = 8;
        QCOMPARE(r.mensajeError, errorDeEncaje(esperado, 45));
    }

    void agregarEvento_trasSubirLaDuracionA45_rechazaConTurnoDuracionYDeficit()
    {
        QVERIFY(m_servicio->crearTurno("Largo", QTime(7, 0), QTime(11, 50), 8).ok);
        fijarDuracion(45);

        // 8 × 45 + 10 = 370 > 290: el evento rompe la invariante y no se escribe.
        auto r = m_servicio->agregarEvento("Largo", "Receso", 2, 10);

        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains("Largo"), qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains("45"), qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains("déficit 80"), qPrintable(r.mensajeError));
        QVERIFY(m_servicio->obtenerTurno("Largo").valor.eventos.isEmpty());

        // Mismo mensaje que `errorDeEncaje` con el candidato dentro.
        TurnoDTO esperado = m_servicio->obtenerTurno("Largo").valor;
        EventoDTO candidato;
        candidato.nombre        = QStringLiteral("Receso");
        candidato.despuesDeSlot = 2;
        candidato.orden         = 1;
        candidato.duracion      = 10;
        esperado.eventos.append(candidato);
        QCOMPARE(r.mensajeError, errorDeEncaje(esperado, 45));
    }

    void actualizarTurno_conDuracion45_rechazaSinEscribir()
    {
        QVERIFY(m_servicio->crearTurno("Largo", QTime(7, 0), QTime(11, 50), 8).ok);
        fijarDuracion(45);

        // Aunque la ventana no cambie, 8 franjas ya no encajan con 45 min.
        auto r = m_servicio->actualizarTurno("Largo", QTime(7, 0), QTime(11, 50), 8);

        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains("déficit 70"), qPrintable(r.mensajeError));

        auto turno = m_servicio->obtenerTurno("Largo").valor;
        QCOMPARE(turno.numSlots, 8);
        QCOMPARE(turno.inicio, QTime(7, 0));
        QCOMPARE(turno.fin, QTime(11, 50));
    }

    void actualizarEvento_trasSubirLaDuracion_rechazaSinEscribir()
    {
        QVERIFY(m_servicio->crearTurno("Largo", QTime(7, 0), QTime(11, 50), 8).ok);
        // Con D = 35: 8 × 35 + 10 = 290, cabe justo.
        auto creado = m_servicio->agregarEvento("Largo", "Receso", 2, 10);
        QVERIFY2(creado.ok, qPrintable(creado.mensajeError));

        fijarDuracion(45);

        // Con D = 45: 8 × 45 + 12 = 372 > 290; la edición se rechaza.
        auto r = m_servicio->actualizarEvento(creado.valor.id, "Receso", 2, 12,
                                              QTime(), QTime(), 1);

        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains("Largo"), qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains("déficit 82"), qPrintable(r.mensajeError));

        auto turno = m_servicio->obtenerTurno("Largo").valor;
        QCOMPARE(turno.eventos.size(), 1);
        QCOMPARE(turno.eventos.first().duracion, 10); // no se escribió el 12
    }

    // ─── Compatibilidad retirada (T9) ─────────────────────────────────────
    //
    // La vista de recesos y los métodos antiguos ya no existen: los eventos son
    // la única representación. Su cobertura vive en los tests de eventos
    // (`agregarEvento_*`, `actualizarEvento_*`, `eliminarEvento_*`).

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioTurnosEventos> m_servicio;
};

QTEST_MAIN(TestServicioTurnosEventos)
#include "test_servicio_turnos_eventos.moc"
