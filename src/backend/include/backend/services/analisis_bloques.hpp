#pragma once

/**
 * @file analisis_bloques.hpp
 * @brief Informe puro de bloques, materias, desvío, huecos y separación (spec 014, RF-12).
 *
 * Se calcula sobre el `HorarioSalida` y la configuración; no cambia el contrato de salida del
 * solver. Los días son de **dominio** (0 = domingo … 6 = sábado): `(día de generación + 1) % 7`.
 */

#include <QString>
#include <QVector>

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Un bloque de dos horas del horario.
struct BloqueSalida {
    QString curso;        ///< Clave del curso («año|sección»).
    int diaDominio = -1;  ///< Día de dominio (0 = domingo).
    int slotInicio = -1;  ///< Primera franja del bloque.
    int slotFin = -1;     ///< Segunda franja del bloque.
    int materiaA = -1;    ///< Materia de la primera franja.
    int materiaB = -1;    ///< Materia de la segunda franja.
    bool compartido = false;
    int aula = -1;        ///< Aula real del bloque (la misma en las dos franjas).
};

/// Una aparición (bloque) de una materia-curso.
struct AparicionSalida {
    int diaDominio = -1;
    int slotInicio = -1;
    int slotFin = -1;
};

/// Resumen de una materia-curso.
struct MateriaSalida {
    QString curso;                    ///< Clave del curso («año|sección»).
    int materia = -1;
    int apariciones = 0;              ///< Bloques (simples o compartidos) en los que participa.
    QVector<AparicionSalida> franjas; ///< Una aparición por bloque, con su día y sus franjas.
    QVector<int> diasDominio;         ///< Días de dominio en los que aparece (ordenados).
    int franjasDescubiertas = 0;      ///< Franjas fuera de un bloque simple suyo.
    int resto = 0;                    ///< `h mod 2`.
    QVector<int> diasIncumplidosSeparacion; ///< Días de dominio implicados en pares consecutivos.
    int incumplimientosSeparacion = 0;      ///< Pares de apariciones en días consecutivos.
};

/// Agregados de un curso.
struct CursoSalida {
    QString curso;
    int desvio = 0;
    int tope = 0;
    int bloquesSimples = 0;
    int bloquesCompartidos = 0;
    int bloquesDeUno = 0;
    int huecos = 0;              ///< Franjas de hueco del curso.
    int huecosNoViernes = 0;     ///< Franjas de hueco fuera del viernes (día de generación 4).
    int incumplimientosSeparacion = 0;
};

/// Un tramo de huecos de un curso en un día.
struct HuecoSalida {
    QString curso;
    int diaDominio = -1;
    int slotInicio = -1;
    int slotFin = -1;   ///< Inclusivo.
    bool esViernes = false;  ///< El viernes es preferencia blanda (dominio 5).
};

/// Informe completo de la generación.
struct AnalisisBloques {
    QVector<BloqueSalida> bloques;
    QVector<MateriaSalida> materias;
    QVector<HuecoSalida> huecos;
    QVector<CursoSalida> cursos;

    int bloquesSimples = 0;
    int bloquesCompartidos = 0;
    int bloquesDeUno = 0;   ///< Comprobación del P4: 0 en toda solución válida.
    int desvio = 0;         ///< Único desvío: Σ_m máx(0, descubiertas_m − h_m mod 2).
    int tope = 0;           ///< Σ_m (h_m mod 2).
    int incumplimientosSeparacion = 0;
    int huecosNoFinal = 0;  ///< Franjas de hueco con clase después en el día.
    int huecosNoViernes = 0;///< Franjas de hueco fuera del viernes (día de generación 4).
};

/// Calcula el informe (puro). No modifica sus argumentos.
AnalisisBloques analizarBloques(const HorarioSalida& horario, const SolverConfig& config);
