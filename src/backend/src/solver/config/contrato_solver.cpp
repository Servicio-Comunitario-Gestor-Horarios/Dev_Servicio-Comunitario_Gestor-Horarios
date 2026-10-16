#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/config/solver_config.hpp"

namespace {
    int bloquesConDuracion(int horas, int duracion_minutos) {
        
        if (duracion_minutos <= 0) {
            return 0;
        }
        
        const int minutos = horas * 60;
        return (minutos + duracion_minutos - 1) / duracion_minutos;
    }
}

int bloques(int horas, int duracion_minutos) {
    return bloquesConDuracion(horas, duracion_minutos);
}

int bloquesAula(int horas_aula, int duracion_minutos) {
    return bloquesConDuracion(horas_aula, duracion_minutos);
}

QString claveCurso(int anio, const QString& seccion) {
    return QStringLiteral("%1|%2").arg(anio).arg(seccion);
}

QVector<PlanMateriaConfig> materiasDelCurso(const SolverConfig &config, const CursoSolverConfig &curso){
    for (const PlanSolverConfig& plan : config.planes) {
        if (plan.codigo != curso.plan) {
            continue;
        }

        QVector<PlanMateriaConfig> materias;
        for (const PlanMateriaConfig& m : plan.materias) {
            if (m.curso == curso.anio) {
                materias.append(m);
            }
        }
        return materias;
    }
    return {};
}

namespace {
    /// Slots declarados por el turno `nombre`, o vacío si no existe.
    QVector<int> slotsDelTurno(const SolverConfig& config, const QString& nombre) {
        for (const TurnoConfig& t : config.turnos.turnos)
            if (t.nombre == nombre)
                return t.slot;
        return {};
    }
}

bool docenteOptaPorMateria(const ProfesorSolverConfig& prof, int materiaIDx) {
    if (prof.materias_asignadas.contains(materiaIDx)) return true;
    for (const MateriaSuplente& s : prof.materias_suplente)
        if (s.materiaIDx == materiaIDx) return true;
    return false;
}

QVector<int> slotsDisponibles(const SolverConfig& config, const CursoSolverConfig& curso,
                              const ProfesorSolverConfig& prof, int dia) {
    QVector<int> disponibles;
    const QVector<int> slotsCurso = slotsDelTurno(config, curso.turno);          // D11
    const QVector<int> slotsProfesor =
        prof.turno.isEmpty() ? QVector<int>() : slotsDelTurno(config, prof.turno);

    QVector<int> dispDia;
    for (const Disponibilidad& disp : prof.disponibilidad)
        if (disp.dia == dia) { dispDia = disp.slot; break; }

    for (int s : slotsCurso) {
        if (!dispDia.contains(s)) continue;                                     // disponibilidad
        if (!prof.turno.isEmpty() && !slotsProfesor.contains(s)) continue;      // D6
        disponibles.append(s);
    }
    return disponibles;
}

bool docenteCandidatoElegible(const SolverConfig& config, const CursoSolverConfig& curso,
                              const ProfesorSolverConfig& prof, int materiaIDx) {
    if (!docenteOptaPorMateria(prof, materiaIDx)) return false;                 // titular o suplente
    if (!prof.plan.isEmpty() && prof.plan != curso.plan) return false;          // D7
    if (!prof.anios.isEmpty() && !prof.anios.contains(curso.anio)) return false; // D13
    for (int d = 0; d < config.dimensiones.num_dias; ++d)
        if (!slotsDisponibles(config, curso, prof, d).isEmpty()) return true;
    return false;
}
