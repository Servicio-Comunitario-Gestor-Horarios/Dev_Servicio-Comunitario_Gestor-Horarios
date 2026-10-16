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
