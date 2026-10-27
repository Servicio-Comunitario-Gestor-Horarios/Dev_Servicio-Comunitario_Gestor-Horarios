#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/config/solver_config.hpp"

namespace {
    int bloquesConDuracion(int horas, int duracion_minutos) {
        // Rectificación (2026-10-17, constitución §7): la hora académica es un bloque, no una
        // hora de reloj. La conversión es la IDENTIDAD: bloques(horas) = horas, sin multiplicar
        // por 60 ni dividir por `duracion_minutos`. `duracion_minutos` define solo la duración
        // real de la franja (rejilla horaria, receso y disponibilidad) y NO interviene en la
        // conversión. Se conserva en la firma por estabilidad de los call sites y como guarda
        // defensiva: `0` si `duracion_minutos <= 0` (V2 ya rechaza ese caso antes de resolver).
        if (duracion_minutos <= 0) {
            return 0;
        }
        return horas;
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

QString claveTipo(const QString& nombre) {
    return nombre.trimmed().toCaseFolded();
}

QString tipoGeneralAula() {
    // Convención fija por nombre (decisión del 2026-10-26): no hay columna, sección de contrato ni
    // consulta a la base. V18 y las cuentas de capacidad la reconocen con `claveTipo`.
    return QStringLiteral("Aula de estudio");
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

QVector<int> slotsTurno(const SolverConfig& config, const QString& turno) {
    for (const TurnoConfig& t : config.turnos.turnos)
        if (t.nombre == turno)
            return t.slot;
    return {};
}

QSet<FranjaSemana> franjasDelCurso(const SolverConfig& config, const CursoSolverConfig& curso) {
    QSet<FranjaSemana> franjas;
    const QVector<int> slotsDelCurso = slotsTurno(config, curso.turno);
    for (int d = 0; d < config.dimensiones.num_dias; ++d)
        for (int s : slotsDelCurso)
            franjas.insert(FranjaSemana{d, s});
    return franjas;
}

int franjasSemanales(const SolverConfig& config, const CursoSolverConfig& curso) {
    return franjasDelCurso(config, curso).size();
}

QSet<FranjaSemana> franjasDelTipo(const SolverConfig& config, const QString& clave) {
    QSet<FranjaSemana> franjas;
    for (const CursoSolverConfig& curso : config.cursos) {
        bool requiereElTipo = false;
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso)) {
            if (mc.materiaIDx < 0 || mc.materiaIDx >= config.materias.size()) continue;
            if (claveTipo(config.materias[mc.materiaIDx].tipo) == clave) {
                requiereElTipo = true;
                break;
            }
        }
        if (requiereElTipo) franjas.unite(franjasDelCurso(config, curso));
    }
    return franjas;
}

int demandaDelTipo(const SolverConfig& config, const QString& clave) {
    int total = 0;
    for (const CursoSolverConfig& curso : config.cursos) {
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso)) {
            if (mc.materiaIDx < 0 || mc.materiaIDx >= config.materias.size()) continue;
            if (claveTipo(config.materias[mc.materiaIDx].tipo) == clave)
                total += bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);
        }
    }
    return total;
}

int aulasDelTipo(const SolverConfig& config, const QString& clave) {
    int aulas = 0;
    for (const AulaSolverConfig& aula : config.aulas)
        if (claveTipo(aula.tipo) == clave) ++aulas;
    return aulas;
}

int capacidadDelTipo(const SolverConfig& config, const QString& clave) {
    return aulasDelTipo(config, clave) * franjasDelTipo(config, clave).size();
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
    const QVector<int> slotsCurso = slotsTurno(config, curso.turno);            // D11
    const QVector<int> slotsProfesor =
        prof.turno.isEmpty() ? QVector<int>() : slotsTurno(config, prof.turno);

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
