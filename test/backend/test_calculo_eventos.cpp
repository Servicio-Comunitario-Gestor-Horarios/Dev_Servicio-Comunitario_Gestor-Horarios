#include <QPair>
#include <QTest>
#include <QTime>
#include <QVector>

#include <backend/services/ServicioTurnosEventos.hpp>
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

    // ─── minutosDeVentana y tiempoDeEventos (públicas desde la 011) ────────

    void minutosDeVentanaYTiempoDeEventos_sumanLaVentanaYLosEventos()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);
        t.eventos.append(evento("Receso", 2, 1, 10));
        t.eventos.append(evento("SinDuracion", 8, 1, 0)); // no positiva: no suma

        QCOMPARE(minutosDeVentana(t), 290);
        QCOMPARE(tiempoDeEventos(t), 10);
    }

    // ─── franjasUtilizables(turno, D) (RF-4) ────────────────────────────────

    void franjasUtilizables_conLaDuracionDeLaHoraAcademica()
    {
        // (290 − 10) / 35 = 8; (290 − 10) / 45 = 6,22 → 6. El sobrante no se reparte.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);
        t.eventos.append(evento("Receso", 2, 1, 10));

        QCOMPARE(franjasUtilizables(t, 35), 8);
        QCOMPARE(franjasUtilizables(t, 45), 6);
    }

    void franjasUtilizables_turnoCortoYDuracionInvalida()
    {
        // (150 − 10) / 35 = 4.
        TurnoDTO t = turno("Corto", QTime(7, 0), QTime(9, 30), 5);
        t.eventos.append(evento("Receso", 1, 1, 10));

        QCOMPARE(franjasUtilizables(t, 35), 4);
        QCOMPARE(franjasUtilizables(t, 0), 0);
        QCOMPARE(franjasUtilizables(turno("SinHoras", QTime(), QTime(), 5), 35), 0);
    }

    // ─── deficitDeEncaje(turno, D) (RF-4) ───────────────────────────────────

    void deficitDeEncaje_conLaDuracionDeLaHoraAcademica()
    {
        // 8 × 45 + 10 = 370 > 290 → faltan 80.
        TurnoDTO manana = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);
        manana.eventos.append(evento("Receso", 2, 1, 10));
        QCOMPARE(deficitDeEncaje(manana, 45), 80);
        // 8 × 35 + 10 = 290 = ventana → cabe justo.
        QCOMPARE(deficitDeEncaje(manana, 35), 0);

        // 8 × 35 + 10 = 290 ≤ 370 → sobrante no usado.
        TurnoDTO largo = turno("Largo", QTime(7, 0), QTime(13, 10), 8);
        largo.eventos.append(evento("Receso", 2, 1, 10));
        QCOMPARE(deficitDeEncaje(largo, 35), 0);

        // 5 × 35 + 10 = 185 > 150 → faltan 35.
        TurnoDTO corto = turno("Corto", QTime(7, 0), QTime(9, 30), 5);
        corto.eventos.append(evento("Receso", 1, 1, 10));
        QCOMPARE(deficitDeEncaje(corto, 35), 35);
    }

    // ─── errorDeEncaje(turno, D) (RF-4, RNF-1) ──────────────────────────────

    void errorDeEncaje_citaTurnoDuracionYDeficit()
    {
        TurnoDTO manana = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);
        manana.eventos.append(evento("Receso", 2, 1, 10));

        const QString error = errorDeEncaje(manana, 45);

        QVERIFY(!error.isEmpty());
        QVERIFY2(error.contains(QStringLiteral("Mañana")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("45")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("déficit 80")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("Ajuste el número de franjas")), qPrintable(error));
    }

    void errorDeEncaje_siCabe_esVacio()
    {
        TurnoDTO largo = turno("Largo", QTime(7, 0), QTime(13, 10), 8);
        largo.eventos.append(evento("Receso", 2, 1, 10));

        QVERIFY(errorDeEncaje(largo, 35).isEmpty());
    }

    void errorDeEncaje_turnoCortoCon35_citaDeficit35()
    {
        TurnoDTO corto = turno("Corto", QTime(7, 0), QTime(9, 30), 5);
        corto.eventos.append(evento("Receso", 1, 1, 10));

        QVERIFY2(errorDeEncaje(corto, 35).contains(QStringLiteral("déficit 35")),
                 qPrintable(errorDeEncaje(corto, 35)));
    }

    void errorDeEncaje_turnoSinVentana_pideDeclararLasHoras()
    {
        const TurnoDTO sinHoras = turno("SinHoras", QTime(), QTime(), 5);

        const QString error = errorDeEncaje(sinHoras, 35);

        QVERIFY(!error.isEmpty());
        QVERIFY2(error.contains(QStringLiteral("SinHoras")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("Declare sus horas")), qPrintable(error));
    }

    void errorDeEncaje_duracionFueraDeRango_devuelveElMensajeDelRango()
    {
        // La validación del rango es la de `duracion_horaria`, no se duplica aquí.
        const TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);

        const QString error = errorDeEncaje(t, 5);

        QVERIFY2(error.contains(QStringLiteral("30")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("90")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("Ajuste la duración a un valor del rango")),
                 qPrintable(error));
    }

    // ─── ventanaDePosicion(turno, posicion, D) (RF-4) ───────────────────────

    void ventanaDePosicion_conDuracion_franjaDeAnchoD()
    {
        // 07:00–11:50, 8 franjas de 35 y un receso de 10 tras la 2.ª: la posición
        // 2 va de 08:10 a 08:20 y la última empieza/termina en el fin del turno.
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);
        t.eventos.append(evento("Receso", 2, 1, 10));

        const auto segunda = ventanaDePosicion(t, 2, 35);
        QCOMPARE(segunda.first, QTime(8, 10));
        QCOMPARE(segunda.second, QTime(8, 20));

        const auto primera = ventanaDePosicion(t, 1, 35);
        QCOMPARE(primera.first, QTime(7, 35));
        QCOMPARE(primera.second, QTime(7, 35));

        const auto ultima = ventanaDePosicion(t, 8, 35);
        QCOMPARE(ultima.first, QTime(11, 50));
        QCOMPARE(ultima.second, QTime(11, 50));
    }

    void ventanaDePosicion_conDuracion_invalidaFueraDeRangoSinFranjaOSinVentana()
    {
        const TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);

        for (int posicion : {0, 9, -1})
        {
            const auto ventana = ventanaDePosicion(t, posicion, 35);
            QVERIFY(!ventana.first.isValid());
            QVERIFY(!ventana.second.isValid());
        }

        const auto sinFranja = ventanaDePosicion(t, 1, 0);
        QVERIFY(!sinFranja.first.isValid());

        const auto sinVentana = ventanaDePosicion(turno("SinHoras", QTime(), QTime(), 8), 1, 35);
        QVERIFY(!sinVentana.first.isValid());
    }

    // ─── eventoDentroDeVentana(evento, turno, posicion, D) (RF-4) ───────────

    void eventoDentroDeVentana_conDuracion_acotaLaVentanaDelEvento()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(11, 50), 8);
        const EventoDTO receso = evento("Receso", 2, 1, 10, QTime(8, 10), QTime(8, 20));
        t.eventos.append(receso);

        QVERIFY(eventoDentroDeVentana(receso, t, 2, 35));

        // Con D = 45 la ventana de la posición 2 es 08:30–08:40: el receso no cabe.
        QVERIFY(!eventoDentroDeVentana(receso, t, 2, 45));

        // Sin horas no hay intervalo que comprobar.
        const EventoDTO sinHoras = evento("SinHoras", 2, 1, 10);
        QVERIFY(eventoDentroDeVentana(sinHoras, t, 2, 35));
    }

    // ─── legado temporal: las firmas antiguas delegan con D = estimada ─────

    void ventanaDePosicion_legadoDelegaEnLaFranjaEstimada()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(13, 0), 6);
        t.eventos.append(evento("Receso", 2, 1, 10));
        t.eventos.append(evento("Acto", 6, 1, 20));

        const int estimada = duracionFranjaEstimada(t);
        for (int posicion = 1; posicion <= t.numSlots; ++posicion)
            QCOMPARE(ventanaDePosicion(t, posicion),
                     ventanaDePosicion(t, posicion, estimada));
    }

    void eventoDentroDeVentana_legadoDelegaEnLaFranjaEstimada()
    {
        TurnoDTO t = turno("Mañana", QTime(7, 0), QTime(13, 0), 6);
        const EventoDTO receso = evento("Receso", 2, 1, 10, QTime(8, 50), QTime(9, 0));
        t.eventos.append(receso);

        const int estimada = duracionFranjaEstimada(t);
        QCOMPARE(eventoDentroDeVentana(receso, t, 2),
                 eventoDentroDeVentana(receso, t, 2, estimada));
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
