#include "backend/services/ValidadorSalidaSolver.hpp"

#include <QMap>
#include <QPair>
#include <QSet>

namespace
{
    /// Índice del curso de `config` con ese nombre, o -1.
    int indiceCurso(const SolverConfig& config, const QString& nombre)
    {
        for (int i = 0; i < config.cursos.size(); ++i)
            if (config.cursos[i].nombre == nombre)
                return i;
        return -1;
    }

    /// Horas semanales esperadas de `materia` en el curso de `config`, o -1.
    int horasEsperadas(const SolverConfig& config, int idxCurso, int materia)
    {
        if (idxCurso < 0)
            return -1;
        for (const MateriaCurso& mc : config.cursos[idxCurso].materias)
            if (mc.materiaIDx == materia)
                return mc.horasSemanales;
        return -1;
    }
} // namespace

AnalisisSalida analizarSalidaSolver(const HorarioSalida& horario, const SolverConfig& config)
{
    AnalisisSalida analisis;

    // Ocupación por (día, slot) para detectar solapamientos (P4).
    QMap<QPair<int, int>, QString> ocupacionProfesor;  // -> nombre de curso
    QMap<QPair<int, int>, QString> ocupacionAula;

    // Conteos para P1 y P2.
    // (curso, materia) -> nº de asignaciones
    QMap<QPair<QString, int>, int> cubiertas;
    QMap<int, int> horasPorProfesor;   // índice de profesor -> asignaciones

    for (auto itCurso = horario.horarios.constBegin(); itCurso != horario.horarios.constEnd();
         ++itCurso)
    {
        const QString& nombreCurso = itCurso.key();
        const CursoOutput& curso = itCurso.value();
        const int idxCurso = indiceCurso(config, nombreCurso);

        for (const DiaOutput& dia : curso.dias)
        {
            QSet<int> slotsDelCurso;  // un curso no puede tener dos clases a la vez

            for (const AsignacionOutput& asig : dia.asignaciones)
            {
                const QPair<int, int> clave{dia.dia, asig.slot};

                // P4: el mismo curso, dos veces en el mismo slot.
                if (slotsDelCurso.contains(asig.slot))
                {
                    analisis.conflictos.append(
                        QStringLiteral("El curso «%1» tiene dos clases en el día %2, slot %3.")
                            .arg(nombreCurso)
                            .arg(dia.dia)
                            .arg(asig.slot));
                }
                slotsDelCurso.insert(asig.slot);

                // P4: profesor en dos sitios a la vez.
                if (ocupacionProfesor.contains(clave))
                {
                    const int idx = asig.profesor;
                    const QString nombreProf =
                        (idx >= 0 && idx < config.profesores.size())
                            ? config.profesores[idx].nombre
                            : QStringLiteral("(desconocido)");
                    analisis.conflictos.append(
                        QStringLiteral("El docente «%1» está en dos clases a la vez (día %2, slot %3).")
                            .arg(nombreProf)
                            .arg(dia.dia)
                            .arg(asig.slot));
                }
                else
                {
                    ocupacionProfesor[clave] = nombreCurso;
                }

                // P4: aula en dos sitios a la vez.
                if (ocupacionAula.contains(clave))
                {
                    const int idx = asig.aula;
                    const QString nombreAula =
                        (idx >= 0 && idx < config.aulas.size())
                            ? config.aulas[idx].nombre
                            : QStringLiteral("(desconocida)");
                    analisis.conflictos.append(
                        QStringLiteral("El aula «%1» está ocupada por dos clases a la vez (día %2, slot %3).")
                            .arg(nombreAula)
                            .arg(dia.dia)
                            .arg(asig.slot));
                }
                else
                {
                    ocupacionAula[clave] = nombreCurso;
                }

                // Conteos P1/P2.
                cubiertas[{nombreCurso, asig.materia}] += 1;
                if (asig.profesor >= 0)
                    horasPorProfesor[asig.profesor] += 1;

                // P3: capacidad de aula.
                if (idxCurso >= 0 && asig.aula >= 0 && asig.aula < config.aulas.size())
                {
                    const int estudiantes = config.cursos[idxCurso].num_estudiantes;
                    const int capacidad = config.aulas[asig.aula].capacidad;
                    if (capacidad > 0 && estudiantes > capacidad)
                    {
                        const QString aviso =
                            QStringLiteral("El curso «%1» (%2 estudiantes) excede la capacidad del aula «%3» (%4).")
                                .arg(nombreCurso)
                                .arg(estudiantes)
                                .arg(config.aulas[asig.aula].nombre)
                                .arg(capacidad);
                        if (!analisis.avisos.contains(aviso))
                            analisis.avisos.append(aviso);
                    }
                }
            }
        }
    }

    // P1: horas semanales cubiertas por curso/materia.
    for (auto itCurso = horario.horarios.constBegin(); itCurso != horario.horarios.constEnd();
         ++itCurso)
    {
        const QString& nombreCurso = itCurso.key();
        const int idxCurso = indiceCurso(config, nombreCurso);
        if (idxCurso < 0)
            continue;

        for (const MateriaCurso& mc : config.cursos[idxCurso].materias)
        {
            const int esperadas = mc.horasSemanales;
            const int cubiertasActual = cubiertas.value({nombreCurso, mc.materiaIDx}, 0);
            if (cubiertasActual < esperadas)
            {
                const QString nombreMateria =
                    (mc.materiaIDx >= 0 && mc.materiaIDx < config.materias.size())
                        ? config.materias[mc.materiaIDx].nombre
                        : QString::number(mc.materiaIDx);
                analisis.avisos.append(
                    QStringLiteral("Horas no cubiertas: «%1» del curso «%2» tiene %3 de %4 horas.")
                        .arg(nombreMateria)
                        .arg(nombreCurso)
                        .arg(cubiertasActual)
                        .arg(esperadas));
            }
        }
    }

    // P2: ningún profesor excede sus horas requeridas.
    for (auto it = horasPorProfesor.constBegin(); it != horasPorProfesor.constEnd(); ++it)
    {
        const int idx = it.key();
        if (idx < 0 || idx >= config.profesores.size())
            continue;
        const int requeridas = config.profesores[idx].horas_requeridas;
        if (requeridas > 0 && it.value() > requeridas)
        {
            analisis.avisos.append(
                QStringLiteral("Exceso de horas: el docente «%1» tiene %2 de %3 horas.")
                    .arg(config.profesores[idx].nombre)
                    .arg(it.value())
                    .arg(requeridas));
        }
    }

    return analisis;
}
