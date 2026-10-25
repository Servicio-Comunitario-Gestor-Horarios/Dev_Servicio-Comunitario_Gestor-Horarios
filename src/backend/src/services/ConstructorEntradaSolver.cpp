#include "backend/services/ConstructorEntradaSolver.hpp"

#include "backend/services/ServicioCursos.hpp"

#include <QJsonArray>
#include <QTime>
#include <QVector>

#include <algorithm>

namespace
{
    /// Turno del dominio con su offset absoluto de slots dentro del día.
    struct TurnoOrdenado
    {
        QString nombre;
        QTime   inicio;
        QTime   fin;
        int     numSlots = 0;
        int     offset = 0;
    };

    /// Ordena los turnos por hora de inicio y calcula el offset de slots de cada
    /// uno (los slots de un turno continúan donde terminan los del anterior).
    QVector<TurnoOrdenado> ordenarTurnos(const QVector<TurnoDTO>& turnos)
    {
        QVector<TurnoOrdenado> ordenados;
        ordenados.reserve(turnos.size());
        for (const TurnoDTO& t : turnos)
            ordenados.append(TurnoOrdenado{t.nombre, t.inicio, t.fin, t.numSlots, 0});

        std::sort(ordenados.begin(), ordenados.end(),
                  [](const TurnoOrdenado& a, const TurnoOrdenado& b) {
                      return a.inicio < b.inicio;
                  });

        int offset = 0;
        for (TurnoOrdenado& t : ordenados)
        {
            t.offset = offset;
            offset += t.numSlots;
        }
        return ordenados;
    }

    /// Índice del aula con `id` en `aulas`, o -1 si no está.
    int indiceAula(const QVector<AulaDTO>& aulas, int id)
    {
        for (int i = 0; i < aulas.size(); ++i)
            if (aulas[i].id == id)
                return i;
        return -1;
    }

    /// Índice de la materia con `id` en el pool global, o -1 si no está.
    int indiceMateria(const QVector<MateriaDTO>& materias, int id)
    {
        for (int i = 0; i < materias.size(); ++i)
            if (materias[i].id == id)
                return i;
        return -1;
    }

    /// Indice de la materia con ese `nombre` en el pool global, o -1.
    int indiceMateriaPorNombre(const QVector<MateriaDTO>& materias, const QString& nombre)
    {
        for (int i = 0; i < materias.size(); ++i)
            if (materias[i].nombre == nombre)
                return i;
        return -1;
    }

    /// Curso del preset casado por identidad (anio+seccion), o nullptr.
    const CursoSolverConfig* parametrosDeCurso(const SolverConfig& parametros,
                                               int anio, const QString& seccion)
    {
        for (const CursoSolverConfig& c : parametros.cursos)
            if (c.anio == anio && c.seccion == seccion)
                return &c;
        return nullptr;
    }

    /// Plan compuesto del dominio con ese codigo, o nullptr.
    const PlanEstudio* planPorCodigo(const QVector<PlanEstudio>& planes, const QString& codigo)
    {
        for (const PlanEstudio& p : planes)
            if (p.codigo == codigo)
                return &p;
        return nullptr;
    }

    /// Id del docente con `nombre`, o cadena vacía. Se usa para casar el preset.
    const ProfesorSolverConfig* parametrosDeDocente(const SolverConfig& parametros,
                                                    const QString& nombre)
    {
        for (const ProfesorSolverConfig& p : parametros.profesores)
            if (p.nombre == nombre)
                return &p;
        return nullptr;
    }

    /// Slots (absolutos del día) cubiertos por una franja de disponibilidad.
    ///
    /// La franja declara el horario de reloj en que el docente está disponible y
    /// puede abarcar VARIOS turnos (p. ej. 07:00–18:00 cubre mañana y tarde). Se
    /// emite cada bloque —anclado en el `inicio` de su turno y avanzando en pasos
    /// de `duracionMinutos`, sin que el receso desplace bloques— que cae
    /// ÍNTEGRAMENTE dentro de la franja. Es la misma semántica que usa la
    /// cobertura del seed («la franja contiene un bloque completo»).
    QVector<int> slotsDeFranja(const FranjaHoraria& franja,
                               const QVector<TurnoOrdenado>& turnos,
                               int duracionMinutos)
    {
        QVector<int> listaSlots;
        if (duracionMinutos <= 0 || !franja.inicio.isValid() || !franja.fin.isValid())
            return listaSlots;

        const int paso = duracionMinutos * 60;
        for (const TurnoOrdenado& t : turnos)
        {
            for (int k = 0; k < t.numSlots; ++k)
            {
                const QTime bloqueInicio = t.inicio.addSecs(k * paso);
                const QTime bloqueFin    = bloqueInicio.addSecs(paso);
                if (bloqueInicio >= franja.inicio && bloqueFin <= franja.fin)
                    listaSlots.append(t.offset + k);
            }
        }
        return listaSlots;
    }

    QJsonArray intArray(const QVector<int>& valores)
    {
        QJsonArray arr;
        for (int v : valores)
            arr.append(v);
        return arr;
    }
} // namespace

Resultado<QJsonObject> construirJsonEntrada(const DatosDominio& datos,
                                            const SolverConfig& parametros,
                                            const QDateTime& ahora)
{
    // 1. Tipos obligatorios (RF-1). Orden determinista: aulas y luego materias, en el orden de la
    //    instantánea; el primer defecto detiene la construcción y no se emite JSON.
    for (const AulaDTO& a : datos.aulas)
        if (a.tipoAula.trimmed().isEmpty())
            return Resultado<QJsonObject>::error(
                QStringLiteral("El aula «%1» no tiene tipo de aula. Asígnelo en el catálogo de "
                               "tipos antes de generar el horario.")
                    .arg(a.nombre));

    for (const MateriaDTO& m : datos.materias)
        if (m.tipoAula.trimmed().isEmpty())
            return Resultado<QJsonObject>::error(
                QStringLiteral("La materia «%1» no tiene tipo de aula requerido. Asígnelo antes "
                               "de generar el horario.")
                    .arg(m.nombre));

    const QVector<TurnoOrdenado> turnosOrdenados = ordenarTurnos(datos.turnos);

    SolverConfig salida;

    // Parametros del solver que no son entidad. La entrada es SIEMPRE del contrato 2.0: la
    // version no se copia del preset (RF-1).
    salida.version        = QStringLiteral("2.0");
    salida.aula_fija      = parametros.aula_fija;      // modo GLOBAL (del preset)
    salida.franja_horaria = parametros.franja_horaria;

    // Materias: pool global en el orden de la instantánea; el tipo viaja por su nombre.
    for (const MateriaDTO& m : datos.materias)
        salida.materias.append(MateriaSolverConfig{m.nombre, m.tipoAula});

    // Aulas: el tipo de aula sale de los datos, sin valor por defecto.
    for (const AulaDTO& a : datos.aulas)
    {
        AulaSolverConfig aula;
        aula.nombre    = a.nombre;
        aula.tipo      = a.tipoAula;
        aula.capacidad = a.capacidad;
        salida.aulas.append(aula);
    }

    // Turnos: slots absolutos por orden de inicio + ventana del turno para V2.
    for (const TurnoOrdenado& t : turnosOrdenados)
    {
        TurnoConfig turno;
        turno.nombre = t.nombre;
        for (int k = 0; k < t.numSlots; ++k)
            turno.slot.append(t.offset + k);
        turno.inicio = t.inicio.isValid() ? t.inicio.toString(QStringLiteral("HH:mm")) : QString();
        turno.fin    = t.fin.isValid() ? t.fin.toString(QStringLiteral("HH:mm")) : QString();
        salida.turnos.turnos.append(turno);
    }

    // Eventos del turno (contrato 2.0): una entrada por evento, sin identidad.
    for (const TurnoDTO& t : datos.turnos)
    {
        for (const EventoDTO& evento : t.eventos)
        {
            EventoConfig config;
            config.nombre          = evento.nombre;
            config.turno           = t.nombre;
            config.despues_de_slot = evento.despuesDeSlot;
            config.orden           = evento.orden;
            config.duracion        = evento.duracion;
            config.inicio          = evento.inicio.isValid()
                                         ? evento.inicio.toString(QStringLiteral("HH:mm"))
                                         : QString();
            config.fin             = evento.fin.isValid()
                                         ? evento.fin.toString(QStringLiteral("HH:mm"))
                                         : QString();
            salida.eventos.append(config);
        }
    }

    // Cursos: de la BD (anio/seccion/turno/aula); del preset (plan/num_estudiantes).
    for (const CursoDTO& c : datos.cursos)
    {
        CursoSolverConfig curso;
        curso.anio    = c.anio;
        curso.seccion = c.seccion;
        curso.turno   = c.turno;   // base (spec 005 I1); el preset puede sobrescribirlo
        if (const CursoSolverConfig* pc = parametrosDeCurso(parametros, c.anio, c.seccion))
        {
            curso.plan            = pc->plan;
            curso.num_estudiantes = pc->num_estudiantes;
            // El turno del curso puede aportarse desde el preset (guía de datos de
            // prueba del frontend, §4: los cursos del centro se siembran sin turno).
            // Si el preset no lo declara, se conserva el de la base (spec 005 I1).
            if (!pc->turno.isEmpty())
                curso.turno = pc->turno;
        }
        // Aula: indice en modo fijo; -1 en modo movil (la decide el solver).
        curso.aula = salida.aula_fija ? indiceAula(datos.aulas, c.aula) : -1;
        salida.cursos.append(curso);
    }

    // Planes EMBEBIDOS: solo los referenciados por los cursos.
    for (const CursoSolverConfig& c : salida.cursos)
    {
        bool ya = false;
        for (const PlanSolverConfig& p : salida.planes)
            if (p.codigo == c.plan) { ya = true; break; }
        if (ya) continue;

        const PlanEstudio* plan = planPorCodigo(datos.planes, c.plan);
        if (!plan) continue;   // V10/V11 lo rechazaran

        PlanSolverConfig embebido;
        embebido.codigo = plan->codigo;
        for (const MateriaEnPlan& m : plan->materias)
        {
            const int idx = indiceMateriaPorNombre(datos.materias, m.materia.nombre);
            if (idx >= 0)
                embebido.materias.append(PlanMateriaConfig{idx, m.curso, m.horas});
        }
        salida.planes.append(embebido);
    }

    // Profesores: entidad del dominio + parámetros del solver casados por nombre.
    for (const ProfesorDTO& d : datos.docentes)
    {
        ProfesorSolverConfig prof;
        prof.nombre = d.nombre;

        if (const ProfesorSolverConfig* p = parametrosDeDocente(parametros, d.nombre))
        {
            prof.horas_requeridas = p->horas_requeridas;
            prof.horas_aula       = p->horas_aula;
            prof.turno            = p->turno;
            prof.plan             = p->plan;
            prof.anios            = p->anios;
            prof.materias_suplente = p->materias_suplente;
        }

        for (const MateriaAsignadaDTO& m : d.materias)
        {
            const int idx = indiceMateria(datos.materias, m.id);
            if (idx >= 0)
                prof.materias_asignadas.append(idx);
        }

        for (const DisponibilidadDTO& disp : d.disponibilidad)
        {
            Disponibilidad sol;
            sol.dia  = disp.franja.dia - 1;   // dominio: 0=domingo; solver: 0=lunes
            sol.slot = slotsDeFranja(disp.franja, turnosOrdenados,
                                     parametros.franja_horaria.duracion_minutos);
            if (sol.dia >= 0)
                prof.disponibilidad.append(sol);
        }

        salida.profesores.append(prof);
    }

    // Dimensiones derivadas (conteos) + días/slots desde el preset.
    salida.dimensiones.num_profesores = salida.profesores.size();
    salida.dimensiones.num_materias   = salida.materias.size();
    salida.dimensiones.num_aulas      = salida.aulas.size();
    salida.dimensiones.num_cursos     = salida.cursos.size();
    salida.dimensiones.num_dias       = parametros.dimensiones.num_dias;

    int totalSlots = 0;
    for (const TurnoOrdenado& t : turnosOrdenados)
        totalSlots += t.numSlots;
    salida.dimensiones.num_slots_dia =
        parametros.dimensiones.num_slots_dia > 0 ? parametros.dimensiones.num_slots_dia
                                                 : totalSlots;

    // JSON final: 13 secciones (12 del SolverConfig + `meta`).
    QJsonObject json = salida.toJson();

    QJsonObject meta;
    meta["descripcion"] = QStringLiteral("Entrada del solver generada por el cliente");
    meta["fecha_modificacion"] =
        ahora.isValid() ? ahora.toString(Qt::ISODate) : QString();
    json["meta"] = meta;

    return Resultado<QJsonObject>::exito(json);
}

Resultado<QJsonObject> ConstructorEntradaSolver::construir(const DatosDominio& datos,
                                                           const SolverConfig& parametros,
                                                           const QDateTime& ahora)
{
    return construirJsonEntrada(datos, parametros, ahora);
}
