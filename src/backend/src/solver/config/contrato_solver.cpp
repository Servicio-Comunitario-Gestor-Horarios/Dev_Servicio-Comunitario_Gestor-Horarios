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

QVector<QVector<int>> segmentosTurno(const TurnoConfig& turno, const QVector<RecesoConfig>& recesos)
{
    // 1. Obtener recesos de este turno
    QVector<RecesoConfig> recesosTurno;
    for (const RecesoConfig& r : recesos)
        if (r.turno == turno.nombre)
            recesosTurno.append(r);

    // 2. Ordenar recesos por despues_de_slot
    std::sort(recesosTurno.begin(), recesosTurno.end(),
              [](const RecesoConfig& a, const RecesoConfig& b) {
                  return a.despues_de_slot < b.despues_de_slot;
              });

    // 3. Cortar slots por recesos
    // El campo `despues_de_slot` en RecesoConfig es un índice ABSOLUTO del día (0-15).
    // Hay que convertirlo a índice RELATIVO dentro del array de slots del turno.
    if (turno.slot.isEmpty()) {
        return {};
    }
    int slotBase = turno.slot.first();  // primer slot absoluto del turno

    QVector<QVector<int>> segmentos;
    int inicioRel = 0;  // índice relativo dentro de turno.slot
    for (const RecesoConfig& r : recesosTurno) {
        // Convertir despues_de_slot absoluto a índice relativo
        int corteAbs = r.despues_de_slot;
        int corteRel = corteAbs - slotBase;
        if (corteRel >= 0 && corteRel < turno.slot.size() && inicioRel <= corteRel) {
            QVector<int> seg;
            for (int i = inicioRel; i <= corteRel; ++i)
                seg.append(turno.slot[i]);
            if (!seg.isEmpty()) segmentos.append(seg);
            inicioRel = corteRel + 1;
        }
    }
    // Último segmento
    if (inicioRel < turno.slot.size()) {
        QVector<int> seg;
        for (int i = inicioRel; i < turno.slot.size(); ++i)
            seg.append(turno.slot[i]);
        if (!seg.isEmpty()) segmentos.append(seg);
    }
    // Si no hay recesos, devolver todos los slots
    if (segmentos.isEmpty() && !turno.slot.isEmpty()) {
        segmentos.append(turno.slot);
    }
    return segmentos;
}

bool esInicioSegmento(int slot, const QVector<QVector<int>>& segmentos)
{
    for (const QVector<int>& seg : segmentos)
        if (!seg.isEmpty() && seg.first() == slot)
            return true;
    return false;
}

int calcularPenalizacionGrupo(int tamano, int horasDia)
{
    if (tamano <= 0) return 0;
    if (tamano == 1) return (horasDia == 1) ? 0 : 1;  // D-4
    if (tamano == 2 || tamano == 3) return 0;
    return tamano - 3;  // ≥ 4
}
