#include <QPair>
#include <QTest>
#include <QTime>
#include <QVector>

#include <backend/services/ServicioTurnosRecesos.hpp>
#include <backend/services/calculo_eventos.hpp>

namespace
{
/// Evento de conveniencia para las pruebas: posición, orden, duración y horas.
EventoDTO evento(const QString& nombre, int posicion, int orden, int duracion,
                 const QTime& inicio = QTime(), const QTime& fin = QTime())
{
    EventoDTO e;
    e.nombre        = nombre;
    e.despuesDeSlot = posicion;
    e.orden         = orden;
    e.duracion      = duracion;
    e.inicio        = inicio;
    e.fin           = fin;
    return e;
}

/// Turno de conveniencia: nombre, ventana y número de franjas.
TurnoDTO turno(const QString& nombre, const QTime& inicio, const QTime& fin, int numSlots)
{
    TurnoDTO t;
    t.nombre   = nombre;
    t.inicio   = inicio;
    t.fin      = fin;
    t.numSlots = numSlots;
    return t;
}
} // namespace

/// Pruebas de las funciones puras de `calculo_eventos` (RF-3, RF-4): reparto de
/// la franja, ventana de cada posición, contención, solapes y orden.
///
/// La ventana de la posición k va del fin de la franja k al inicio de la k+1; la
/// última llega al fin del turno (N5 de la 009). La franja estimada reparte
/// `ventana − Σ duraciones` entre las franjas (plan §3).
class TestCalculoEventos : public QObject
{
    Q_OBJECT

private slots:
    // ─── duracionFranjaEstimada ─────────────────────────────────────────────

    void duracionFranjaEstimada_reparteVentanaMenosEventos()
    {
        // 330 min de ventana − 10 del evento = 320; 320 / 8 = 40.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(12, 30), 8);
        t.eventos.append(evento("Receso", 2, 1, 10));

        QCOMPARE(duracionFranjaEstimada(t), 40);
    }

    void duracionFranjaEstimada_sinEventos()
    {
        // 300 min / 5 franjas = 60.
        const TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(12, 0), 5);

        QCOMPARE(duracionFranjaEstimada(t), 60);
    }

    void duracionFranjaEstimada_numSlotsCero_devuelveCero()
    {
        // Guarda: sin franjas no se divide (antes provocaba división por cero).
        TurnoDTO t = turno("Roto", QTime(7, 0), QTime(12, 0), 0);
        t.eventos.append(evento("Receso", 1, 1, 10));

        QCOMPARE(duracionFranjaEstimada(t), 0);
    }

    void duracionFranjaEstimada_numSlotsNegativo_devuelveCero()
    {
        TurnoDTO t = turno("Roto", QTime(7, 0), QTime(12, 0), -3);
        t.eventos.append(evento("Receso", 1, 1, 10));

        QCOMPARE(duracionFranjaEstimada(t), 0);
    }

    void duracionFranjaEstimada_ventanaInvalida_devuelveCero()
    {
        QCOMPARE(duracionFranjaEstimada(turno("Invertido", QTime(12, 0), QTime(7, 0), 5)), 0);
        QCOMPARE(duracionFranjaEstimada(turno("SinHoras", QTime(), QTime(), 5)), 0);
    }

    void duracionFranjaEstimada_eventosNoCaben_devuelveCero()
    {
        // Σ duraciones == ventana: no queda reparto.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(7, 20), 2);
        t.eventos.append(evento("A", 1, 1, 10));
        t.eventos.append(evento("B", 2, 1, 10));

        QCOMPARE(duracionFranjaEstimada(t), 0);
    }

    void duracionFranjaEstimada_repartoNoPositivo_devuelveCero()
    {
        // 300 − 299 = 1 min para 5 franjas: el reparto entero es 0.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(12, 0), 5);
        t.eventos.append(evento("Largo", 1, 1, 299));

        QCOMPARE(duracionFranjaEstimada(t), 0);
    }

    // ─── ventanaDePosicion ──────────────────────────────────────────────────

    void ventanaDePosicion_delFinDeLaFranjaAlInicioDeLaSiguiente()
    {
        // 360 min − 30 de eventos = 330; 330 / 6 = 55 min de franja.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(13, 0), 6);
        t.eventos.append(evento("Receso", 2, 1, 10));
        t.eventos.append(evento("Acto", 6, 1, 20));

        const auto segunda = ventanaDePosicion(t, 2);
        QCOMPARE(segunda.first, QTime(8, 50));
        QCOMPARE(segunda.second, QTime(9, 0));

        // La siguiente empieza donde acaba la anterior más una franja.
        const auto tercera = ventanaDePosicion(t, 3);
        QCOMPARE(tercera.first, QTime(9, 55));
        QCOMPARE(tercera.second, QTime(9, 55));

        // La última termina en el fin del turno.
        const auto sexta = ventanaDePosicion(t, 6);
        QCOMPARE(sexta.first, QTime(12, 40));
        QCOMPARE(sexta.second, QTime(13, 0));
    }

    void ventanaDePosicion_ultimaLlegaAlFinDelTurnoConResto()
    {
        // 331 min − 10 = 321; 321 / 8 = 40 (resto 1). La última llega al fin.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(12, 31), 8);
        t.eventos.append(evento("Receso", 2, 1, 10));

        const auto ultima = ventanaDePosicion(t, 8);
        QCOMPARE(ultima.first, QTime(12, 30));
        QCOMPARE(ultima.second, QTime(12, 31));
    }

    void ventanaDePosicion_posicionFueraDeRango_ventanaInvalida()
    {
        const TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(12, 0), 5);

        for (int posicion : {0, 6, -1})
        {
            const auto ventana = ventanaDePosicion(t, posicion);
            QVERIFY(!ventana.first.isValid());
            QVERIFY(!ventana.second.isValid());
        }
    }

    void ventanaDePosicion_turnoSinReparto_ventanaInvalida()
    {
        // Sin franjas no hay rejilla: la ventana no existe.
        TurnoDTO t = turno("Roto", QTime(7, 0), QTime(12, 0), 0);
        t.eventos.append(evento("Receso", 1, 1, 10));

        const auto ventana = ventanaDePosicion(t, 1);
        QVERIFY(!ventana.first.isValid());
        QVERIFY(!ventana.second.isValid());
    }

    // ─── eventoDentroDeVentana ──────────────────────────────────────────────

    void eventoDentroDeVentana_dentro_verdadero()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(13, 0), 6);
        const EventoDTO receso = evento("Receso", 2, 1, 10, QTime(8, 56), QTime(9, 6));
        t.eventos.append(receso);

        QVERIFY(eventoDentroDeVentana(receso, t, 2));
    }

    void eventoDentroDeVentana_antesDeLaVentana_falso()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(13, 0), 6);
        const EventoDTO receso = evento("Receso", 2, 1, 10, QTime(8, 46), QTime(8, 56));
        t.eventos.append(receso);

        QVERIFY(!eventoDentroDeVentana(receso, t, 2));
    }

    void eventoDentroDeVentana_despuesDeLaVentana_falso()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(13, 0), 6);
        const EventoDTO receso = evento("Receso", 2, 1, 10, QTime(9, 6), QTime(9, 16));
        t.eventos.append(receso);

        QVERIFY(!eventoDentroDeVentana(receso, t, 2));
    }

    void eventoDentroDeVentana_sinHoras_verdadero()
    {
        // Sin horas no hay intervalo que comprobar.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(13, 0), 6);
        const EventoDTO receso = evento("Receso", 2, 1, 10);
        t.eventos.append(receso);

        QVERIFY(eventoDentroDeVentana(receso, t, 2));
    }

    // ─── haySolapeCronologico ───────────────────────────────────────────────

    void haySolape_sinEventos_falso()
    {
        QVERIFY(!haySolapeCronologico({}));
    }

    void haySolape_eventosSinHoras_falso()
    {
        QVector<EventoDTO> eventos;
        eventos.append(evento("A", 1, 1, 10));
        eventos.append(evento("B", 1, 2, 10));

        QVERIFY(!haySolapeCronologico(eventos));
    }

    void haySolape_mismaPosicion_verdadero()
    {
        QVector<EventoDTO> eventos;
        eventos.append(evento("A", 2, 1, 10, QTime(8, 50), QTime(9, 0)));
        eventos.append(evento("B", 2, 2, 10, QTime(8, 55), QTime(9, 5)));

        QVERIFY(haySolapeCronologico(eventos));
    }

    void haySolape_entrePosicionesInvertido_verdadero()
    {
        QVector<EventoDTO> eventos;
        eventos.append(evento("Primero", 1, 1, 10, QTime(9, 0), QTime(9, 10)));
        eventos.append(evento("Segundo", 2, 1, 10, QTime(8, 50), QTime(9, 0)));

        QVERIFY(haySolapeCronologico(eventos));
    }

    void haySolape_contiguoSinSolape_falso()
    {
        QVector<EventoDTO> eventos;
        eventos.append(evento("A", 1, 1, 10, QTime(8, 0), QTime(8, 10)));
        eventos.append(evento("B", 2, 1, 10, QTime(8, 10), QTime(8, 20)));

        QVERIFY(!haySolapeCronologico(eventos));
    }

    void haySolape_ordenDeEntradaNoImporta_verdadero()
    {
        // Aunque lleguen desordenados, se comparan por (posición, orden).
        QVector<EventoDTO> eventos;
        eventos.append(evento("B", 2, 1, 10, QTime(8, 55), QTime(9, 5)));
        eventos.append(evento("A", 2, 2, 10, QTime(8, 50), QTime(9, 0)));

        QVERIFY(haySolapeCronologico(eventos));
    }

    void haySolape_tercerEventoSolapaConElPrimero_verdadero()
    {
        // B cabe dentro de A y C empieza antes de que A acabe: hay que recordar
        // el fin máximo, no solo el del evento anterior.
        QVector<EventoDTO> eventos;
        eventos.append(evento("A", 1, 1, 120, QTime(8, 0), QTime(10, 0)));
        eventos.append(evento("B", 1, 2, 10, QTime(9, 0), QTime(9, 10)));
        eventos.append(evento("C", 1, 3, 10, QTime(9, 20), QTime(9, 30)));

        QVERIFY(haySolapeCronologico(eventos));
    }

    // ─── ordenSiguiente ─────────────────────────────────────────────────────

    void ordenSiguiente_vacio_uno()
    {
        QCOMPARE(ordenSiguiente({}, 2), 1);
    }

    void ordenSiguiente_maximoMasUnoPorPosicion()
    {
        QVector<EventoDTO> eventos;
        eventos.append(evento("A", 2, 1, 10));
        eventos.append(evento("B", 2, 3, 10));
        eventos.append(evento("C", 1, 7, 10));

        QCOMPARE(ordenSiguiente(eventos, 2), 4);
        QCOMPARE(ordenSiguiente(eventos, 1), 8);
        QCOMPARE(ordenSiguiente(eventos, 3), 1);
    }

    // ─── eventosFueraDeRango ────────────────────────────────────────────────

    void eventosFueraDeRango_listaLosDePosicionesMayores()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(12, 0), 5);
        t.eventos.append(evento("Dentro", 2, 1, 10));
        t.eventos.append(evento("Borde", 3, 1, 10));
        t.eventos.append(evento("Fuera", 5, 1, 10));

        const auto afectados = eventosFueraDeRango(t, 3);
        QCOMPARE(afectados.size(), 1);
        QCOMPARE(afectados.first().nombre, QStringLiteral("Fuera"));
        QCOMPARE(afectados.first().despuesDeSlot, 5);
    }

    void eventosFueraDeRango_sinAfectados_vacio()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(12, 0), 5);
        t.eventos.append(evento("Dentro", 2, 1, 10));
        t.eventos.append(evento("Borde", 5, 1, 10));

        QVERIFY(eventosFueraDeRango(t, 5).isEmpty());
    }
};

QTEST_MAIN(TestCalculoEventos)
#include "test_calculo_eventos.moc"
