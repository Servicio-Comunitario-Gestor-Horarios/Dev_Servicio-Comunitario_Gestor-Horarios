#include "backend/services/preferencias_cumplimiento.hpp"

#include <algorithm>

namespace {

/// Clases del docente `docente` que caen en el día `dia` y en alguna de las franjas `slots`.
///
/// La preferencia solo se aplica a **las clases del propio docente** (RF-1): las de otro docente
/// en la misma franja no cuentan. Cada clase se informa con el curso (`anio|seccion`), la materia
/// y la franja.
QVector<ClaseIncumplida> clasesEnFranjas(const HorarioSalida& horario, int docente, int dia,
                                         const QVector<int>& franjas)
{
    QVector<ClaseIncumplida> clases;
    for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd(); ++it) {
        for (const DiaOutput& diaOutput : it.value().dias) {
            if (diaOutput.dia != dia)
                continue;
            for (const AsignacionOutput& asignacion : diaOutput.asignaciones) {
                if (asignacion.profesor != docente)
                    continue;
                if (!franjas.contains(asignacion.slot))
                    continue;
                clases.append(ClaseIncumplida{it.key(), asignacion.materia, asignacion.slot});
            }
        }
    }

    // Orden determinista de la lista de clases (RNF-1): curso, franja y materia.
    std::sort(clases.begin(), clases.end(),
              [](const ClaseIncumplida& a, const ClaseIncumplida& b) {
                  if (a.curso != b.curso) return a.curso < b.curso;
                  if (a.slot != b.slot) return a.slot < b.slot;
                  return a.materia < b.materia;
              });
    return clases;
}

} // namespace

CumplimientoPreferencias analizarPreferencias(const SolverConfig& config,
                                              const HorarioSalida& horario,
                                              const QVector<PreferenciaNoAplicada>& noAplicadas)
{
    CumplimientoPreferencias cumplimiento;

    // 1. No aplicadas: llegan de la construcción con la franja de DOMINIO y un solo motivo.
    for (const PreferenciaNoAplicada& noAplicada : noAplicadas) {
        PreferenciaEstado estado;
        estado.docente = noAplicada.docente;
        estado.dia     = noAplicada.franja.dia;   // ya está en numeración de dominio
        estado.sentido = textoDeSentido(noAplicada.sentido);
        estado.estado  = EstadoPreferencia::NoAplicada;
        estado.motivo  = noAplicada.motivo;
        cumplimiento.porPreferencia.append(estado);
    }

    // 2. Intervinientes: el estado sale del horario, sobre sus franjas ya proyectadas.
    for (const PreferenciaSolverConfig& preferencia : config.preferencias) {
        PreferenciaEstado estado;
        estado.docente = preferencia.docente;
        estado.dia     = preferencia.dia + 1;   // generación 0 = lunes → dominio 1 = lunes
        estado.slot    = preferencia.slot;
        estado.sentido = preferencia.sentido;
        estado.motivo  = MotivoPreferencia::Ninguno;   // solo las no aplicadas llevan motivo

        const QVector<ClaseIncumplida> clases =
            clasesEnFranjas(horario, preferencia.docente, preferencia.dia, preferencia.slot);

        if (preferencia.sentido == QLatin1String("evitada")) {
            // Evitada: se incumple si al menos una de sus franjas lleva una clase de su docente.
            if (clases.isEmpty()) {
                estado.estado = EstadoPreferencia::Respetada;
            } else {
                estado.estado = EstadoPreferencia::Incumplida;
                estado.clasesIncumplidas = clases;
            }
        } else {
            // Preferida: se cumple si alguna de sus franjas alcanzables lleva una clase suya.
            estado.estado = clases.isEmpty() ? EstadoPreferencia::Incumplida
                                             : EstadoPreferencia::Respetada;
        }

        cumplimiento.porPreferencia.append(estado);
    }

    // 3. Orden canónico: no depende del orden de declaración (RNF-2).
    std::sort(cumplimiento.porPreferencia.begin(), cumplimiento.porPreferencia.end(),
              [](const PreferenciaEstado& a, const PreferenciaEstado& b) {
                  if (a.docente != b.docente) return a.docente < b.docente;
                  if (a.dia != b.dia) return a.dia < b.dia;
                  if (a.sentido != b.sentido) return a.sentido < b.sentido;
                  return a.slot < b.slot;
              });

    // 4. Agregado: los tres totales.
    for (const PreferenciaEstado& estado : cumplimiento.porPreferencia) {
        switch (estado.estado) {
        case EstadoPreferencia::Respetada:
            ++cumplimiento.respetadas;
            break;
        case EstadoPreferencia::Incumplida:
            ++cumplimiento.incumplidas;
            break;
        case EstadoPreferencia::NoAplicada:
            ++cumplimiento.noAplicadas;
            break;
        }
    }

    return cumplimiento;
}
