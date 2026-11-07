#include "backend/services/ValidadorSalidaSolver.hpp"

#include "backend/solver/config/bloques.hpp"
#include "backend/solver/config/contrato_solver.hpp"

#include <algorithm>
#include <QMap>
#include <QSet>

namespace
{
    /// Índice en `config.cursos` del curso con esa clave (`año|sección`), o -1.
    int indiceCurso(const SolverConfig& config, const QString& clave)
    {
        for (int i = 0; i < config.cursos.size(); ++i)
            if (claveCurso(config.cursos[i].anio, config.cursos[i].seccion) == clave)
                return i;
        return -1;
    }

    /// P4 ampliado (014, RF-12): bloque de 1, materia mal alineada o dos aulas en un bloque.
    void comprobarBloques(const HorarioSalida& horario, const SolverConfig& config,
                          AnalisisSalida& analisis)
    {
        for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd(); ++it)
        {
            const QString& clave = it.key();
            const int curso = indiceCurso(config, clave);
            if (curso < 0) continue;

            QSet<FranjaSemana> ocupadas;
            QMap<int, QSet<FranjaSemana>> porMateria;
            QMap<FranjaSemana, int> materiaPorFranja;
            QMap<FranjaSemana, int> aulaPorFranja;
            for (const DiaOutput& dia : it.value().dias)
            {
                for (const AsignacionOutput& asignacion : dia.asignaciones)
                {
                    const FranjaSemana franja{dia.dia, asignacion.slot};
                    ocupadas.insert(franja);
                    porMateria[asignacion.materia].insert(franja);
                    materiaPorFranja.insert(franja, asignacion.materia);
                    aulaPorFranja.insert(franja, asignacion.aula);
                }
            }

            // Bloque de 1: franjas ocupadas que no forman par.
            const int sueltas = rejilla::bloquesDeUno(config, curso, ocupadas);
            if (sueltas > 0)
                analisis.conflictos.append(
                    QStringLiteral("El curso «%1» tiene %2 franjas sin bloque de dos horas.")
                        .arg(clave)
                        .arg(sueltas));

            // Materia mal alineada: más franjas descubiertas que su resto.
            for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
            {
                const int materia = mc.materiaIDx;
                const int horas =
                    bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);
                const int descubiertas = rejilla::franjasDescubiertasDeMateria(
                    config, curso, porMateria.value(materia));
                if (descubiertas > horas % 2)
                {
                    const QString nombre =
                        (materia >= 0 && materia < config.materias.size())
                            ? config.materias[materia].nombre
                            : QString::number(materia);
                    analisis.conflictos.append(
                        QStringLiteral("La materia «%1» del curso «%2» tiene %3 franjas "
                                       "descubiertas y su resto es %4.")
                            .arg(nombre)
                            .arg(clave)
                            .arg(descubiertas)
                            .arg(horas % 2));
                }
            }

            // Hueco que no es sufijo: una franja libre con clase después en el mismo día.
            QVector<int> franjasTurno = slotsTurno(config, config.cursos[curso].turno);
            std::sort(franjasTurno.begin(), franjasTurno.end());
            for (int dia = 0; dia < config.dimensiones.num_dias; ++dia)
            {
                bool hayHueco = false;
                for (int slot : franjasTurno)
                {
                    const bool ocupada = ocupadas.contains(FranjaSemana{dia, slot});
                    if (!ocupada)
                        hayHueco = true;
                    else if (hayHueco)
                    {
                        analisis.conflictos.append(
                            QStringLiteral("El curso «%1» tiene un hueco que no es sufijo el día %2 "
                                           "(franja %3).")
                                .arg(clave)
                                .arg(dia)
                                .arg(slot));
                        break;
                    }
                }
            }

            // Dos aulas distintas en un mismo bloque.
            for (const rejilla::Par& par : rejilla::paresDeCurso(config, curso, ocupadas))
            {
                const int aulaA = aulaPorFranja.value(FranjaSemana{par.dia, par.inicio}, -1);
                const int aulaB = aulaPorFranja.value(FranjaSemana{par.dia, par.inicio + 1}, -1);
                if (aulaA >= 0 && aulaB >= 0 && aulaA != aulaB)
                    analisis.conflictos.append(
                        QStringLiteral("El bloque del curso «%1» (día %2, franjas %3-%4) usa "
                                       "dos aulas distintas.")
                            .arg(clave)
                            .arg(par.dia)
                            .arg(par.inicio)
                            .arg(par.inicio + 1));
            }
        }
    }

    /// Clave de ocupación de P4: un recurso (docente o aula) ocupado en una franja concreta.
    ///
    /// P4 debe distinguir el recurso: dos clases en la misma franja solo chocan si comparten
    /// docente o aula, no por coincidir en el día y el slot.
    struct ClaveOcupacion
    {
        int recurso = -1;  ///< índice de profesor o de aula
        int dia = -1;
        int slot = -1;

        bool operator<(const ClaveOcupacion& otro) const
        {
            if (recurso != otro.recurso)
                return recurso < otro.recurso;
            if (dia != otro.dia)
                return dia < otro.dia;
            return slot < otro.slot;
        }
    };

} // namespace

AnalisisSalida analizarSalidaSolver(const HorarioSalida& horario, const SolverConfig& config)
{
    AnalisisSalida analisis;

    // Ocupación de cada recurso en cada franja para detectar solapamientos (P4): las claves
    // incluyen el docente / el aula, no solo el día y el slot.
    QMap<ClaveOcupacion, QString> ocupacionProfesor;  // -> nombre de curso
    QMap<ClaveOcupacion, QString> ocupacionAula;

    // Conteo para P2: índice de profesor -> asignaciones.
    QMap<int, int> horasPorProfesor;

    for (auto itCurso = horario.horarios.constBegin(); itCurso != horario.horarios.constEnd();
         ++itCurso)
    {
        const QString& nombreCurso = itCurso.key();
        const CursoOutput& curso = itCurso.value();

        for (const DiaOutput& dia : curso.dias)
        {
            QSet<int> slotsDelCurso;  // un curso no puede tener dos clases a la vez

            for (const AsignacionOutput& asig : dia.asignaciones)
            {
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

                // P4: profesor en dos sitios a la vez. Un docente sin asignar (índice
                // negativo) no ocupa la franja: dos clases sin docente no chocan entre sí.
                if (asig.profesor >= 0)
                {
                    const ClaveOcupacion clave{asig.profesor, dia.dia, asig.slot};
                    if (ocupacionProfesor.contains(clave))
                    {
                        const int idx = asig.profesor;
                        const QString nombreProf =
                            (idx < config.profesores.size())
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
                }

                // P4: aula en dos sitios a la vez. Un aula sin asignar (índice negativo) no
                // ocupa la franja. Un índice positivo fuera de rango se indexa tal cual: no
                // choca con las aulas del catálogo, pero dos clases que informan la misma
                // aula inexistente sí colisionan y se nombran como desconocida.
                if (asig.aula >= 0)
                {
                    const ClaveOcupacion clave{asig.aula, dia.dia, asig.slot};
                    if (ocupacionAula.contains(clave))
                    {
                        const int idx = asig.aula;
                        const QString nombreAula =
                            (idx < config.aulas.size())
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
                }

                // Conteo P2.
                if (asig.profesor >= 0)
                    horasPorProfesor[asig.profesor] += 1;
            }
        }
    }

    // P4 ampliado (014): bloque de 1, materia mal alineada o dos aulas en un bloque.
    comprobarBloques(horario, config, analisis);

    // P2: ningún profesor excede sus horas requeridas. Es el único aviso que queda (005 I2):
    // las horas no cubiertas (P1) y la capacidad de aula (P3) se retiraron.
    for (auto it = horasPorProfesor.constBegin(); it != horasPorProfesor.constEnd(); ++it)
    {
        const int idx = it.key();
        if (idx < 0 || idx >= config.profesores.size())
            continue;
        const int requeridas = config.profesores[idx].horas_requeridas;
        if (requeridas != -1 && it.value() > requeridas)   // -1 = horas_requeridas ausente
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
