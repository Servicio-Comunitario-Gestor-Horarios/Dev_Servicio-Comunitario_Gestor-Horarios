#pragma once

/**
 * @file proyeccion_preferencias.hpp
 * @brief Rejilla compartida y proyección reloj → franja de las preferencias (RF-3 de la 013).
 *
 * Puro: sin E/S, sin reloj y sin OR-Tools. La rejilla —el inicio de cada turno y la duración
 * de la hora académica— es la MISMA para la disponibilidad y para las preferencias, así que
 * `slotsDeFranja` solo se queda con los bloques que caben ENTEROS dentro de un rango de reloj,
 * recorre todos los turnos del día y no se corta por los eventos del turno.
 */

#include <QString>
#include <QTime>
#include <QVector>

#include "backend/data/franja_horaria.hpp"
#include "backend/services/ServicioTurnosEventos.hpp"
#include "backend/services/preferencias.hpp"
#include "backend/solver/config/solver_config.hpp"

struct DatosDominio;

/// Turno del dominio con su offset absoluto de slots dentro del día.
struct TurnoProyectado {
    QString nombre;
    QTime   inicio;
    QTime   fin;
    int     numSlots = 0;
    int     offset = 0;
};

/// Ordena los turnos por hora de inicio y calcula el offset de slots de cada uno: los slots
/// de un turno continúan donde terminan los del anterior.
QVector<TurnoProyectado> ordenarTurnos(const QVector<TurnoDTO>& turnos);

/// Slots absolutos del día cubiertos por `franja`.
///
/// Cada bloque se ancla en el `inicio` de su turno y avanza en pasos de `duracionMinutos`; solo
/// se emite si cae ÍNTEGRAMENTE dentro del rango (`[inicio, fin) ⊆ [franja.inicio, franja.fin)`).
/// Recorre TODOS los turnos (una franja puede abarcar mañana y tarde), no recorta el rango entre
/// turnos y los eventos no desplazan ni cortan la rejilla. Es la misma semántica de la
/// disponibilidad: la franja contiene un bloque completo.
QVector<int> slotsDeFranja(const FranjaHoraria& franja,
                           const QVector<TurnoProyectado>& turnos,
                           int duracionMinutos);

/// Motivo por el que una preferencia no interviene (`Ninguno` si interviene).
enum class MotivoPreferencia {
    Ninguno,
    DocenteNoParticipa,   ///< (c) no es candidato elegible de ninguna materia-curso
    DiaFueraDeSemana,     ///< (a) día fuera de la semana declarada
    SinFranjaDelTurno,    ///< (b) el rango no alcanza ninguna franja del turno de un curso suyo
};

/// Texto del motivo en español (vacío para `Ninguno`), para informar la preferencia no aplicada.
QString textoDeMotivo(MotivoPreferencia motivo);

/// Preferencia ya proyectada: docente por índice, día de la GENERACIÓN y franjas alcanzadas.
struct PreferenciaProyectada {
    int                docente = -1;
    int                dia = -1;   ///< 0 = lunes … num_dias-1
    QVector<int>       slot;       ///< slots absolutos del día, ascendentes y sin repetir
    SentidoPreferencia sentido = SentidoPreferencia::Preferida;
};

/// Preferencia que no interviene: conserva la franja de DOMINIO (0 = domingo … 6 = sábado) y
/// su ÚNICO motivo, el primero que aplica en la precedencia (c) > (a) > (b).
struct PreferenciaNoAplicada {
    int                docente = -1;
    FranjaHoraria      franja;
    SentidoPreferencia sentido = SentidoPreferencia::Preferida;
    MotivoPreferencia  motivo = MotivoPreferencia::Ninguno;
};

struct ProyeccionPreferencias {
    QVector<PreferenciaProyectada> intervinientes;
    QVector<PreferenciaNoAplicada> noAplicadas;
};

/// Proyecta las preferencias de `datos` sobre la rejilla y decide cuáles intervienen (RF-3).
///
/// `config` es la configuración PARCIAL de la generación —cursos, planes, materias, turnos y
/// docentes con su disponibilidad ya proyectada—, sin la sección de preferencias. El docente
/// `i` de `datos` se corresponde con `config.profesores[i]` (así los construye la entrada).
/// «Participa» reutiliza `docenteCandidatoElegible` (la noción de V13) y los motivos tienen
/// precedencia **(c) no participa > (a) día fuera de la semana > (b) sin franja del turno de un
/// curso donde el docente es candidato elegible**. Los intervinientes salen en orden canónico
/// `(docente, día, sentido, franjas)`; las no aplicadas, en el orden de los docentes y, dentro,
/// en el de la preferencia en los datos.
ProyeccionPreferencias proyectarPreferencias(const DatosDominio& datos,
                                             const SolverConfig& config,
                                             int duracionMinutos);
