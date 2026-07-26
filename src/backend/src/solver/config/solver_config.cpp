#include "backend/solver/config/solver_config.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>

/**
 * @file solver_config.cpp
 * @brief Implementación del parser de configuración del solver CP-SAT.
 */

/* --- Helpers de parseo por sección --- */

/**
 * @brief Parsea la sección de dimensiones del JSON.
 * @param obj Objeto JSON raíz.
 * @return Estructura de dimensiones parseada.
 */
static DimensionesConfig parsearDimensiones(const QJsonObject& obj) {
    DimensionesConfig d;
    d.num_profesores = obj["num_profesores"].toInt();
    d.num_materias   = obj["num_materias"].toInt();
    d.num_aulas      = obj["num_aulas"].toInt();
    d.num_cursos     = obj["num_cursos"].toInt();
    d.num_dias       = obj["num_dias"].toInt();
    d.num_slots_dia  = obj["num_slots_dia"].toInt();
    return d;
}

/**
 * @brief Parsea la sección de franja horaria del JSON.
 * @param obj Objeto JSON raíz.
 * @return Estructura de franja horaria parseada.
 */
static FranjaHorariaConfig parsearFranjaHoraria(const QJsonObject& obj) {
    FranjaHorariaConfig f;
    f.duracion_minutos = obj["duracion_minutos"].toInt();
    f.slots_por_turno  = obj["slots_por_turno"].toInt();
    return f;
}

/**
 * @brief Parsea un array de turnos individuales.
 * @param arr Array JSON con objetos de turno.
 * @return Vector de turnos parseados.
 */
static QVector<TurnoConfig> parsearTurnosIndividuales(const QJsonArray& arr) {
    QVector<TurnoConfig> turnos;
    for (const auto& item : arr) {
        TurnoConfig t;
        QJsonObject obj = item.toObject();
        t.nombre = obj["nombre"].toString();
        for (const auto& slot : obj["slot"].toArray()) {
            t.slot.append(slot.toInt());
        }
        turnos.append(t);
    }
    return turnos;
}

/**
 * @brief Parsea la sección de turnos del JSON.
 * @param obj Objeto JSON raíz.
 * @return Estructura de turnos parseada.
 */
static TurnosConfig parsearTurnos(const QJsonObject& obj) {
    TurnosConfig tc;
    tc.turnos = parsearTurnosIndividuales(obj["turnos"].toArray());
    return tc;
}

/**
 * @brief Parsea un array de recesos.
 * @param arr Array JSON con objetos de receso.
 * @return Vector de recesos parseados.
 */
static QVector<RecesoConfig> parsearRecesos(const QJsonArray& arr) {
    QVector<RecesoConfig> recesos;
    for (const auto& item : arr) {
        RecesoConfig r;
        QJsonObject obj = item.toObject();
        r.turno           = obj["turno"].toString();
        r.despues_de_slot = obj["despues_de_slot"].toInt();
        r.duracion        = obj["duracion"].toInt();
        r.inicio          = obj["inicio"].toString();
        r.fin             = obj["fin"].toString();
        recesos.append(r);
    }
    return recesos;
}

/**
 * @brief Parsea el array de materias de un curso.
 * @param arr Array JSON con objetos de materia-curso.
 * @return Vector de MateriaCurso parseados.
 */
static QVector<MateriaCurso> parsearMateriasCurso(const QJsonArray& arr) {
    QVector<MateriaCurso> materias;
    for (const auto& item : arr) {
        MateriaCurso mc;
        QJsonObject obj = item.toObject();
        mc.materiaIDx    = obj["materia_idx"].toInt();
        mc.horasSemanales = obj["horas_semanales"].toInt();
        materias.append(mc);
    }
    return materias;
}

/**
 * @brief Parsea el array de cursos.
 * @param arr Array JSON con objetos de curso.
 * @return Vector de cursos parseados.
 */
static QVector<CursoSolverConfig> parsearCursos(const QJsonArray& arr) {
    QVector<CursoSolverConfig> cursos;
    for (const auto& item : arr) {
        CursoSolverConfig c;
        QJsonObject obj = item.toObject();
        c.nombre           = obj["nombre"].toString();
        c.turno            = obj["turno"].toString();
        c.aula_fija        = obj["aula_fija"].toInt(-1);
        c.num_estudiantes  = obj["num_estudiantes"].toInt();
        c.plan             = obj["plan"].toString();
        c.materias         = parsearMateriasCurso(obj["materias"].toArray());
        cursos.append(c);
    }
    return cursos;
}

/**
 * @brief Parsea el array de materias suplentes de un profesor.
 * @param arr Array JSON con objetos de materia suplente.
 * @return Vector de MateriaSuplente parseados.
 */
static QVector<MateriaSuplente> parsearMateriasSuplentes(const QJsonArray& arr) {
    QVector<MateriaSuplente> suplentes;
    for (const auto& item : arr) {
        MateriaSuplente ms;
        QJsonObject obj = item.toObject();
        ms.materiaIDx = obj["materia_idx"].toInt();
        ms.peso       = obj["peso"].toInt();
        suplentes.append(ms);
    }
    return suplentes;
}

/**
 * @brief Parsea la disponibilidad de un profesor.
 * @param arr Array JSON con objetos de disponibilidad.
 * @return Vector de Disponibilidad parseados.
 */
static QVector<Disponibilidad> parsearDisponibilidad(const QJsonArray& arr) {
    QVector<Disponibilidad> disponibilidad;
    for (const auto& item : arr) {
        Disponibilidad d;
        QJsonObject obj = item.toObject();
        d.dia = obj["dia"].toInt();
        for (const auto& slot : obj["slot"].toArray()) {
            d.slot.append(slot.toInt());
        }
        disponibilidad.append(d);
    }
    return disponibilidad;
}

/**
 * @brief Parsea el array de profesores.
 * @param arr Array JSON con objetos de profesor.
 * @return Vector de profesores parseados.
 */
static QVector<ProfesorSolverConfig> parsearProfesores(const QJsonArray& arr) {
    QVector<ProfesorSolverConfig> profesores;
    for (const auto& item : arr) {
        ProfesorSolverConfig p;
        QJsonObject obj = item.toObject();
        p.nombre               = obj["nombre"].toString();
        p.horas_requeridas     = obj["horas_requeridas"].toInt();
        p.horas_aula           = obj["horas_aula"].toInt();
        p.turno                = obj["turno"].toString();
        p.plan                 = obj["plan"].toString();

        for (const auto& m : obj["materias_asignadas"].toArray()) {
            p.materias_asignadas.append(m.toInt());
        }

        p.materias_suplente = parsearMateriasSuplentes(obj["materias_suplente"].toArray());
        p.disponibilidad    = parsearDisponibilidad(obj["disponibilidad"].toArray());
        profesores.append(p);
    }
    return profesores;
}

/**
 * @brief Parsea el array de materias globales.
 * @param arr Array JSON con objetos de materia.
 * @return Vector de materias parseadas.
 */
static QVector<MateriaSolverConfig> parsearMaterias(const QJsonArray& arr) {
    QVector<MateriaSolverConfig> materias;
    for (const auto& item : arr) {
        MateriaSolverConfig m;
        QJsonObject obj = item.toObject();
        m.nombre = obj["nombre"].toString();
        materias.append(m);
    }
    return materias;
}

/**
 * @brief Parsea el array de aulas.
 * @param arr Array JSON con objetos de aula.
 * @return Vector de aulas parseadas.
 */
static QVector<AulaSolverConfig> parsearAulas(const QJsonArray& arr) {
    QVector<AulaSolverConfig> aulas;
    for (const auto& item : arr) {
        AulaSolverConfig a;
        QJsonObject obj = item.toObject();
        a.nombre    = obj["nombre"].toString();
        a.tipo      = obj["tipo"].toString();
        a.capacidad = obj["capacidad"].toInt();
        aulas.append(a);
    }
    return aulas;
}

/**
 * @brief Parsea la sección de planificación.
 * @param obj Objeto JSON de planificación.
 * @return Estructura de planificación parseada.
 */
static PlanificacionConfig parsearPlanificacion(const QJsonObject& obj) {
    PlanificacionConfig p;
    p.activa = obj["activa"].toBool();
    return p;
}

/**
 * @brief Parsea la sección de generación.
 * @param obj Objeto JSON de generación.
 * @return Estructura de generación parseada.
 */
static GeneracionConfig parsearGeneracion(const QJsonObject& obj) {
    GeneracionConfig g;
    for (const auto& item : obj["cursos_a_generar"].toArray()) {
        g.cursos_a_generar.append(item.toInt());
    }
    return g;
}

/**
 * @brief Parsea la sección de penalizaciones.
 * @param obj Objeto JSON de penalizaciones.
 * @return Estructura de penalizaciones parseada.
 */
static PenalizacionConfig parsearPenalizaciones(const QJsonObject& obj) {
    PenalizacionConfig p;
    p.capacidad_aula       = obj["capacidad_aula"].toInt(100);
    p.emergencia_profesor  = obj["emergencia_profesor"].toInt(50);
    return p;
}

/* --- Validaciones --- */

/**
 * @brief Valida que un índice esté dentro del rango permitido.
 * @param indice Índice a validar.
 * @param tamano Tamaño del array de referencia.
 * @param nombreCampo Nombre del campo para el mensaje de error.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarIndice(int indice, int tamano, const QString& nombreCampo) {
    if (indice < 0 || indice >= tamano) {
        return QString("Error de validación: %1 (%2) fuera de rango [0, %3)")
            .arg(nombreCampo).arg(indice).arg(tamano);
    }
    return {};
}

/**
 * @brief Valida las dimensiones contra los tamaños de arrays.
 * @param config Configuración parcialmente parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarDimensiones(const SolverConfig& config) {
    const auto& d = config.dimensiones;

    if (d.num_profesores != config.profesores.size()) {
        return QString("Error de validación: dimensiones.num_profesores (%1) != profesores.size (%2)")
            .arg(d.num_profesores).arg(config.profesores.size());
    }
    if (d.num_materias != config.materias.size()) {
        return QString("Error de validación: dimensiones.num_materias (%1) != materias.size (%2)")
            .arg(d.num_materias).arg(config.materias.size());
    }
    if (d.num_aulas != config.aulas.size()) {
        return QString("Error de validación: dimensiones.num_aulas (%1) != aulas.size (%2)")
            .arg(d.num_aulas).arg(config.aulas.size());
    }
    if (d.num_cursos != config.cursos.size()) {
        return QString("Error de validación: dimensiones.num_cursos (%1) != cursos.size (%2)")
            .arg(d.num_cursos).arg(config.cursos.size());
    }
    if (d.num_dias <= 0) {
        return QString("Error de validación: dimensiones.num_dias (%1) debe ser > 0").arg(d.num_dias);
    }
    if (d.num_slots_dia <= 0) {
        return QString("Error de validación: dimensiones.num_slots_dia (%1) debe ser > 0").arg(d.num_slots_dia);
    }
    return {};
}

/**
 * @brief Valida los turnos de cada curso.
 * @param config Configuración parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarTurnosCursos(const SolverConfig& config) {
    QSet<QString> turnosValidos;
    for (const auto& t : config.turnos.turnos) {
        turnosValidos.insert(t.nombre);
    }

    for (int i = 0; i < config.cursos.size(); ++i) {
        const auto& c = config.cursos[i];
        if (!c.turno.isEmpty() && !turnosValidos.contains(c.turno)) {
            return QString("Error de validación: curso[%1] \"%2\" tiene turno inválido \"%3\"")
                .arg(i).arg(c.nombre).arg(c.turno);
        }
    }
    return {};
}

/**
 * @brief Valida las aulas fijas de cada curso.
 * @param config Configuración parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarAulasCursos(const SolverConfig& config) {
    for (int i = 0; i < config.cursos.size(); ++i) {
        const auto& c = config.cursos[i];
        if (c.aula_fija != -1) {
            QString error = validarIndice(c.aula_fija, config.aulas.size(),
                QString("curso[%1] \"%2\" aula_fija").arg(i).arg(c.nombre));
            if (!error.isEmpty()) return error;
        }
    }
    return {};
}

/**
 * @brief Valida los índices de materias en cursos y profesores.
 * @param config Configuración parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarIndicesMaterias(const SolverConfig& config) {
    // V4: Materias en cursos
    for (int i = 0; i < config.cursos.size(); ++i) {
        const auto& c = config.cursos[i];
        for (int j = 0; j < c.materias.size(); ++j) {
            QString error = validarIndice(c.materias[j].materiaIDx, config.materias.size(),
                QString("curso[%1] \"%2\" materia[%3]").arg(i).arg(c.nombre).arg(j));
            if (!error.isEmpty()) return error;
        }
    }

    // V4: Materias asignadas en profesores
    for (int i = 0; i < config.profesores.size(); ++i) {
        const auto& p = config.profesores[i];
        for (int j = 0; j < p.materias_asignadas.size(); ++j) {
            QString error = validarIndice(p.materias_asignadas[j], config.materias.size(),
                QString("profesor[%1] \"%2\" materias_asignadas[%3]").arg(i).arg(p.nombre).arg(j));
            if (!error.isEmpty()) return error;
        }
        // Materias suplentes
        for (int j = 0; j < p.materias_suplente.size(); ++j) {
            QString error = validarIndice(p.materias_suplente[j].materiaIDx, config.materias.size(),
                QString("profesor[%1] \"%2\" materias_suplente[%3]").arg(i).arg(p.nombre).arg(j));
            if (!error.isEmpty()) return error;
        }
    }
    return {};
}

/**
 * @brief Valida que cada curso tenga al menos una materia.
 * @param config Configuración parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarCursosNoVacios(const SolverConfig& config) {
    for (int i = 0; i < config.cursos.size(); ++i) {
        if (config.cursos[i].materias.isEmpty()) {
            return QString("Error de validación: curso[%1] \"%2\" no tiene materias")
                .arg(i).arg(config.cursos[i].nombre);
        }
        if (config.cursos[i].num_estudiantes <= 0) {
            return QString("Error de validación: curso[%1] \"%2\" num_estudiantes (%3) debe ser > 0")
                .arg(i).arg(config.cursos[i].nombre).arg(config.cursos[i].num_estudiantes);
        }
    }
    return {};
}

/**
 * @brief Valida que cada profesor tenga al menos una materia asignada.
 * @param config Configuración parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarProfesoresNoVacios(const SolverConfig& config) {
    for (int i = 0; i < config.profesores.size(); ++i) {
        const auto& p = config.profesores[i];
        if (p.materias_asignadas.isEmpty() && p.materias_suplente.isEmpty()) {
            return QString("Error de validación: profesor[%1] \"%2\" no tiene materias asignadas ni suplentes")
                .arg(i).arg(p.nombre);
        }
        if (p.horas_requeridas <= 0) {
            return QString("Error de validación: profesor[%1] \"%2\" horas_requeridas (%3) debe ser > 0")
                .arg(i).arg(p.nombre).arg(p.horas_requeridas);
        }
        if (p.horas_aula <= 0) {
            return QString("Error de validación: profesor[%1] \"%2\" horas_aula (%3) debe ser > 0")
                .arg(i).arg(p.nombre).arg(p.horas_aula);
        }
        if (p.horas_aula > p.horas_requeridas) {
            return QString("Error de validación: profesor[%1] \"%2\" horas_aula (%3) > horas_requeridas (%4)")
                .arg(i).arg(p.nombre).arg(p.horas_aula).arg(p.horas_requeridas);
        }
    }
    return {};
}

/**
 * @brief Valida que cada curso tenga un solo plan.
 * @param config Configuración parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarUnSoloPlanPorCurso(const SolverConfig& config) {
    for (int i = 0; i < config.cursos.size(); ++i) {
        const auto& c = config.cursos[i];
        if (c.plan.isEmpty()) {
            return QString("Error de validación: curso[%1] \"%2\" no tiene plan asignado")
                .arg(i).arg(c.nombre);
        }
    }
    return {};
}

/**
 * @brief Valida la disponibilidad de los profesores.
 * @param config Configuración parseada.
 * @return Empty string si es válido, mensaje de error si no.
 */
static QString validarDisponibilidadProfesores(const SolverConfig& config) {
    const int totalSlots = config.dimensiones.num_dias * config.dimensiones.num_slots_dia;

    for (int i = 0; i < config.profesores.size(); ++i) {
        const auto& p = config.profesores[i];
        for (int j = 0; j < p.disponibilidad.size(); ++j) {
            const auto& d = p.disponibilidad[j];
            if (d.dia < 0 || d.dia >= config.dimensiones.num_dias) {
                return QString("Error de validación: profesor[%1] \"%2\" disponibilidad[%3] dia (%4) fuera de rango [0, %5)")
                    .arg(i).arg(p.nombre).arg(j).arg(d.dia).arg(config.dimensiones.num_dias);
            }
            for (int k = 0; k < d.slot.size(); ++k) {
                if (d.slot[k] < 0 || d.slot[k] >= config.dimensiones.num_slots_dia) {
                    return QString("Error de validación: profesor[%1] \"%2\" disponibilidad[%3] slot[%4] (%5) fuera de rango [0, %6)")
                        .arg(i).arg(p.nombre).arg(j).arg(k).arg(d.slot[k]).arg(config.dimensiones.num_slots_dia);
                }
            }
        }
    }
    return {};
}

/* --- Función principal --- */

Resultado<SolverConfig> SolverConfig::fromJson(const QJsonObject& obj) {
    // V1: Verificar que el JSON no esté vacío
    if (obj.isEmpty()) {
        return Resultado<SolverConfig>::error("Error de validación: JSON vacío o no parseable");
    }

    // V1: Verificar secciones requeridas
    QStringList seccionesRequeridas = {
        "version", "dimensiones", "franja_horaria", "turnos", "recesos",
        "cursos", "profesores", "materias", "aulas",
        "planificacion", "generacion", "penalizaciones"
    };
    for (const auto& seccion : seccionesRequeridas) {
        if (!obj.contains(seccion)) {
            return Resultado<SolverConfig>::error(
                QString("Error de validación: sección requerida \"%1\" no encontrada").arg(seccion));
        }
    }

    SolverConfig config;

    // Parsear secciones simples
    config.version        = obj["version"].toString();
    config.dimensiones    = parsearDimensiones(obj["dimensiones"].toObject());
    config.franja_horaria = parsearFranjaHoraria(obj["franja_horaria"].toObject());
    config.turnos         = parsearTurnos(obj["turnos"].toObject());
    config.recesos        = parsearRecesos(obj["recesos"].toArray());
    config.planificacion  = parsearPlanificacion(obj["planificacion"].toObject());
    config.generacion     = parsearGeneracion(obj["generacion"].toObject());
    config.penalizaciones = parsearPenalizaciones(obj["penalizaciones"].toObject());

    // Parsear secciones con arrays
    config.cursos     = parsearCursos(obj["cursos"].toArray());
    config.profesores = parsearProfesores(obj["profesores"].toArray());
    config.materias   = parsearMaterias(obj["materias"].toArray());
    config.aulas      = parsearAulas(obj["aulas"].toArray());

    // --- Validaciones ---

    // V3: Dimensiones vs tamaños de arrays
    QString error = validarDimensiones(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    // V7: Turnos de cursos
    error = validarTurnosCursos(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    // V8: Aulas fijas
    error = validarAulasCursos(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    // V4-V5: Índices de materias
    error = validarIndicesMaterias(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    // V9-V10: Cursos no vacíos
    error = validarCursosNoVacios(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    // V10: Profesores no vacíos
    error = validarProfesoresNoVacios(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    // V11: Un solo plan por curso
    error = validarUnSoloPlanPorCurso(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    // V12: Disponibilidad
    error = validarDisponibilidadProfesores(config);
    if (!error.isEmpty()) return Resultado<SolverConfig>::error(error);

    return Resultado<SolverConfig>::exito(config);
}
