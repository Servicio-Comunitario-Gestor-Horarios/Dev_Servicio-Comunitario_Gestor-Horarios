#pragma once

#include <QString>
#include <QStringList>
#include <QTime>
#include <QVector>

#include "seed/perfil.hpp"

namespace seed {

/// Franja de disponibilidad de un docente: día (1 = lunes … 5 = viernes) y rango horario.
struct FranjaDef {
    int dia = 0;
    QTime inicio;
    QTime fin;
};

/// Docente del centro: identidad, contacto, turno (del perfil, no persistido), materias
/// asignadas, disponibilidad (5 filas, lunes–viernes) y tope de horas de aula (`horasAula`
/// = 36 horas académicas para todos, D-3).
struct DocenteDef {
    QString id;
    QString nombre;
    QString email;
    QString telefono;
    QString turno;             ///< «Mañana» o «Tarde»: vive en el perfil y no se persiste
    QStringList materias;      ///< materias-año del catálogo asignadas
    QVector<FranjaDef> disponibilidad; ///< 5 filas, una por día, con la ventana del turno
    int horasAula = 0;         ///< tope de horas académicas de aula (36)
};

/// Los 33 docentes del conjunto (30 posiciones del centro con nombres ficticios, 2 añadidos
/// para las materias sin carga de D-4 y el apoyo de idiomas de la tarde de la 018),
/// deterministas y en orden fijo (P001…P033).
/// Aplica la regla de asignación del plan §5.4: todas las materias-año del área de cada
/// posición, una sola vez, sin filtrar por año y solo si el área tiene cursos en el turno
/// del docente. Pura: no lee reloj, disco ni configuración.
QVector<DocenteDef> docentesDelCentro(const Perfil& perfil);

/// Recuentos esperados del conjunto aplicado (plan §4.8): 2 turnos, 3 eventos, 2 tipos de
/// aula (general + cancha), 13 aulas, 85 materias, 2 planes, 95 cargas, 24 cursos, 33
/// docentes, 272 asignaciones, 165 disponibilidades, 0 preferencias, duración 40 y 1 fila
/// de configuración. Determinista y pura: contrasta lo escrito en la base (RF-8).
Recuentos recuentosEsperados(const Perfil& perfil);

/// Valida la plantilla de docentes (RF-6, plan §6.10): 33 docentes, identidades y contactos
/// únicos, tope de 36 horas de aula, turno declarado y 16/17 por turno (018), 5 filas de
/// disponibilidad en los días 1–5 dentro de la ventana de su turno y 272 asignaciones a
/// materias del catálogo. Deja en `error` el primer problema encontrado, en español.
bool validarDocentes(const Perfil& perfil, const QVector<DocenteDef>& docentes, QString* error);

/// Comprueba la cobertura a nivel de datos (RF-6): cada materia del catálogo tiene al menos
/// un docente asignado y cada materia de cada curso tiene docente habilitado **en el turno
/// de ese curso**. Deja en `error` la primera materia descubierta.
bool coberturaDelCentro(const Perfil& perfil, const QVector<DocenteDef>& docentes,
                        QString* error);

} // namespace seed
