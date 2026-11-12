#include "backend/services/proyeccion_preferencias.hpp"

#include "backend/services/ConstructorEntradaSolver.hpp"
#include "backend/solver/config/contrato_solver.hpp"

#include <algorithm>

QVector<TurnoProyectado> ordenarTurnos(const QVector<TurnoDTO>& turnos)
{
    QVector<TurnoProyectado> ordenados;
    ordenados.reserve(turnos.size());
    for (const TurnoDTO& t : turnos)
        ordenados.append(TurnoProyectado{t.nombre, t.inicio, t.fin, t.numSlots, 0});

    std::sort(ordenados.begin(), ordenados.end(),
              [](const TurnoProyectado& a, const TurnoProyectado& b) {
                  return a.inicio < b.inicio;
              });

    int offset = 0;
    for (TurnoProyectado& t : ordenados) {
        t.offset = offset;
        offset += t.numSlots;
    }
    return ordenados;
}

QVector<int> slotsDeFranja(const FranjaHoraria& franja,
                           const QVector<TurnoProyectado>& turnos,
                           int duracionMinutos)
{
    QVector<int> listaSlots;
    if (duracionMinutos <= 0 || !franja.inicio.isValid() || !franja.fin.isValid())
        return listaSlots;

    const int paso = duracionMinutos * 60;
    for (const TurnoProyectado& t : turnos) {
        for (int k = 0; k < t.numSlots; ++k) {
            const QTime bloqueInicio = t.inicio.addSecs(k * paso);
            const QTime bloqueFin    = bloqueInicio.addSecs(paso);
            if (bloqueInicio >= franja.inicio && bloqueFin <= franja.fin)
                listaSlots.append(t.offset + k);
        }
    }
    return listaSlots;
}

QString textoDeMotivo(MotivoPreferencia motivo)
{
    switch (motivo) {
    case MotivoPreferencia::Ninguno:
        return QString();
    case MotivoPreferencia::DocenteNoParticipa:
        return QStringLiteral(
            "el docente no participa en la generación: no es candidato elegible de ninguna "
            "materia-curso");
    case MotivoPreferencia::DiaFueraDeSemana:
        return QStringLiteral("el día queda fuera de la semana declarada de la generación");
    case MotivoPreferencia::SinFranjaDelTurno:
        return QStringLiteral(
            "el rango no alcanza ninguna franja del turno de un curso donde el docente es "
            "candidato elegible");
    }
    return QString();
}

namespace {

/// Slots del turno de un curso, sin repetir, para la unión de turnos alcanzables.
void acumularSlots(const QVector<int>& slotsDelTurno, QVector<int>& unionSlots)
{
    for (int slot : slotsDelTurno)
        if (!unionSlots.contains(slot))
            unionSlots.append(slot);
}

/// ¿La preferencia interviene en este día? `false` si el día de dominio queda fuera de la
/// semana declarada (domingo nunca aplica; el sábado, solo si `num_dias` lo incluye).
bool diaEnLaSemana(int diaDominio, int numDias)
{
    const int diaGeneracion = diaDominio - 1;   // dominio 0=domingo → generación 0=lunes
    return diaDominio > 0 && diaDominio <= 6 && diaGeneracion < numDias;
}

} // namespace

ProyeccionPreferencias proyectarPreferencias(const DatosDominio& datos,
                                             const SolverConfig& config,
                                             int duracionMinutos)
{
    ProyeccionPreferencias proyeccion;
    const QVector<TurnoProyectado> turnos = ordenarTurnos(datos.turnos);
    const int numDias = config.dimensiones.num_dias;

    // El índice del docente es el del arreglo de la configuración (= el de `datos.docentes`,
    // porque la entrada se construye en ese orden).
    const int numDocentes =
        static_cast<int>(qMin(datos.docentes.size(), config.profesores.size()));

    for (int i = 0; i < numDocentes; ++i) {
        const ProfesorDTO& docente = datos.docentes[i];
        if (docente.preferencias.isEmpty())
            continue;

        const ProfesorSolverConfig& prof = config.profesores[i];

        // (c) Participa si es candidato elegible de al menos una materia-curso (V13). Las
        // franjas alcanzables son las de los turnos de esos cursos: una preferencia de la tarde
        // no aplica a un docente que solo es candidato en cursos de la mañana.
        bool participa = false;
        QVector<int> slotsPosibles;
        for (const CursoSolverConfig& curso : config.cursos) {
            for (const PlanMateriaConfig& materiaCurso : materiasDelCurso(config, curso)) {
                if (!docenteCandidatoElegible(config, curso, prof, materiaCurso.materiaIDx))
                    continue;
                participa = true;
                acumularSlots(slotsTurno(config, curso.turno), slotsPosibles);
            }
        }

        for (const PreferenciaDTO& preferencia : docente.preferencias) {
            PreferenciaNoAplicada noAplicada;
            noAplicada.docente = i;
            noAplicada.franja  = preferencia.franja;
            noAplicada.sentido = preferencia.sentido;

            if (!participa) {
                noAplicada.motivo = MotivoPreferencia::DocenteNoParticipa;
                proyeccion.noAplicadas.append(noAplicada);
                continue;
            }

            if (!diaEnLaSemana(preferencia.franja.dia, numDias)) {
                noAplicada.motivo = MotivoPreferencia::DiaFueraDeSemana;
                proyeccion.noAplicadas.append(noAplicada);
                continue;
            }

            QVector<int> alcanzables;
            for (int slot : slotsDeFranja(preferencia.franja, turnos, duracionMinutos))
                if (slotsPosibles.contains(slot) && !alcanzables.contains(slot))
                    alcanzables.append(slot);

            if (alcanzables.isEmpty()) {
                noAplicada.motivo = MotivoPreferencia::SinFranjaDelTurno;
                proyeccion.noAplicadas.append(noAplicada);
                continue;
            }

            std::sort(alcanzables.begin(), alcanzables.end());

            PreferenciaProyectada interviniente;
            interviniente.docente = i;
            interviniente.dia     = preferencia.franja.dia - 1;
            interviniente.slot    = alcanzables;
            interviniente.sentido = preferencia.sentido;
            proyeccion.intervinientes.append(interviniente);
        }
    }

    // Orden canónico: no depende del orden de declaración (RNF-2).
    std::sort(proyeccion.intervinientes.begin(), proyeccion.intervinientes.end(),
              [](const PreferenciaProyectada& a, const PreferenciaProyectada& b) {
                  if (a.docente != b.docente) return a.docente < b.docente;
                  if (a.dia != b.dia) return a.dia < b.dia;
                  if (a.sentido != b.sentido)
                      return static_cast<int>(a.sentido) < static_cast<int>(b.sentido);
                  return a.slot < b.slot;
              });

    return proyeccion;
}
