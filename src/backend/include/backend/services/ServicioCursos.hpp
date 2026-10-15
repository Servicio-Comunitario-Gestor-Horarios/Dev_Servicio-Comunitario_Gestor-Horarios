#pragma once

/**
 * @file ServicioCursos.hpp
 * @brief Servicio para gestión de cursos (tabla Cursos y su relación con materias).
 */

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QVector>

#include "backend/resultado.hpp"
#include "backend/services/ordenacion.hpp"

/// Identidad de un curso: su año y su sección.
struct IdentidadCurso
{
    int anio = 0;
    QString seccion;
};

/// Recorta los espacios de la sección y la pasa a mayúsculas (Unicode).
inline QString normalizarSeccion(const QString& bruta)
{
    return bruta.trimmed().toUpper();
}

/// Valida y normaliza la identidad de un curso.
///
/// Rechaza un año menor que 1 y una sección que, tras normalizar, no sea
/// exactamente un carácter (por ejemplo `ß` → `SS` produciría dos). El mensaje
/// de error identifica el campo y el valor recibido.
inline Resultado<IdentidadCurso> validarIdentidadCurso(int anio, const QString& seccionBruta)
{
    if (anio < 1)
    {
        return Resultado<IdentidadCurso>::error(
            QStringLiteral("El año del curso debe ser un número entero mayor o igual que 1 "
                           "(valor recibido: %1).")
                .arg(anio));
    }

    const QString seccion = normalizarSeccion(seccionBruta);
    if (seccion.size() != 1)
    {
        return Resultado<IdentidadCurso>::error(
            QStringLiteral("La sección del curso debe ser exactamente un carácter "
                           "(valor recibido: «%1»).")
                .arg(seccionBruta));
    }

    return Resultado<IdentidadCurso>::exito(IdentidadCurso{anio, seccion});
}

/// Nombre derivado del curso para mostrarlo (presentación): ordinal + sección.
///
/// 1→"1ro", 2→"2do", 3→"3ro", el resto →"<n>to"; por ejemplo `"1ro A"` o `"4to B"`.
/// El solver no lo usa; lo calcula quien muestra.
inline QString nombreCurso(int anio, const QString& seccion)
{
    QString ordinal;
    switch (anio)
    {
        case 1: ordinal = QStringLiteral("ro"); break;
        case 2: ordinal = QStringLiteral("do"); break;
        case 3: ordinal = QStringLiteral("ro"); break;
        default: ordinal = QStringLiteral("to"); break;
    }
    return QStringLiteral("%1%2 %3").arg(anio).arg(ordinal).arg(seccion);
}

/// ¿El curso `a` va antes que el curso `b`?
///
/// Ordena por año ascendente; a igual año, por sección con la colación española
/// del proyecto (`nombreAntes`: insensible a mayúsculas y con acentos).
inline bool cursoAntes(int anioA, const QString& secA, int anioB, const QString& secB)
{
    if (anioA != anioB)
        return anioA < anioB;
    return nombreAntes(secA, secB);
}

/// Materia asignada a un curso, con sus horas semanales.
struct CursoMateriaDTO {
    int idMateria = 0;
    QString nombreMateria;
    int horasSemanales = 0;
};

/// Representación persistente de un curso.
struct CursoDTO {
    int id = -1;
    QString nombre;
    QString turno;
    int aulaFija = -1;          ///< id del aula fija (o -1 si no tiene)
    int numEstudiantes = 0;
    QString codigoPlan;         ///< codigo del PlanEstudio asociado (o vacío)

    QVector<CursoMateriaDTO> materias;
};

/// Servicio CRUD sobre la tabla Cursos y la relación Curso_Materia (RF-2).
class ServicioCursos {
public:
    explicit ServicioCursos(QSqlDatabase& db);

    // ─── CRUD ─────────────────────────────────────────────────────────────

    Resultado<CursoDTO> crearCurso(const QString& nombre,
                                   const QString& turno = QString(),
                                   int aulaFija = -1,
                                   int numEstudiantes = 0,
                                   const QString& codigoPlan = QString());

    Resultado<CursoDTO> obtenerCurso(int id) const;

    Resultado<QVector<CursoDTO>> listarCursos() const;

    Resultado<CursoDTO> actualizarCurso(int id,
                                        const QString& nombre,
                                        const QString& turno = QString(),
                                        int aulaFija = -1,
                                        int numEstudiantes = 0,
                                        const QString& codigoPlan = QString());

    bool eliminarCurso(int id);

    // ─── Materias del curso (N:M) ─────────────────────────────────────────

    Resultado<CursoMateriaDTO> asignarMateria(int idCurso, int idMateria,
                                              int horasSemanales);

    bool quitarMateria(int idCurso, int idMateria);

private:
    QSqlDatabase& m_db;

    bool validarCurso(const QString& nombre, int numEstudiantes, QString& error) const;
    CursoDTO mapearARecord(const QSqlRecord& record) const;
    bool cargarMaterias(CursoDTO& dto) const;
};
