#include "backend/services/ServicioConsultaHorario.hpp"

// ─── Constructor ───────────────────────────────────────────────────────────

ServicioConsultaHorario::ServicioConsultaHorario(const HorarioSalida& horario)
    : m_horario(horario) {}

// ─── Helper: recopilar todas las asignaciones con contexto ─────────────────

QVector<AsignacionConContexto> ServicioConsultaHorario::recopilarAsignaciones() const {
    QVector<AsignacionConContexto> resultado;

    for (auto it = m_horario.horarios.constBegin(); it != m_horario.horarios.constEnd(); ++it) {
        const QString& curso = it.key();
        const CursoOutput& cursoOutput = it.value();

        for (const DiaOutput& dia : cursoOutput.dias) {
            for (const AsignacionOutput& asig : dia.asignaciones) {
                AsignacionConContexto ctx;
                ctx.asignacion = asig;
                ctx.curso = curso;
                ctx.dia = dia.dia;
                resultado.append(ctx);
            }
        }
    }

    return resultado;
}

// ─── Consultas por filtro ─────────────────────────────────────────────────

Resultado<QVector<AsignacionConContexto>>
ServicioConsultaHorario::buscarPorProfesor(int profesorIdx) const {
    QVector<AsignacionConContexto> resultados;

    for (const auto& ctx : recopilarAsignaciones()) {
        if (ctx.asignacion.profesor == profesorIdx) {
            resultados.append(ctx);
        }
    }

    return Resultado<QVector<AsignacionConContexto>>::exito(resultados);
}

Resultado<QVector<AsignacionConContexto>>
ServicioConsultaHorario::buscarPorAula(int aulaIdx) const {
    QVector<AsignacionConContexto> resultados;

    for (const auto& ctx : recopilarAsignaciones()) {
        if (ctx.asignacion.aula == aulaIdx) {
            resultados.append(ctx);
        }
    }

    return Resultado<QVector<AsignacionConContexto>>::exito(resultados);
}

Resultado<QVector<AsignacionConContexto>>
ServicioConsultaHorario::buscarPorDia(int dia) const {
    QVector<AsignacionConContexto> resultados;

    for (const auto& ctx : recopilarAsignaciones()) {
        if (ctx.dia == dia) {
            resultados.append(ctx);
        }
    }

    return Resultado<QVector<AsignacionConContexto>>::exito(resultados);
}

Resultado<QVector<AsignacionConContexto>>
ServicioConsultaHorario::buscarPorCurso(const QString& curso) const {
    if (!m_horario.horarios.contains(curso)) {
        return Resultado<QVector<AsignacionConContexto>>::error(
            QString("No se encontró el curso '%1'.").arg(curso));
    }

    QVector<AsignacionConContexto> resultados;
    const CursoOutput& cursoOutput = m_horario.horarios[curso];

    for (const DiaOutput& dia : cursoOutput.dias) {
        for (const AsignacionOutput& asig : dia.asignaciones) {
            AsignacionConContexto ctx;
            ctx.asignacion = asig;
            ctx.curso = curso;
            ctx.dia = dia.dia;
            resultados.append(ctx);
        }
    }

    return Resultado<QVector<AsignacionConContexto>>::exito(resultados);
}

// ─── Detección de conflictos ──────────────────────────────────────────────

Resultado<bool>
ServicioConsultaHorario::hayConflicto(const QString& curso, int dia, int slot) const {
    if (!m_horario.horarios.contains(curso)) {
        return Resultado<bool>::error(
            QString("No se encontró el curso '%1'.").arg(curso));
    }

    // Buscar la asignación especificada
    const CursoOutput& cursoOutput = m_horario.horarios[curso];
    AsignacionOutput objetivo;
    bool encontrado = false;

    for (const DiaOutput& d : cursoOutput.dias) {
        if (d.dia == dia) {
            for (const AsignacionOutput& a : d.asignaciones) {
                if (a.slot == slot) {
                    objetivo = a;
                    encontrado = true;
                    break;
                }
            }
            if (encontrado) break;
        }
    }

    if (!encontrado) {
        return Resultado<bool>::error(
            QString("No se encontró una asignación en curso '%1', día %2, slot %3.")
                .arg(curso).arg(dia).arg(slot));
    }

    // Verificar conflictos con otras asignaciones en el mismo slot/día
    for (const auto& ctx : recopilarAsignaciones()) {
        if (ctx.curso == curso && ctx.dia == dia && ctx.asignacion.slot == slot) {
            continue; // Saltar la misma asignación
        }

        if (ctx.dia == dia && ctx.asignacion.slot == slot) {
            if (ctx.asignacion.profesor == objetivo.profesor) {
                return Resultado<bool>::exito(true);
            }
            if (ctx.asignacion.aula == objetivo.aula) {
                return Resultado<bool>::exito(true);
            }
        }
    }

    return Resultado<bool>::exito(false);
}

Resultado<QVector<ParConflicto>>
ServicioConsultaHorario::detectarConflictos() const {
    QVector<ParConflicto> conflictos;
    QVector<AsignacionConContexto> todas = recopilarAsignaciones();

    // Escaneo O(n²) — suficiente para archivos de salida pequeños
    for (int i = 0; i < todas.size(); ++i) {
        for (int j = i + 1; j < todas.size(); ++j) {
            const auto& a = todas[i];
            const auto& b = todas[j];

            if (a.dia != b.dia || a.asignacion.slot != b.asignacion.slot) {
                continue;
            }

            if (a.asignacion.profesor == b.asignacion.profesor) {
                ParConflicto conflicto;
                conflicto.primera = a;
                conflicto.segunda = b;
                conflicto.tipo = "profesor";
                conflictos.append(conflicto);
            }

            if (a.asignacion.aula == b.asignacion.aula) {
                ParConflicto conflicto;
                conflicto.primera = a;
                conflicto.segunda = b;
                conflicto.tipo = "aula";
                conflictos.append(conflicto);
            }
        }
    }

    return Resultado<QVector<ParConflicto>>::exito(conflictos);
}

// ─── Estadísticas ─────────────────────────────────────────────────────────

Resultado<EstadisticasHorario>
ServicioConsultaHorario::obtenerEstadisticas() const {
    EstadisticasHorario stats;
    QVector<AsignacionConContexto> todas = recopilarAsignaciones();

    stats.total_asignaciones = todas.size();

    for (const auto& ctx : todas) {
        stats.asignaciones_por_profesor[ctx.asignacion.profesor]++;
        stats.asignaciones_por_aula[ctx.asignacion.aula]++;
        stats.asignaciones_por_curso[ctx.curso]++;
        stats.asignaciones_por_dia[ctx.dia]++;
    }

    return Resultado<EstadisticasHorario>::exito(stats);
}
