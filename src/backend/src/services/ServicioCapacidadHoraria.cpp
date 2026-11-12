#include "backend/services/ServicioCapacidadHoraria.hpp"

#include <QHash>
#include <QSet>
#include <algorithm>

#include "backend/services/ServicioCargaHoraria.hpp"
#include "backend/services/ServicioCursos.hpp"
#include "backend/services/ServicioPlanesEstudio.hpp"
#include "backend/services/ServicioProfesor.hpp"

namespace {

/// ¿Va `a` antes que `b` en el orden plan, año, materia e id?
bool antesQue(const HorasPlanAnioMateriaDTO& a, const HorasPlanAnioMateriaDTO& b)
{
    if (a.codigoPlan != b.codigoPlan) {
        return a.codigoPlan < b.codigoPlan;
    }
    if (a.anio != b.anio) {
        return a.anio < b.anio;
    }
    if (a.nombreMateria != b.nombreMateria) {
        return a.nombreMateria < b.nombreMateria;
    }
    return a.idMateria < b.idMateria;
}

} // namespace

ServicioCapacidadHoraria::ServicioCapacidadHoraria(QSqlDatabase& db) : m_db(db) {}

Resultado<QVector<HorasPlanAnioMateriaDTO>>
ServicioCapacidadHoraria::horasPorPlanAnioMateria() const
{
    if (!m_db.isOpen()) {
        return Resultado<QVector<HorasPlanAnioMateriaDTO>>::error(
            QStringLiteral("No se pudieron leer las horas de los planes: la base de datos no está"
                           " abierta."));
    }

    const ServicioPlanesEstudio planes(m_db);
    const auto listado = planes.listarPlanes();
    if (!listado.ok) {
        return Resultado<QVector<HorasPlanAnioMateriaDTO>>::error(listado.mensajeError,
                                                                 listado.codigoError);
    }

    const ServicioCargaHoraria carga(m_db);
    QVector<HorasPlanAnioMateriaDTO> detalle;
    for (const PlanDTO& plan : listado.valor) {
        for (const CargaHorariaDTO& fila : carga.listarCargaPorPlan(plan.codigo)) {
            HorasPlanAnioMateriaDTO dto;
            dto.codigoPlan = plan.codigo;
            dto.nombrePlan = plan.nombre;
            dto.anio = fila.curso;
            dto.idMateria = fila.idMateria;
            dto.nombreMateria = fila.nombreMateria;
            dto.horas = fila.horas;
            detalle.append(dto);
        }
    }

    std::stable_sort(detalle.begin(), detalle.end(), antesQue);
    return Resultado<QVector<HorasPlanAnioMateriaDTO>>::exito(detalle);
}

Resultado<ResumenCapacidadHorariaDTO> ServicioCapacidadHoraria::resumen(
    const QMap<QString, int>& horasAulaPorDocente,
    const QMap<QString, QString>& planPorCurso) const
{
    const auto detalle = horasPorPlanAnioMateria();
    if (!detalle.ok) {
        return Resultado<ResumenCapacidadHorariaDTO>::error(detalle.mensajeError,
                                                            detalle.codigoError);
    }

    const ServicioProfesor profesores(m_db);
    const auto docentes = profesores.listarProfesores();
    if (!docentes.ok) {
        return Resultado<ResumenCapacidadHorariaDTO>::error(docentes.mensajeError,
                                                            docentes.codigoError);
    }

    const ServicioCursos cursosServicio(m_db);
    const auto cursos = cursosServicio.listarCursos();
    if (!cursos.ok) {
        return Resultado<ResumenCapacidadHorariaDTO>::error(cursos.mensajeError,
                                                            cursos.codigoError);
    }

    ResumenCapacidadHorariaDTO resumen;
    resumen.detalle = detalle.valor;
    resumen.docentes = docentes.valor.size();
    resumen.cursos = cursos.valor.size();

    // ─── Horas disponibles: el mapa del frontend o 36 por docente ────────────
    QSet<QString> nombresDocentes;
    for (const ProfesorDTO& docente : docentes.valor) {
        nombresDocentes.insert(docente.nombre.trimmed());
    }

    if (horasAulaPorDocente.isEmpty()) {
        resumen.horasDisponibles = resumen.docentes * HORAS_AULA_POR_DOCENTE_POR_DEFECTO;
        resumen.avisos.append(
            QStringLiteral("No se recibieron horas de aula por docente; se usan %1 horas por"
                           " docente por defecto.")
                .arg(HORAS_AULA_POR_DOCENTE_POR_DEFECTO));
    } else {
        for (auto it = horasAulaPorDocente.cbegin(); it != horasAulaPorDocente.cend(); ++it) {
            const QString nombre = it.key().trimmed();
            if (!nombresDocentes.contains(nombre)) {
                resumen.avisos.append(
                    QStringLiteral("El docente «%1» de las horas de aula no existe en la base; se"
                                   " ignoran sus horas.")
                        .arg(nombre));
                continue;
            }
            resumen.horasDisponibles += it.value();
        }
    }

    // ─── Horas necesarias: plan de cada curso en su año ──────────────────────
    if (planPorCurso.isEmpty()) {
        resumen.avisos.append(QStringLiteral("No se recibieron planes por curso; no se pueden"
                                             " calcular las horas necesarias."));
        return Resultado<ResumenCapacidadHorariaDTO>::exito(resumen);
    }

    QHash<QString, QHash<int, int>> horasPorPlanYAnio;
    for (const HorasPlanAnioMateriaDTO& fila : resumen.detalle) {
        horasPorPlanYAnio[fila.codigoPlan][fila.anio] += fila.horas;
    }

    QSet<QString> nombresCursos;
    for (const CursoDTO& curso : cursos.valor) {
        const QString nombre = nombreCurso(curso.anio, curso.seccion);
        nombresCursos.insert(nombre);

        const auto planIt = planPorCurso.constFind(nombre);
        if (planIt == planPorCurso.cend()) {
            resumen.avisos.append(
                QStringLiteral("El curso «%1» no tiene plan asignado; se omite de las horas"
                               " necesarias.")
                    .arg(nombre));
            continue;
        }

        const QString codigoPlan = planIt.value().trimmed();
        const auto porAnio = horasPorPlanYAnio.constFind(codigoPlan);
        if (porAnio == horasPorPlanYAnio.cend()) {
            resumen.avisos.append(
                QStringLiteral("El plan «%1» del curso «%2» no existe; se omite de las horas"
                               " necesarias.")
                    .arg(codigoPlan, nombre));
            continue;
        }

        const auto horasDelAnio = porAnio.value().constFind(curso.anio);
        if (horasDelAnio == porAnio.value().cend()) {
            resumen.avisos.append(
                QStringLiteral("El plan «%1» no tiene materias en el año %2 (curso «%3»); se omite"
                               " de las horas necesarias.")
                    .arg(codigoPlan)
                    .arg(curso.anio)
                    .arg(nombre));
            continue;
        }

        resumen.horasNecesarias += horasDelAnio.value();
    }

    // El frontend puede enviar cursos que ya no existen: se avisa y se ignoran.
    for (auto it = planPorCurso.cbegin(); it != planPorCurso.cend(); ++it) {
        if (!nombresCursos.contains(it.key())) {
            resumen.avisos.append(
                QStringLiteral("El curso «%1» del mapa de planes no existe en la base; se omite.")
                    .arg(it.key()));
        }
    }

    return Resultado<ResumenCapacidadHorariaDTO>::exito(resumen);
}
