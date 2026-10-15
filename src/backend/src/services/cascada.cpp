#include "backend/services/cascada.hpp"

namespace
{
    /// Construye una relación de cascada de forma legible.
    RelacionCascada relacion(const QString& dominio, const QString& tabla,
                             const QString& columna, const QString& columnaId,
                             const QString& etiqueta, const QString& descripcion,
                             bool esDominio, ModoAccion modo = ModoAccion::Eliminar)
    {
        RelacionCascada r;
        r.dominio = dominio;
        r.tabla = tabla;
        r.columna = columna;
        r.columnaId = columnaId;
        r.etiqueta = etiqueta;
        r.descripcion = descripcion;
        r.esDominio = esDominio;
        r.modo = modo;
        return r;
    }

    /// Etiqueta legible de un curso dependiente: «año sección».
    const QString ETIQUETA_CURSO = QStringLiteral("anio || ' ' || seccion");
} // namespace

QVector<RelacionCascada> dependenciasDe(const QString& dominio)
{
    QVector<RelacionCascada> relaciones;

    if (dominio == QStringLiteral("plan"))
    {
        // Vínculos plan-materia y cursos que pertenecen al plan.
        relaciones.append(relacion(
            QStringLiteral("plan_materia"), QStringLiteral("PlanEstudio_Materia"),
            QStringLiteral("codigo_PlanEstudio"), QStringLiteral("rowid"),
            QStringLiteral("codigo_PlanEstudio"), QStringLiteral("Materia del plan"), false));

    }
    else if (dominio == QStringLiteral("materia"))
    {
        relaciones.append(relacion(
            QStringLiteral("plan_materia"), QStringLiteral("PlanEstudio_Materia"),
            QStringLiteral("id_Materia"), QStringLiteral("rowid"),
            QStringLiteral("codigo_PlanEstudio"), QStringLiteral("Plan que incluye la materia"),
            false));

        relaciones.append(relacion(
            QStringLiteral("profesor_materia"), QStringLiteral("Profesor_Materia"),
            QStringLiteral("id_Materia"), QStringLiteral("rowid"),
            QStringLiteral("id_Profesor"), QStringLiteral("Docente que imparte la materia"),
            false));
    }
    else if (dominio == QStringLiteral("profesor"))
    {
        relaciones.append(relacion(
            QStringLiteral("profesor_materia"), QStringLiteral("Profesor_Materia"),
            QStringLiteral("id_Profesor"), QStringLiteral("rowid"),
            QStringLiteral("id_Materia"), QStringLiteral("Materia del docente"), false));

        relaciones.append(relacion(
            QStringLiteral("disponibilidad"), QStringLiteral("Disponibilidad_Profesor"),
            QStringLiteral("id_Profesor"), QStringLiteral("id"),
            QStringLiteral("dia"), QStringLiteral("Franja de disponibilidad"), false));
    }
    else if (dominio == QStringLiteral("aula"))
    {
        relaciones.append(relacion(
            QStringLiteral("curso"), QStringLiteral("Cursos"),
            QStringLiteral("aula"), QStringLiteral("id"),
            ETIQUETA_CURSO, QStringLiteral("Curso con esa aula"), false,
            ModoAccion::AnularReferencia));
    }
    else if (dominio == QStringLiteral("curso"))
    {
        // El curso ya no arrastra nada (se retiro Curso_Materia).
    }
    else if (dominio == QStringLiteral("turno"))
    {
        relaciones.append(relacion(
            QStringLiteral("receso"), QStringLiteral("Recesos"),
            QStringLiteral("turno"), QStringLiteral("despues_de_slot"),
            QString(), QStringLiteral("Receso del turno"), false));

        relaciones.append(relacion(
            QStringLiteral("curso"), QStringLiteral("Cursos"),
            QStringLiteral("turno"), QStringLiteral("id"),
            ETIQUETA_CURSO, QStringLiteral("Curso con ese turno"), false,
            ModoAccion::AnularReferencia));
    }

    return relaciones;
}

bool dominioValido(const QString& dominio)
{
    return !claveDeDominio(dominio).tabla.isEmpty();
}

ClaveDominio claveDeDominio(const QString& dominio)
{
    ClaveDominio clave;

    if (dominio == QStringLiteral("plan"))
    {
        clave.tabla = QStringLiteral("PlanEstudio");
        clave.columna = QStringLiteral("codigo");
    }
    else if (dominio == QStringLiteral("materia"))
    {
        clave.tabla = QStringLiteral("Materias");
        clave.columna = QStringLiteral("id");
    }
    else if (dominio == QStringLiteral("profesor"))
    {
        clave.tabla = QStringLiteral("Profesores");
        clave.columna = QStringLiteral("id");
    }
    else if (dominio == QStringLiteral("aula"))
    {
        clave.tabla = QStringLiteral("Aulas");
        clave.columna = QStringLiteral("id");
    }
    else if (dominio == QStringLiteral("curso"))
    {
        clave.tabla = QStringLiteral("Cursos");
        clave.columna = QStringLiteral("id");
    }
    else if (dominio == QStringLiteral("turno"))
    {
        clave.tabla = QStringLiteral("Turnos");
        clave.columna = QStringLiteral("nombre");
    }

    return clave;
}
