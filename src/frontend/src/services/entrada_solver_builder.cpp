#include "services/entrada_solver_builder.hpp"

#if defined(GESTOR_TIENE_BACKEND)

#include "backend/services/ConstructorEntradaSolver.hpp"
#include "backend/services/NucleoDatos.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/config/solver_config.hpp"

#include <QDateTime>
#include <QJsonArray>

namespace GeneracionSolver {
namespace {

/// Índice en el pool global de la materia con ese nombre, o -1.
int indiceMateriaPorNombre(const QVector<MateriaDTO>& materias, const QString& nombre)
{
    const QString buscado = nombre.trimmed();
    for (int i = 0; i < materias.size(); ++i)
        if (QString::compare(materias[i].nombre.trimmed(), buscado, Qt::CaseInsensitive) == 0)
            return i;
    return -1;
}

/// Inserta (o reemplaza) un bloque en la rejilla `schedule` (día -> slot -> bloque).
void insertarEnHorario(QJsonObject& schedule, int dia, int slot, const QJsonObject& bloque)
{
    const QString claveDia = QString::number(dia);
    const QString claveSlot = QString::number(slot);
    QJsonObject diaObj = schedule.value(claveDia).toObject();
    diaObj.insert(claveSlot, bloque);
    schedule.insert(claveDia, diaObj);
}

/// Lee el dominio completo del núcleo, acumulando avisos en `comp`.
DatosDominio leerDominio(const NucleoDatos& nucleo, Composicion& comp)
{
    DatosDominio datos;

    if (const auto r = nucleo.listarDocentes(); r.ok) datos.docentes = r.valor;
    else comp.avisos << QStringLiteral("No se pudieron leer los docentes: %1").arg(r.mensajeError);

    if (const auto r = nucleo.listarAulas(); r.ok) datos.aulas = r.valor;
    else comp.avisos << QStringLiteral("No se pudieron leer las aulas: %1").arg(r.mensajeError);

    if (const auto r = nucleo.listarMaterias(); r.ok) datos.materias = r.valor;
    else comp.avisos << QStringLiteral("No se pudieron leer las materias: %1").arg(r.mensajeError);

    // Los planes EMBEBIDOS se componen aparte, y solo los que referencian los
    // cursos (ver `construirEntrada`): `DatosDominio::planes` usa `PlanEstudio`.

    if (const auto r = nucleo.listarCursos(); r.ok) datos.cursos = r.valor;
    else comp.avisos << QStringLiteral("No se pudieron leer los cursos: %1").arg(r.mensajeError);

    if (const auto r = nucleo.listarTurnos(); r.ok) datos.turnos = r.valor;
    else comp.avisos << QStringLiteral("No se pudieron leer los turnos: %1").arg(r.mensajeError);

    return datos;
}

/// Rejilla real con la que se generó el horario, leída de la entrada ya
/// construida (evita duplicar la lógica del contrato). El inicio de cada turno
/// no viaja al solver, así que se toma del dominio por nombre.
CuadriculaVisual leerCuadricula(const QJsonObject& entrada, const QVector<TurnoDTO>& turnos)
{
    CuadriculaVisual c;
    c.duracionMinutos = entrada.value(QStringLiteral("franja_horaria")).toObject()
                            .value(QStringLiteral("duracion_minutos")).toInt(0);
    const QJsonObject dimensiones = entrada.value(QStringLiteral("dimensiones")).toObject();
    c.numDias = dimensiones.value(QStringLiteral("num_dias")).toInt(5);
    c.numSlotsDia = dimensiones.value(QStringLiteral("num_slots_dia")).toInt(0);

    QMap<QString, QString> inicioPorTurno;
    for (const TurnoDTO& t : turnos)
        if (t.inicio.isValid())
            inicioPorTurno.insert(t.nombre, t.inicio.toString(QStringLiteral("HH:mm")));

    const QJsonArray turnosJson = entrada.value(QStringLiteral("turnos")).toObject()
                                      .value(QStringLiteral("turnos")).toArray();
    for (const QJsonValue& v : turnosJson) {
        const QJsonObject o = v.toObject();
        TurnoVisual t;
        t.nombre = o.value(QStringLiteral("nombre")).toString();
        const QJsonArray franjas = o.value(QStringLiteral("slot")).toArray();
        for (const QJsonValue& s : franjas)
            t.franjas.append(s.toInt());
        t.inicio = inicioPorTurno.value(t.nombre);
        c.turnos.append(t);
    }

    const QJsonArray recesosJson = entrada.value(QStringLiteral("recesos")).toArray();
    for (const QJsonValue& v : recesosJson) {
        const QJsonObject o = v.toObject();
        RecesoVisual r;
        r.turno = o.value(QStringLiteral("turno")).toString();
        r.despuesDeSlot = o.value(QStringLiteral("despues_de_slot")).toInt(0);
        r.duracion = o.value(QStringLiteral("duracion")).toInt(0);
        r.inicio = o.value(QStringLiteral("inicio")).toString();
        r.fin = o.value(QStringLiteral("fin")).toString();
        c.recesos.append(r);
    }

    return c;
}

/// Serializa la rejilla para que la visualización la lea del propio horario.
QJsonObject cuadriculaAJson(const CuadriculaVisual& c)
{
    QJsonObject out;
    out.insert(QStringLiteral("duracion_minutos"), c.duracionMinutos);
    out.insert(QStringLiteral("num_dias"), c.numDias);
    out.insert(QStringLiteral("num_slots_dia"), c.numSlotsDia);

    QJsonArray turnos;
    for (const TurnoVisual& t : c.turnos) {
        QJsonObject obj;
        obj.insert(QStringLiteral("nombre"), t.nombre);
        QJsonArray franjas;
        for (int slot : t.franjas)
            franjas.append(slot);
        obj.insert(QStringLiteral("slot"), franjas);
        if (!t.inicio.isEmpty())
            obj.insert(QStringLiteral("inicio"), t.inicio);
        turnos.append(obj);
    }
    out.insert(QStringLiteral("turnos"), turnos);

    QJsonArray recesos;
    for (const RecesoVisual& r : c.recesos) {
        QJsonObject obj;
        obj.insert(QStringLiteral("turno"), r.turno);
        obj.insert(QStringLiteral("despues_de_slot"), r.despuesDeSlot);
        obj.insert(QStringLiteral("duracion"), r.duracion);
        if (!r.inicio.isEmpty()) obj.insert(QStringLiteral("inicio"), r.inicio);
        if (!r.fin.isEmpty())    obj.insert(QStringLiteral("fin"), r.fin);
        recesos.append(obj);
    }
    out.insert(QStringLiteral("recesos"), recesos);
    return out;
}

} // namespace

/// ¿El plan `codigo` (ya compuesto) tiene materias para el año `anio`?
bool planTieneMaterias(const QVector<PlanEstudio>& planes, const QString& codigo, int anio)
{
    for (const PlanEstudio& plan : planes) {
        if (plan.codigo != codigo)
            continue;
        for (const MateriaEnPlan& m : plan.materias)
            if (m.curso == anio)
                return true;
        return false;
    }
    return false;
}

Composicion construirEntrada(const NucleoDatos& nucleo, const OpcionesUI& opciones)
{
    Composicion comp;
    DatosDominio datos = leerDominio(nucleo, comp);

    // ── Planes EMBEBIDOS: solo los referenciados por los cursos ───────────
    // El contrato nuevo espera `planes` como `PlanEstudio` (materias + horas),
    // así que se recomponen desde la base en vez de usar los encabezados.
    QStringList codigosPlan;
    for (const CursoDTO& c : datos.cursos) {
        const QString plan = opciones.cursos.value(nombreCurso(c.anio, c.seccion)).plan;
        if (!plan.isEmpty() && !codigosPlan.contains(plan))
            codigosPlan << plan;
    }
    for (const QString& codigo : codigosPlan) {
        const auto plan = nucleo.componerPlanCompleto(codigo);
        if (plan.ok)
            datos.planes.append(plan.valor);
        else
            comp.avisos << QStringLiteral("No se pudo componer el plan \"%1\": %2")
                               .arg(codigo, plan.mensajeError);
    }

    // Horas sugeridas por docente (para valores por defecto válidos).
    const QMap<QString, int> sugeridas = horasSugeridasPorDocente(nucleo, opciones.cursos);

    // ── Preset del solver (lo que no es entidad) ──────────────────────────
    SolverConfig preset;
    preset.version = opciones.version;
    preset.aula_fija = opciones.usarAulasFijas;
    preset.franja_horaria.duracion_minutos = opciones.duracionMinutos;
    preset.dimensiones.num_dias = opciones.numDias;
    preset.dimensiones.num_slots_dia = opciones.numSlotsDia;

    // Cursos: identidad (año/sección) del dominio + plan/turno/estudiantes de la
    // UI. El aula la toma el constructor desde la base (modo global de aulas fijas).
    for (const CursoDTO& c : datos.cursos) {
        const QString nombre = nombreCurso(c.anio, c.seccion);
        const ParametrosCurso pc = opciones.cursos.value(nombre);

        CursoSolverConfig curso;
        curso.anio = c.anio;
        curso.seccion = c.seccion;
        curso.plan = pc.plan;
        curso.turno = pc.turno;
        curso.num_estudiantes = pc.numEstudiantes;
        preset.cursos.append(curso);

        if (pc.plan.isEmpty()) {
            comp.avisos << QStringLiteral("El curso \"%1\" no tiene plan asignado.").arg(nombre);
        } else if (!planTieneMaterias(datos.planes, pc.plan, c.anio)) {
            comp.avisos << QStringLiteral("El plan \"%1\" no tiene materias para el año %2 "
                                          "(curso \"%3\").")
                               .arg(pc.plan)
                               .arg(c.anio)
                               .arg(nombre);
        }
    }

    // Docentes: nombre del dominio + parámetros de la UI (casados por nombre).
    for (const ProfesorDTO& d : datos.docentes) {
        ProfesorSolverConfig p;
        p.nombre = d.nombre;

        const ParametrosDocente pd = opciones.docentes.value(d.nombre);
        const int sugerido = sugeridas.value(d.nombre, 0);
        int requeridas = pd.horasRequeridas > 0 ? pd.horasRequeridas
                                                : (sugerido > 0 ? sugerido : 1);
        int enAula = pd.horasAula > 0 ? pd.horasAula : requeridas;
        if (enAula > requeridas) enAula = requeridas;

        p.horas_requeridas = requeridas;
        p.horas_aula = enAula;
        p.turno = pd.turno;
        p.plan = pd.plan;
        p.anios = pd.anios;
        for (const QString& nombreSuplente : pd.materiasSuplentes) {
            const int idx = indiceMateriaPorNombre(datos.materias, nombreSuplente);
            if (idx >= 0) p.materias_suplente.append(MateriaSuplente{idx, 1});
        }
        preset.profesores.append(p);
    }

    // ── Entrada (dominio + preset) ────────────────────────────────────────
    // `construirJsonEntrada` embebe los planes referenciados, casa los cursos
    // por año/sección, aplica el modo global de aulas y deriva dimensiones,
    // turnos/recesos y disponibilidad.
    comp.entrada = construirJsonEntrada(datos, preset, QDateTime::currentDateTime());

    // ── Contexto para traducir índices a nombres al visualizar ────────────
    for (const MateriaDTO& m : datos.materias)
        comp.contexto.materias << m.nombre;
    for (const ProfesorDTO& d : datos.docentes)
        comp.contexto.profesores << d.nombre;
    for (const AulaDTO& a : datos.aulas)
        comp.contexto.aulas << a.nombre;
    for (const CursoDTO& c : datos.cursos)
        comp.contexto.cursos.insert(claveCurso(c.anio, c.seccion),
                                    qMakePair(c.anio, c.seccion));

    // Rejilla real (franjas, duración y recesos) para que la visualización se
    // dibuje con los datos del motor y no con una plantilla fija.
    comp.contexto.cuadricula = leerCuadricula(comp.entrada, datos.turnos);

    return comp;
}

QMap<QString, int> horasSugeridasPorDocente(
    const NucleoDatos& nucleo,
    const QMap<QString, ParametrosCurso>& cursos)
{
    QMap<QString, int> resultado;

    const auto docentesR = nucleo.listarDocentes();
    const auto cursosR = nucleo.listarCursos();
    if (!docentesR.ok || !cursosR.ok)
        return resultado;

    for (const CursoDTO& c : cursosR.valor) {
        const QString nombre = nombreCurso(c.anio, c.seccion);
        const QString plan = cursos.value(nombre).plan;
        if (plan.isEmpty())
            continue;

        const auto cargaR = nucleo.listarCargaDePlan(plan);
        if (!cargaR.ok)
            continue;

        QVector<QPair<int, int>> materiasDelAnio; // (idMateria, horas)
        for (const CargaHorariaDTO& ch : cargaR.valor)
            if (ch.curso == c.anio)
                materiasDelAnio.append(qMakePair(ch.idMateria, ch.horas));
        if (materiasDelAnio.isEmpty())
            continue;

        for (const ProfesorDTO& d : docentesR.valor) {
            int horas = 0;
            for (const MateriaAsignadaDTO& ma : d.materias)
                for (const auto& par : materiasDelAnio)
                    if (par.first == ma.id)
                        horas += par.second;
            if (horas > 0)
                resultado[d.nombre] += horas;
        }
    }
    return resultado;
}

QJsonObject aFormatoVisualizacion(const HorarioSalida& horario,
                                  const ContextoHorario& contexto)
{
    // Rango válido: el declarado por la configuración (la entrada del solver), no
    // una plantilla fija. Así las franjas de la tarde (slots 12+) no se pierden.
    const int numDias = contexto.cuadricula.numDias > 0 ? contexto.cuadricula.numDias : 5;
    const int numSlotsDia = contexto.cuadricula.numSlotsDia;

    // Un horario por docente (mismo orden que el contexto) y uno por curso.
    QVector<QJsonObject> horarioPorDocente(contexto.profesores.size());
    QMap<QString, QJsonObject> horarioPorCurso;
    for (auto it = contexto.cursos.constBegin(); it != contexto.cursos.constEnd(); ++it)
        horarioPorCurso.insert(it.key(), QJsonObject());

    for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd(); ++it) {
        const QString clave = it.key(); // claveCurso: "año|sección"
        const CursoOutput& curso = it.value();

        const auto cursoCtx = contexto.cursos.constFind(clave);
        const QString nombreVisible = cursoCtx != contexto.cursos.constEnd()
                                          ? nombreCurso(cursoCtx->first, cursoCtx->second)
                                          : clave;

        QJsonObject& horarioCurso = horarioPorCurso[clave];

        for (const DiaOutput& dia : curso.dias) {
            const int diaVisual = dia.dia + 1; // solver: 0=lunes -> visual: 1=lunes
            if (diaVisual < 1 || diaVisual > numDias)
                continue;

            for (const AsignacionOutput& a : dia.asignaciones) {
                if (a.slot < 0)
                    continue;
                if (numSlotsDia > 0 && a.slot >= numSlotsDia)
                    continue;

                const QString materia = contexto.materias.value(a.materia);
                const QString profesor = contexto.profesores.value(a.profesor);
                const QString aula = contexto.aulas.value(a.aula);

                QJsonObject bloqueCurso;
                bloqueCurso.insert(QStringLiteral("materia"), materia);
                if (!profesor.isEmpty()) bloqueCurso.insert(QStringLiteral("profesor"), profesor);
                if (!aula.isEmpty()) bloqueCurso.insert(QStringLiteral("aula"), aula);
                insertarEnHorario(horarioCurso, diaVisual, a.slot, bloqueCurso);

                if (a.profesor >= 0 && a.profesor < horarioPorDocente.size()) {
                    QJsonObject bloqueDocente = bloqueCurso;
                    bloqueDocente.insert(QStringLiteral("curso"), nombreVisible);
                    insertarEnHorario(horarioPorDocente[a.profesor], diaVisual, a.slot,
                                      bloqueDocente);
                }
            }
        }
    }

    // ── Docentes ──────────────────────────────────────────────────────────
    QJsonArray teachers;
    for (int i = 0; i < contexto.profesores.size(); ++i) {
        QJsonObject t;
        t.insert(QStringLiteral("name"), contexto.profesores[i]);
        t.insert(QStringLiteral("schedule"), horarioPorDocente[i]);
        teachers.append(t);
    }

    // ── Años y secciones ──────────────────────────────────────────────────
    QMap<int, QJsonArray> seccionesPorAnio;
    for (auto it = contexto.cursos.constBegin(); it != contexto.cursos.constEnd(); ++it) {
        const int anio = it->first;
        const QString seccion = it->second;

        QJsonObject sec;
        sec.insert(QStringLiteral("name"), nombreCurso(anio, seccion));
        sec.insert(QStringLiteral("schedule"), horarioPorCurso.value(it.key()));
        // El turno del curso decide qué franjas se dibujan en su rejilla.
        const auto cursoSalida = horario.horarios.constFind(it.key());
        if (cursoSalida != horario.horarios.constEnd() && !cursoSalida->turno.isEmpty())
            sec.insert(QStringLiteral("turno"), cursoSalida->turno);
        seccionesPorAnio[anio].append(sec);
    }

    QJsonArray years;
    for (auto it = seccionesPorAnio.constBegin(); it != seccionesPorAnio.constEnd(); ++it) {
        QJsonObject y;
        y.insert(QStringLiteral("year"), it.key());
        y.insert(QStringLiteral("name"), QStringLiteral("%1° Año").arg(it.key()));
        y.insert(QStringLiteral("sections"), it.value());
        years.append(y);
    }

    QJsonObject root;
    root.insert(QStringLiteral("teachers"), teachers);
    root.insert(QStringLiteral("years"), years);

    QJsonObject meta;
    meta.insert(QStringLiteral("fecha_generacion"), horario.metadata.fecha_generacion);
    meta.insert(QStringLiteral("total_asignaciones"), horario.metadata.total_asignaciones);
    meta.insert(QStringLiteral("cursos_generados"), horario.metadata.cursos_generados);
    root.insert(QStringLiteral("_meta"), meta);

    // Rejilla temporal real: la visualización la usa para pintar las franjas
    // (duración del bloque), los turnos y sus recesos.
    if (contexto.cuadricula.valida())
        root.insert(QStringLiteral("_cuadricula"), cuadriculaAJson(contexto.cuadricula));

    return root;
}

} // namespace GeneracionSolver

#endif // GESTOR_TIENE_BACKEND
