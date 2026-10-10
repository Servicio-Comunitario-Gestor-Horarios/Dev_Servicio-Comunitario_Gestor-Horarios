#include "backend/services/ConstructorEntradaSolver.hpp"

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
    QVector<int> slotsDeFranja(const FranjaHoraria& franja,
                               const QVector<TurnoOrdenado>& turnos,
                               int duracionMinutos)
    {
        QVector<int> listaSlots;
        if (duracionMinutos <= 0)
            return listaSlots;

        for (const TurnoOrdenado& t : turnos)
        {
            if (franja.inicio >= t.inicio && franja.fin <= t.fin)
            {
                const int desde = t.inicio.secsTo(franja.inicio) / (duracionMinutos * 60);
                const int cuantos = franja.inicio.secsTo(franja.fin) / (duracionMinutos * 60);
                for (int k = 0; k < cuantos; ++k)
                    listaSlots.append(t.offset + desde + k);
                return listaSlots;
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

QJsonObject construirJsonEntrada(const DatosDominio& datos,
                                 const SolverConfig& parametros,
                                 const QDateTime& ahora)
{
    const QVector<TurnoOrdenado> turnosOrdenados = ordenarTurnos(datos.turnos);

    SolverConfig salida;

    // Parámetros del solver que no son entidad.
    salida.version        = parametros.version;
    salida.franja_horaria = parametros.franja_horaria;
    salida.planificacion  = parametros.planificacion;
    salida.generacion     = parametros.generacion;
    salida.penalizaciones = parametros.penalizaciones;

    // Materias: pool global en el orden de la instantánea.
    for (const MateriaDTO& m : datos.materias)
        salida.materias.append(MateriaSolverConfig{m.nombre});

    // Aulas.
    for (const AulaDTO& a : datos.aulas)
    {
        AulaSolverConfig aula;
        aula.nombre    = a.nombre;
        aula.tipo      = QStringLiteral("regular");   // el dominio no distingue tipo aún
        aula.capacidad = a.capacidad;
        salida.aulas.append(aula);
    }

    // Turnos: slots absolutos por orden de inicio.
    for (const TurnoOrdenado& t : turnosOrdenados)
    {
        TurnoConfig turno;
        turno.nombre = t.nombre;
        for (int k = 0; k < t.numSlots; ++k)
            turno.slot.append(t.offset + k);
        salida.turnos.turnos.append(turno);
    }

    // Recesos: se aplanan desde los turnos del dominio.
    for (const TurnoDTO& t : datos.turnos)
    {
        for (const RecesoDTO& r : t.recesos)
        {
            RecesoConfig receso;
            receso.turno           = t.nombre;
            receso.despues_de_slot = r.despuesDeSlot;
            receso.duracion        = r.duracion;
            receso.inicio          = r.inicio.isValid() ? r.inicio.toString(QStringLiteral("HH:mm")) : QString();
            receso.fin             = r.fin.isValid() ? r.fin.toString(QStringLiteral("HH:mm")) : QString();
            salida.recesos.append(receso);
        }
    }

    // Cursos.
    for (const CursoDTO& c : datos.cursos)
    {
        CursoSolverConfig curso;
        curso.nombre          = c.nombre;
        curso.turno           = c.turno;
        curso.aula_fija       = indiceAula(datos.aulas, c.aulaFija);
        curso.num_estudiantes = c.numEstudiantes;
        curso.plan            = c.codigoPlan;

        for (const CursoMateriaDTO& cm : c.materias)
        {
            MateriaCurso mc;
            mc.materiaIDx      = indiceMateria(datos.materias, cm.idMateria);
            mc.horasSemanales  = cm.horasSemanales;
            curso.materias.append(mc);
        }
        salida.cursos.append(curso);
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

    return json;
}

QJsonObject ConstructorEntradaSolver::construir(const DatosDominio& datos,
                                                const SolverConfig& parametros,
                                                const QDateTime& ahora)
{
    return construirJsonEntrada(datos, parametros, ahora);
}
