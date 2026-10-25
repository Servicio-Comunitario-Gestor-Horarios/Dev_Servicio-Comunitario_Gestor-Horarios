#include "backend/solver/config/solver_config.hpp"

#include "backend/solver/config/contrato_solver.hpp"

#include <QJsonArray>
#include <QJsonValue>
#include <QStringList>

namespace {

int leerEntero(const QJsonObject& obj, const char* clave, int porDefecto = 0)
{
    return obj.contains(QLatin1String(clave))
               ? obj.value(QLatin1String(clave)).toInt(porDefecto)
               : porDefecto;
}

QString leerTexto(const QJsonObject& obj, const char* clave)
{
    return obj.value(QLatin1String(clave)).toString();
}

QVector<int> leerEnteros(const QJsonArray& arr)
{
    QVector<int> valores;
    valores.reserve(arr.size());
    for (const QJsonValue& valor : arr)
        valores.append(valor.toInt());
    return valores;
}

QJsonArray aJson(const QVector<int>& valores)
{
    QJsonArray arr;
    for (int valor : valores) arr.append(valor);
    return arr;
}

/* ── Dimensiones ── */
DimensionesConfig parsearDimensiones(const QJsonObject& obj)
{
    DimensionesConfig d;
    d.num_profesores = leerEntero(obj, "num_profesores");
    d.num_materias   = leerEntero(obj, "num_materias");
    d.num_aulas      = leerEntero(obj, "num_aulas");
    d.num_cursos     = leerEntero(obj, "num_cursos");
    d.num_dias       = leerEntero(obj, "num_dias");
    d.num_slots_dia  = leerEntero(obj, "num_slots_dia");
    return d;
}
QJsonObject aJson(const DimensionesConfig& d)
{
    QJsonObject obj;
    obj["num_profesores"] = d.num_profesores;
    obj["num_materias"]   = d.num_materias;
    obj["num_aulas"]      = d.num_aulas;
    obj["num_cursos"]     = d.num_cursos;
    obj["num_dias"]       = d.num_dias;
    obj["num_slots_dia"]  = d.num_slots_dia;
    return obj;
}

/* ── Franja horaria ── */
FranjaHorariaConfig parsearFranjaHoraria(const QJsonObject& obj)
{
    FranjaHorariaConfig f;
    f.duracion_minutos = leerEntero(obj, "duracion_minutos");
    f.slots_por_turno  = leerEntero(obj, "slots_por_turno");
    return f;
}
QJsonObject aJson(const FranjaHorariaConfig& f)
{
    QJsonObject obj;
    obj["duracion_minutos"] = f.duracion_minutos;
    obj["slots_por_turno"]  = f.slots_por_turno;
    return obj;
}

/* ── Turnos ── */
TurnosConfig parsearTurnos(const QJsonObject& obj)
{
    TurnosConfig tc;
    for (const QJsonValue& valor : obj["turnos"].toArray()) {
        const QJsonObject o = valor.toObject();
        TurnoConfig t;
        t.nombre = leerTexto(o, "nombre");
        t.slot   = leerEnteros(o["slot"].toArray());
        t.inicio = leerTexto(o, "inicio");
        t.fin    = leerTexto(o, "fin");
        tc.turnos.append(t);
    }
    return tc;
}
QJsonObject aJson(const TurnosConfig& tc)
{
    QJsonArray arr;
    for (const TurnoConfig& t : tc.turnos) {
        QJsonObject obj;
        obj["nombre"] = t.nombre;
        obj["slot"]   = aJson(t.slot);
        obj["inicio"] = t.inicio;
        obj["fin"]    = t.fin;
        arr.append(obj);
    }
    QJsonObject obj;
    obj["turnos"] = arr;
    return obj;
}

/* ── Eventos del turno ── */
QVector<EventoConfig> parsearEventos(const QJsonArray& arr)
{
    QVector<EventoConfig> eventos;
    eventos.reserve(arr.size());
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        EventoConfig e;
        e.nombre           = leerTexto(o, "nombre");
        e.turno            = leerTexto(o, "turno");
        e.despues_de_slot  = leerEntero(o, "despues_de_slot");
        e.orden            = leerEntero(o, "orden");
        e.duracion         = leerEntero(o, "duracion");
        e.inicio           = leerTexto(o, "inicio");
        e.fin              = leerTexto(o, "fin");
        eventos.append(e);
    }
    return eventos;
}
QJsonArray aJson(const QVector<EventoConfig>& eventos)
{
    QJsonArray arr;
    for (const EventoConfig& e : eventos) {
        QJsonObject obj;
        obj["nombre"]          = e.nombre;
        obj["turno"]           = e.turno;
        obj["despues_de_slot"] = e.despues_de_slot;
        obj["orden"]           = e.orden;
        obj["duracion"]        = e.duracion;
        obj["inicio"]          = e.inicio;
        obj["fin"]             = e.fin;
        arr.append(obj);
    }
    return arr;
}

/* ── Planes embebidos ── */
QVector<PlanSolverConfig> parsearPlanes(const QJsonArray& arr)
{
    QVector<PlanSolverConfig> planes;
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        PlanSolverConfig plan;
        plan.codigo = leerTexto(o, "codigo");
        for (const QJsonValue& mv : o["materias"].toArray()) {
            const QJsonObject mo = mv.toObject();
            PlanMateriaConfig m;
            m.materiaIDx     = leerEntero(mo, "materia_idx");
            m.curso          = leerEntero(mo, "curso");
            m.horasSemanales = leerEntero(mo, "horas_semanales");
            plan.materias.append(m);
        }
        planes.append(plan);
    }
    return planes;
}
QJsonArray aJson(const QVector<PlanSolverConfig>& planes)
{
    QJsonArray arr;
    for (const PlanSolverConfig& plan : planes) {
        QJsonArray mats;
        for (const PlanMateriaConfig& m : plan.materias) {
            QJsonObject mo;
            mo["materia_idx"]     = m.materiaIDx;
            mo["curso"]           = m.curso;
            mo["horas_semanales"] = m.horasSemanales;
            mats.append(mo);
        }
        QJsonObject obj;
        obj["codigo"]   = plan.codigo;
        obj["materias"] = mats;
        arr.append(obj);
    }
    return arr;
}

/* ── Cursos ── */
QVector<CursoSolverConfig> parsearCursos(const QJsonArray& arr)
{
    QVector<CursoSolverConfig> cursos;
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        CursoSolverConfig c;
        c.anio            = leerEntero(o, "anio");
        c.seccion         = leerTexto(o, "seccion");
        c.turno           = leerTexto(o, "turno");
        c.num_estudiantes = leerEntero(o, "num_estudiantes", -1);
        c.plan            = leerTexto(o, "plan");
        c.aula            = leerEntero(o, "aula", -1);
        cursos.append(c);
    }
    return cursos;
}
QJsonArray aJson(const QVector<CursoSolverConfig>& cursos)
{
    QJsonArray arr;
    for (const CursoSolverConfig& c : cursos) {
        QJsonObject obj;
        obj["anio"]            = c.anio;
        obj["seccion"]         = c.seccion;
        obj["turno"]           = c.turno;
        obj["num_estudiantes"] = c.num_estudiantes;
        obj["plan"]            = c.plan;
        obj["aula"]            = c.aula;
        arr.append(obj);
    }
    return arr;
}

/* ── Profesores ── */
QVector<MateriaSuplente> parsearMateriasSuplentes(const QJsonArray& arr)
{
    QVector<MateriaSuplente> suplentes;
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        MateriaSuplente m;
        m.materiaIDx = leerEntero(o, "materia_idx");
        m.peso       = leerEntero(o, "peso");
        suplentes.append(m);
    }
    return suplentes;
}
QJsonArray aJson(const QVector<MateriaSuplente>& suplentes)
{
    QJsonArray arr;
    for (const MateriaSuplente& m : suplentes) {
        QJsonObject obj;
        obj["materia_idx"] = m.materiaIDx;
        obj["peso"]        = m.peso;
        arr.append(obj);
    }
    return arr;
}
QVector<Disponibilidad> parsearDisponibilidad(const QJsonArray& arr)
{
    QVector<Disponibilidad> disp;
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        Disponibilidad d;
        d.dia  = leerEntero(o, "dia");
        d.slot = leerEnteros(o["slot"].toArray());
        disp.append(d);
    }
    return disp;
}
QJsonArray aJson(const QVector<Disponibilidad>& disp)
{
    QJsonArray arr;
    for (const Disponibilidad& d : disp) {
        QJsonObject obj;
        obj["dia"]  = d.dia;
        obj["slot"] = aJson(d.slot);
        arr.append(obj);
    }
    return arr;
}
QVector<ProfesorSolverConfig> parsearProfesores(const QJsonArray& arr)
{
    QVector<ProfesorSolverConfig> profesores;
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        ProfesorSolverConfig p;
        p.nombre             = leerTexto(o, "nombre");
        p.horas_requeridas   = leerEntero(o, "horas_requeridas", -1);
        p.horas_aula         = leerEntero(o, "horas_aula");
        p.turno              = leerTexto(o, "turno");
        p.plan               = leerTexto(o, "plan");
        p.anios              = leerEnteros(o["anios"].toArray());
        p.materias_asignadas = leerEnteros(o["materias_asignadas"].toArray());
        p.materias_suplente  = parsearMateriasSuplentes(o["materias_suplente"].toArray());
        p.disponibilidad     = parsearDisponibilidad(o["disponibilidad"].toArray());
        profesores.append(p);
    }
    return profesores;
}
QJsonArray aJson(const QVector<ProfesorSolverConfig>& profesores)
{
    QJsonArray arr;
    for (const ProfesorSolverConfig& p : profesores) {
        QJsonObject obj;
        obj["nombre"]             = p.nombre;
        obj["horas_requeridas"]   = p.horas_requeridas;
        obj["horas_aula"]         = p.horas_aula;
        obj["turno"]              = p.turno;
        obj["plan"]               = p.plan;
        obj["anios"]              = aJson(p.anios);
        obj["materias_asignadas"] = aJson(p.materias_asignadas);
        obj["materias_suplente"]  = aJson(p.materias_suplente);
        obj["disponibilidad"]     = aJson(p.disponibilidad);
        arr.append(obj);
    }
    return arr;
}

/* ── Materias / Aulas ── */
QVector<MateriaSolverConfig> parsearMaterias(const QJsonArray& arr)
{
    QVector<MateriaSolverConfig> materias;
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        MateriaSolverConfig m;
        m.nombre = leerTexto(o, "nombre");
        m.tipo   = leerTexto(o, "tipo");
        materias.append(m);
    }
    return materias;
}
QJsonArray aJson(const QVector<MateriaSolverConfig>& materias)
{
    QJsonArray arr;
    for (const MateriaSolverConfig& m : materias) {
        QJsonObject obj;
        obj["nombre"] = m.nombre;
        obj["tipo"]   = m.tipo;
        arr.append(obj);
    }
    return arr;
}
QVector<AulaSolverConfig> parsearAulas(const QJsonArray& arr)
{
    QVector<AulaSolverConfig> aulas;
    for (const QJsonValue& valor : arr) {
        const QJsonObject o = valor.toObject();
        AulaSolverConfig a;
        a.nombre    = leerTexto(o, "nombre");
        a.tipo      = leerTexto(o, "tipo");
        a.capacidad = leerEntero(o, "capacidad");
        aulas.append(a);
    }
    return aulas;
}
QJsonArray aJson(const QVector<AulaSolverConfig>& aulas)
{
    QJsonArray arr;
    for (const AulaSolverConfig& a : aulas) {
        QJsonObject obj;
        obj["nombre"]    = a.nombre;
        obj["tipo"]      = a.tipo;
        obj["capacidad"] = a.capacidad;
        arr.append(obj);
    }
    return arr;
}

/* ── Validaciones V1-V15 (RF-11). Devuelven "" si OK. ── */

/// V4/V6/V12: indice dentro de [0, tamano).
QString validarIndice(int indice, int tamano, const QString& campo)
{
    if (indice < 0 || indice >= tamano)
        return QStringLiteral("%1: indice %2 fuera de rango [0,%3).")
            .arg(campo).arg(indice).arg(tamano);
    return {};
}

const TurnoConfig* buscarTurno(const SolverConfig& c, const QString& nombre)
{
    for (const TurnoConfig& t : c.turnos.turnos)
        if (t.nombre == nombre) return &t;
    return nullptr;
}

/// V1. Estructura: la version se comprueba ANTES que las secciones y el JSON 2.0 trae todas las
/// secciones requeridas, incluida la de eventos (RF-1).
QString validarEstructura(const QJsonObject& obj)
{
    // La version manda: un 1.0 sin `eventos` se rechaza por version, no por «falta eventos».
    const QString version = obj.value(QLatin1String("version")).toString();
    if (version.isEmpty())
        return QStringLiteral("V1: la configuración no declara versión; se espera '2.0'.");
    if (version != QLatin1String("2.0"))
        return QStringLiteral("V1: versión de configuración no reconocida ('%1'); se espera '2.0'.")
            .arg(version);

    const QStringList requeridas = {QStringLiteral("version"),      QStringLiteral("dimensiones"),
                                    QStringLiteral("franja_horaria"), QStringLiteral("turnos"),
                                    QStringLiteral("eventos"),       QStringLiteral("planes"),
                                    QStringLiteral("cursos"),        QStringLiteral("profesores"),
                                    QStringLiteral("materias"),      QStringLiteral("aulas")};
    for (const QString& clave : requeridas)
        if (!obj.contains(clave))
            return QStringLiteral("V1: falta la seccion '%1'.").arg(clave);
    return {};
}

/// V2. Tipos/rangos + peso >= 0 + duracion_minutos > 0.
QString validarTiposYRangos(const SolverConfig& c)
{
    if (c.franja_horaria.duracion_minutos <= 0)
        return QStringLiteral("V2: duracion_minutos debe ser mayor que 0.");
    for (const ProfesorSolverConfig& p : c.profesores)
        for (const MateriaSuplente& s : p.materias_suplente)
            if (s.peso < 0)
                return QStringLiteral("V2: peso negativo en una suplencia de '%1'.").arg(p.nombre);
    return {};
}

/// V3. Las dimensiones coinciden con los tamanos de las listas.
QString validarDimensiones(const SolverConfig& c)
{
    const DimensionesConfig& d = c.dimensiones;
    if (d.num_profesores != c.profesores.size()) return QStringLiteral("V3: num_profesores no coincide.");
    if (d.num_materias   != c.materias.size())   return QStringLiteral("V3: num_materias no coincide.");
    if (d.num_aulas      != c.aulas.size())      return QStringLiteral("V3: num_aulas no coincide.");
    if (d.num_cursos     != c.cursos.size())     return QStringLiteral("V3: num_cursos no coincide.");
    return {};
}

/// V4. Indices de materias en rango (planes y docentes).
QString validarIndicesMaterias(const SolverConfig& c)
{
    const int nM = c.materias.size();
    for (const PlanSolverConfig& plan : c.planes)
        for (const PlanMateriaConfig& m : plan.materias) {
            const QString e = validarIndice(m.materiaIDx, nM, QStringLiteral("V4 materia del plan"));
            if (!e.isEmpty()) return e;
        }
    for (const ProfesorSolverConfig& p : c.profesores) {
        for (int idx : p.materias_asignadas) {
            const QString e = validarIndice(idx, nM, QStringLiteral("V4 materia asignada"));
            if (!e.isEmpty()) return e;
        }
        for (const MateriaSuplente& s : p.materias_suplente) {
            const QString e = validarIndice(s.materiaIDx, nM, QStringLiteral("V4 materia suplente"));
            if (!e.isEmpty()) return e;
        }
    }
    return {};
}

/// V5. Indices de profesores en rango (sin referencias cruzadas hoy).
QString validarIndicesProfesores(const SolverConfig&) { return {}; }

/// V6. Indices de aulas en rango.
QString validarIndicesAulas(const SolverConfig& c)
{
    for (const CursoSolverConfig& cu : c.cursos)
        if (cu.aula >= 0) {
            const QString e = validarIndice(cu.aula, c.aulas.size(), QStringLiteral("V6 aula de curso"));
            if (!e.isEmpty()) return e;
        }
    return {};
}

/// V7. El curso declara un turno y ese turno existe.
QString validarTurnosDeCursos(const SolverConfig& c)
{
    for (const CursoSolverConfig& cu : c.cursos) {
        if (cu.turno.isEmpty())
            return QStringLiteral("V7: el curso %1%2 no declara turno.").arg(cu.anio).arg(cu.seccion);
        if (!buscarTurno(c, cu.turno))
            return QStringLiteral("V7: el turno '%1' del curso %2%3 no existe.")
                .arg(cu.turno).arg(cu.anio).arg(cu.seccion);
    }
    return {};
}

/// V8. Con aula fija activa cada curso declara aula; inactiva, se ignora.
QString validarAulasDeCursos(const SolverConfig& c)
{
    if (!c.aula_fija) return {};
    for (const CursoSolverConfig& cu : c.cursos)
        if (cu.aula < 0)
            return QStringLiteral("V8: modo aula fija activo y el curso %1%2 no declara aula.")
                .arg(cu.anio).arg(cu.seccion);
    return {};
}

/// V9. num_estudiantes opcional: si esta presente (centinela -1 = ausente) debe ser positivo.
QString validarNumeroEstudiantes(const SolverConfig& c)
{
    for (const CursoSolverConfig& cu : c.cursos)
        if (cu.num_estudiantes != -1 && cu.num_estudiantes <= 0)
            return QStringLiteral("V9: num_estudiantes debe ser un entero positivo en el curso %1%2.")
                .arg(cu.anio).arg(cu.seccion);
    return {};
}

/// Coherencia de datos (RF-11): si `horas_requeridas` esta presente (centinela -1 = ausente),
/// `horas_aula` no debe superarla. Es un chequeo del dato de dominio, no una regla del solver.
QString validarCoherenciaHoras(const SolverConfig& c)
{
    for (const ProfesorSolverConfig& p : c.profesores)
        if (p.horas_requeridas != -1 && p.horas_aula > p.horas_requeridas)
            return QStringLiteral("El docente '%1' tiene horas_aula (%2) mayor que horas_requeridas (%3).")
                .arg(p.nombre).arg(p.horas_aula).arg(p.horas_requeridas);
    return {};
}

/// V10. Curso con plan y materias; docente con materias.
QString validarListasNoVacias(const SolverConfig& c)
{
    for (const CursoSolverConfig& cu : c.cursos) {
        if (cu.plan.isEmpty())
            return QStringLiteral("V10: el curso %1%2 no declara plan.").arg(cu.anio).arg(cu.seccion);
        if (materiasDelCurso(c, cu).isEmpty())
            return QStringLiteral("V10: el curso %1%2 no tiene materias en su plan.").arg(cu.anio).arg(cu.seccion);
    }
    for (const ProfesorSolverConfig& p : c.profesores)
        if (p.materias_asignadas.isEmpty() && p.materias_suplente.isEmpty())
            return QStringLiteral("V10: el docente '%1' no tiene materias.").arg(p.nombre);
    return {};
}

/// V11. Cada curso referencia exactamente un plan existente.
QString validarUnPlanPorCurso(const SolverConfig& c)
{
    for (const CursoSolverConfig& cu : c.cursos) {
        int coincidencias = 0;
        for (const PlanSolverConfig& plan : c.planes)
            if (plan.codigo == cu.plan) ++coincidencias;
        if (coincidencias != 1)
            return QStringLiteral("V11: el curso %1%2 debe referenciar exactamente un plan ('%3').")
                .arg(cu.anio).arg(cu.seccion).arg(cu.plan);
    }
    return {};
}

/// V12. Disponibilidad dentro de dias y slots declarados.
QString validarDisponibilidad(const SolverConfig& c)
{
    for (const ProfesorSolverConfig& p : c.profesores)
        for (const Disponibilidad& d : p.disponibilidad) {
            if (d.dia < 0 || d.dia >= c.dimensiones.num_dias)
                return QStringLiteral("V12: dia fuera de rango en la disponibilidad de '%1'.").arg(p.nombre);
            for (int slot : d.slot)
                if (slot < 0 || slot >= c.dimensiones.num_slots_dia)
                    return QStringLiteral("V12: slot fuera de rango en la disponibilidad de '%1'.").arg(p.nombre);
        }
    return {};
}

/// V13. Cada materia-curso tiene al menos un candidato elegible.
/// Exige, para cada par (curso, materia) derivado del plan, un docente que opte por la materia
/// (titular o suplente), respete plan y año y tenga disponibilidad que SOLAPE con las franjas del
/// turno declarado del curso. Evalúa sobre los contratos, antes de resolver; no exige capacidad.
QString validarCandidatosElegibles(const SolverConfig& c)
{
    for (const CursoSolverConfig& cu : c.cursos) {
        for (const PlanMateriaConfig& mc : materiasDelCurso(c, cu)) {
            bool hay = false;
            for (const ProfesorSolverConfig& p : c.profesores)
                if (docenteCandidatoElegible(c, cu, p, mc.materiaIDx)) { hay = true; break; }
            if (!hay)
                return QStringLiteral("V13: la materia %1 del curso %2%3 no tiene candidato elegible "
                                      "con disponibilidad en las franjas de su turno '%4'.")
                    .arg(mc.materiaIDx).arg(cu.anio).arg(cu.seccion).arg(cu.turno);
        }
    }
    return {};
}

/// V14. Suma de bloques del curso <= n_dias * franjas del turno.
QString validarFranjasUtiles(const SolverConfig& c)
{
    for (const CursoSolverConfig& cu : c.cursos) {
        const TurnoConfig* turno = buscarTurno(c, cu.turno);
        if (!turno) continue;
        int totalBloques = 0;
        for (const PlanMateriaConfig& mc : materiasDelCurso(c, cu))
            totalBloques += bloques(mc.horasSemanales, c.franja_horaria.duracion_minutos);
        const int franjasUtiles = c.dimensiones.num_dias * turno->slot.size();
        if (totalBloques > franjasUtiles)
            return QStringLiteral("V14: el curso %1%2 necesita %3 bloques y solo tiene %4 franjas utiles.")
                .arg(cu.anio).arg(cu.seccion).arg(totalBloques).arg(franjasUtiles);
    }
    return {};
}

/// V15. Modo de aulas moviles exige que el centro tenga aulas.
QString validarAulasMoviles(const SolverConfig& c)
{
    if (!c.aula_fija && c.aulas.isEmpty())
        return QStringLiteral("V15: modo de aulas moviles sin aulas disponibles.");
    return {};
}

} // namespace

Resultado<SolverConfig> SolverConfig::fromJson(const QJsonObject& obj)
{
    SolverConfig config;
    config.version        = leerTexto(obj, "version");
    config.aula_fija      = obj.contains(QLatin1String("aula_fija"))
                                ? obj.value(QLatin1String("aula_fija")).toBool(true)
                                : true;
    config.dimensiones    = parsearDimensiones(obj["dimensiones"].toObject());
    config.franja_horaria = parsearFranjaHoraria(obj["franja_horaria"].toObject());
    config.turnos         = parsearTurnos(obj["turnos"].toObject());
    config.eventos        = parsearEventos(obj["eventos"].toArray());
    config.planes         = parsearPlanes(obj["planes"].toArray());
    config.cursos         = parsearCursos(obj["cursos"].toArray());
    config.profesores     = parsearProfesores(obj["profesores"].toArray());
    config.materias       = parsearMaterias(obj["materias"].toArray());
    config.aulas          = parsearAulas(obj["aulas"].toArray());

    const QString v1 = validarEstructura(obj);
    if (!v1.isEmpty())
        return Resultado<SolverConfig>::error(v1);

    const QStringList errores = {
        validarTiposYRangos(config),       validarDimensiones(config),       validarIndicesMaterias(config),
        validarIndicesProfesores(config),  validarIndicesAulas(config),      validarTurnosDeCursos(config),
        validarAulasDeCursos(config),      validarNumeroEstudiantes(config), validarListasNoVacias(config),
        validarUnPlanPorCurso(config),     validarDisponibilidad(config),    validarCoherenciaHoras(config),
        validarCandidatosElegibles(config), validarFranjasUtiles(config),    validarAulasMoviles(config)};
    for (const QString& error : errores)
        if (!error.isEmpty())
            return Resultado<SolverConfig>::error(error);

    return Resultado<SolverConfig>::exito(config);
}

QJsonObject SolverConfig::toJson() const
{
    QJsonObject obj;
    // Al PRODUCIR/serializar se sella SIEMPRE la version 2.0 (nunca vacio ni 1.0). La LECTURA
    // rechaza ausente, 1.0 o no reconocida (RF-1, RF-2).
    obj["version"]        = QStringLiteral("2.0");
    obj["aula_fija"]      = aula_fija;
    obj["dimensiones"]    = aJson(dimensiones);
    obj["franja_horaria"] = aJson(franja_horaria);
    obj["turnos"]         = aJson(turnos);
    obj["eventos"]        = aJson(eventos);
    obj["planes"]         = aJson(planes);
    obj["cursos"]         = aJson(cursos);
    obj["profesores"]     = aJson(profesores);
    obj["materias"]       = aJson(materias);
    obj["aulas"]          = aJson(aulas);
    return obj;
}
