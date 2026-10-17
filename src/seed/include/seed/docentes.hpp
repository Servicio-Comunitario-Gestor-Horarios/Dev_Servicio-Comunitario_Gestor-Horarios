#pragma once

#include <optional>

#include <QString>
#include <QStringList>
#include <QTime>
#include <QVector>

#include "seed/perfil.hpp"

namespace seed {

/// Las cuatro variantes de profesorado del conjunto (spec 008, RF-6/RF-7/RF-8).
enum class VarianteDocentes {
    Holgada,
    Suplentes,
    SinProfesorado,
    SinDisponibilidad,
};

/// Franja de disponibilidad de un docente: día (1 = lunes … 5 = viernes) y rango horario.
struct FranjaDef {
    int dia;
    QTime inicio;
    QTime fin;
};

/// Docente de una variante: identidad, contacto, materias asignadas, disponibilidad
/// (5 filas, lunes–viernes) y tope de horas de aula (`horasAula` vive en el preset; se
/// declara en el perfil solo para comprobar la carga/cobertura, RF-6/RF-7).
struct DocenteDef {
    QString id;
    QString nombre;
    QString email;
    QString telefono;
    QStringList materias;
    QVector<FranjaDef> disponibilidad;
    int horasAula;
};

/// Genera la plantilla de una variante (determinista, RF-11). El `perfil` aporta las cargas
/// para calcular el tope de los titulares de la variante de suplentes.
QVector<DocenteDef> docentesDeVariante(const Perfil& perfil, VarianteDocentes variante);

/// Recuentos esperados del conjunto aplicado (RF-12): 2 turnos, 2 recesos, 16 aulas, 57
/// materias, 4 planes, 210 materias de plan, 13 cursos y, según la variante, docentes,
/// materias asignadas y disponibilidades. Determinista y pura: se usa para contrastar lo
/// escrito en la base (RF-12) sin recalcular a mano los recuentos de profesorado.
Recuentos recuentosEsperados(const Perfil& perfil, VarianteDocentes variante);

/// Comprueba la cobertura de una variante a nivel de datos: cada materia-año tiene ≥1 docente
/// asignado y, para cada uno de los dos turnos, ≥1 docente de esa materia con un rango que
/// contiene un bloque completo del turno (fronteras `inicio + k×40`). Devuelve la primera
/// materia-año que falla en `error`.
bool coberturaDeVariante(const Perfil& perfil, VarianteDocentes variante, QString* error);

/// Comprueba las restricciones de carga de una variante (RF-6/RF-7): holgada ≤ 0,6×tope;
/// suplentes con cada titular ≥ 0,8×tope, ratio carga/tope en [1,00, 1,15] y Σ topes entre el
/// 85 % y el 90 % de la demanda; las inválidas heredan la relación de la holgada. Pura.
bool docentesCumplenCarga(const Perfil& perfil, VarianteDocentes variante, QString* error);

/// Nombre legible de la variante («holgada», «suplentes», «sin profesorado»,
/// «sin disponibilidad»).
QString nombreVariante(VarianteDocentes variante);

/// Convierte texto a variante (acepta con guiones: «sin-profesorado»). Nulo si no existe.
std::optional<VarianteDocentes> varianteDesdeTexto(const QString& texto);

} // namespace seed
