#include "consola/ConsolaDatos.hpp"

#include "backend/services/NucleoDatos.hpp"
#include "datos/ContextoBaseDatos.hpp"

#include <QTextStream>
#include <QTime>

namespace
{
    /// Divide una línea en tokens respetando comillas dobles (para nombres con
    /// espacios: `docentes crear P1 "Ana Gómez" ana@u`).
    QStringList tokenizar(const QString& linea)
    {
        QStringList tokens;
        QString actual;
        bool enComillas = false;

        for (const QChar c : linea)
        {
            if (c == QLatin1Char('"'))
            {
                enComillas = !enComillas;
                continue;
            }
            if (c.isSpace() && !enComillas)
            {
                if (!actual.isEmpty())
                {
                    tokens << actual;
                    actual.clear();
                }
            }
            else
            {
                actual.append(c);
            }
        }
        if (!actual.isEmpty())
            tokens << actual;

        return tokens;
    }

    /// Texto uniforme de un `Resultado<T>`.
    template <typename T>
    QString resultadoATexto(const Resultado<T>& r, const QString& exito)
    {
        return r.ok ? QStringLiteral("ok: %1").arg(exito)
                    : QStringLiteral("error: %1").arg(r.mensajeError);
    }

    QString textoBool(bool ok, const QString& exito)
    {
        return ok ? QStringLiteral("ok: %1").arg(exito)
                  : QStringLiteral("error: no se pudo completar la operación (revise los cambios"
                                   " pendientes y los permisos)");
    }
} // namespace

ConsolaDatos::ConsolaDatos(ContextoBaseDatos& contexto,
                           QTextStream& salida,
                           QTextStream& entrada,
                           bool interactivo)
    : m_contexto(contexto)
    , m_salida(salida)
    , m_entrada(entrada)
    , m_interactivo(interactivo)
{
    registrarComandos();
}

NucleoDatos& ConsolaDatos::nucleo()
{
    return m_contexto.nucleo();
}

void ConsolaDatos::registrarComandos()
{
    m_comandos[QStringLiteral("docentes")]     = [this](const Args& a) { cmdDocentes(a); };
    m_comandos[QStringLiteral("aulas")]        = [this](const Args& a) { cmdAulas(a); };
    m_comandos[QStringLiteral("materias")]     = [this](const Args& a) { cmdMaterias(a); };
    m_comandos[QStringLiteral("planes")]       = [this](const Args& a) { cmdPlanes(a); };
    m_comandos[QStringLiteral("cursos")]       = [this](const Args& a) { cmdCursos(a); };
    m_comandos[QStringLiteral("turnos")]       = [this](const Args& a) { cmdTurnos(a); };
    m_comandos[QStringLiteral("dependientes")] = [this](const Args& a) { cmdDependientes(a); };
    m_comandos[QStringLiteral("cascada")]      = [this](const Args& a) { cmdCascada(a); };
    m_comandos[QStringLiteral("pendientes")]   = [this](const Args& a) { cmdPendientes(a); };
    m_comandos[QStringLiteral("reintentar")]   = [this](const Args& a) { cmdReintentar(a); };
    m_comandos[QStringLiteral("estado")]       = [this](const Args& a) { cmdEstado(a); };
    m_comandos[QStringLiteral("ayuda")]        = [this](const Args& a) { cmdAyuda(a); };
    m_comandos[QStringLiteral("help")]         = [this](const Args& a) { cmdAyuda(a); };
}

void ConsolaDatos::imprimir(const QString& texto)
{
    m_salida << texto << '\n';
}

bool ConsolaDatos::confirmar(const QString& pregunta, const Args& args)
{
    if (args.contains(QStringLiteral("--si")))
        return true;

    if (!m_interactivo)
    {
        imprimir(QStringLiteral("error: operación destructiva sin confirmar; añada el token --si"));
        return false;
    }

    m_salida << pregunta << QStringLiteral(" [s/N]: ");
    m_salida.flush();

    const QString respuesta = m_entrada.readLine().trimmed().toLower();
    return respuesta == QStringLiteral("s") || respuesta == QStringLiteral("si")
           || respuesta == QStringLiteral("sí") || respuesta == QStringLiteral("y");
}

void ConsolaDatos::imprimirBienvenida()
{
    imprimir(QStringLiteral("Consola de datos (Gestor-Horarios). Escriba 'ayuda' para ver los"
                            " comandos y 'salir' para terminar."));
}

void ConsolaDatos::imprimirAyuda()
{
    imprimir(QStringLiteral(R"(Comandos disponibles:

  estado                         Muestra la base abierta y si hay cambios pendientes.
  ayuda | help                   Muestra esta ayuda.
  salir | q                      Termina la consola.

  docentes  listar
  docentes  crear    <id> "<nombre>" <email> [telefono]
  docentes  actualizar <id> "<nombre>" <email> [telefono]
  docentes  eliminar <id>

  aulas     listar
  aulas     crear    "<nombre>" <capacidad> [edificio] [piso]
  aulas     actualizar <id> "<nombre>" <capacidad> [edificio] [piso]
  aulas     eliminar <id>

  materias  listar
  materias  crear    "<nombre>" ["<requisitos>"]
  materias  actualizar <id> "<nombre>" ["<requisitos>"]
  materias  eliminar <id>

  planes    listar
  planes    crear    <codigo> "<nombre>" ["<descripcion>"]
  planes    actualizar <codigo> "<nombre>" ["<descripcion>"]
  planes    eliminar <codigo>

  cursos    listar
  cursos    crear    "<nombre>" [turno] [aulaFija] [numEstudiantes] [codigoPlan]
  cursos    actualizar <id> "<nombre>" [turno] [aulaFija] [numEstudiantes] [codigoPlan]
  cursos    eliminar <id>
  cursos    asignar  <idCurso> <idMateria> <horasSemanales>
  cursos    quitar   <idCurso> <idMateria>

  turnos    listar
  turnos    crear    "<nombre>" <inicio HH:mm> <fin HH:mm> <numSlots>
  turnos    actualizar "<nombre>" <inicio HH:mm> <fin HH:mm> <numSlots>
  turnos    eliminar "<nombre>"
  turnos    receso-agregar  "<turno>" <despuesDeSlot> <duracion>
  turnos    receso-eliminar "<turno>" <despuesDeSlot>

  dependientes <dominio> <id>    Lista los registros que se eliminarían en cascada.
  cascada      <dominio> <id> [--si]
                                 Elimina en cascada (pide confirmación; --si la omite).
  pendientes                     Lista los cambios pendientes de reintento.
  reintentar   <id>              Reintenta un cambio pendiente.

Dominios válidos para 'dependientes'/'cascada': plan, materia, profesor, aula, curso, turno.)"));
}

bool ConsolaDatos::ejecutarLinea(const QString& linea)
{
    const Args args = tokenizar(linea.trimmed());
    if (args.isEmpty())
        return true;

    const QString comando = args.first().toLower();

    if (comando == QStringLiteral("salir") || comando == QStringLiteral("q")
        || comando == QStringLiteral("quit"))
        return false;

    const auto it = m_comandos.constFind(comando);
    if (it == m_comandos.constEnd())
    {
        imprimir(QStringLiteral("comando desconocido: %1 (use 'ayuda')").arg(comando));
        return true;
    }

    it.value()(args);
    return true;
}

// ─── Docentes ──────────────────────────────────────────────────────────────

void ConsolaDatos::cmdDocentes(const Args& args)
{
    if (args.size() < 2)
    {
        imprimir(QStringLiteral("Uso: docentes listar|crear|actualizar|eliminar ..."));
        return;
    }
    const QString accion = args[1].toLower();

    if (accion == QStringLiteral("listar"))
    {
        const auto r = nucleo().listarDocentes();
        if (!r.ok) { imprimir(QStringLiteral("error: %1").arg(r.mensajeError)); return; }
        imprimir(QStringLiteral("docentes: %1").arg(r.valor.size()));
        for (const ProfesorDTO& d : r.valor)
            imprimir(QStringLiteral("  %1 | %2 | %3 | %4 | materias=%5 disp=%6")
                         .arg(d.id, d.nombre, d.email.isEmpty() ? QStringLiteral("-") : d.email,
                              d.telefono.isEmpty() ? QStringLiteral("-") : d.telefono)
                         .arg(d.materias.size())
                         .arg(d.disponibilidad.size()));
        return;
    }
    if (accion == QStringLiteral("crear"))
    {
        if (args.size() < 5) { imprimir(QStringLiteral("Uso: docentes crear <id> \"<nombre>\" <email> [telefono]")); return; }
        const QString telefono = args.size() > 5 ? args[5] : QString();
        imprimir(resultadoATexto(nucleo().crearDocente(args[2], args[3], args[4], telefono),
                                 QStringLiteral("docente %1 creado").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("actualizar"))
    {
        if (args.size() < 5) { imprimir(QStringLiteral("Uso: docentes actualizar <id> \"<nombre>\" <email> [telefono]")); return; }
        const QString telefono = args.size() > 5 ? args[5] : QString();
        imprimir(resultadoATexto(nucleo().actualizarDocente(args[2], args[3], args[4], telefono),
                                 QStringLiteral("docente %1 actualizado").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("eliminar"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: docentes eliminar <id>")); return; }
        imprimir(textoBool(nucleo().eliminarDocente(args[2]),
                           QStringLiteral("docente %1 eliminado").arg(args[2])));
        return;
    }
    imprimir(QStringLiteral("acción desconocida: %1").arg(accion));
}

// ─── Aulas ─────────────────────────────────────────────────────────────────

void ConsolaDatos::cmdAulas(const Args& args)
{
    if (args.size() < 2) { imprimir(QStringLiteral("Uso: aulas listar|crear|actualizar|eliminar ...")); return; }
    const QString accion = args[1].toLower();

    if (accion == QStringLiteral("listar"))
    {
        const auto r = nucleo().listarAulas();
        if (!r.ok) { imprimir(QStringLiteral("error: %1").arg(r.mensajeError)); return; }
        imprimir(QStringLiteral("aulas: %1").arg(r.valor.size()));
        for (const AulaDTO& a : r.valor)
            imprimir(QStringLiteral("  %1 | %2 | capacidad=%3 | %4 %5")
                         .arg(a.id).arg(a.nombre).arg(a.capacidad)
                         .arg(a.edificio.isEmpty() ? QStringLiteral("-") : a.edificio,
                              a.piso.isEmpty() ? QStringLiteral("") : a.piso));
        return;
    }
    if (accion == QStringLiteral("crear"))
    {
        if (args.size() < 4) { imprimir(QStringLiteral("Uso: aulas crear \"<nombre>\" <capacidad> [edificio] [piso]")); return; }
        bool okCap = false; const int capacidad = args[3].toInt(&okCap);
        if (!okCap) { imprimir(QStringLiteral("error: capacidad inválida")); return; }
        const QString edificio = args.size() > 4 ? args[4] : QString();
        const QString piso = args.size() > 5 ? args[5] : QString();
        imprimir(resultadoATexto(nucleo().crearAula(args[2], capacidad, edificio, piso),
                                 QStringLiteral("aula «%1» creada").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("actualizar"))
    {
        if (args.size() < 5) { imprimir(QStringLiteral("Uso: aulas actualizar <id> \"<nombre>\" <capacidad> [edificio] [piso]")); return; }
        bool okId = false, okCap = false;
        const int id = args[2].toInt(&okId);
        const int capacidad = args[4].toInt(&okCap);
        if (!okId || !okCap) { imprimir(QStringLiteral("error: id o capacidad inválidos")); return; }
        const QString edificio = args.size() > 5 ? args[5] : QString();
        const QString piso = args.size() > 6 ? args[6] : QString();
        imprimir(resultadoATexto(nucleo().actualizarAula(id, args[3], capacidad, edificio, piso),
                                 QStringLiteral("aula %1 actualizada").arg(id)));
        return;
    }
    if (accion == QStringLiteral("eliminar"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: aulas eliminar <id>")); return; }
        bool okId = false; const int id = args[2].toInt(&okId);
        if (!okId) { imprimir(QStringLiteral("error: id inválido")); return; }
        imprimir(textoBool(nucleo().eliminarAula(id), QStringLiteral("aula %1 eliminada").arg(id)));
        return;
    }
    imprimir(QStringLiteral("acción desconocida: %1").arg(accion));
}

// ─── Materias ──────────────────────────────────────────────────────────────

void ConsolaDatos::cmdMaterias(const Args& args)
{
    if (args.size() < 2) { imprimir(QStringLiteral("Uso: materias listar|crear|actualizar|eliminar ...")); return; }
    const QString accion = args[1].toLower();

    if (accion == QStringLiteral("listar"))
    {
        const auto r = nucleo().listarMaterias();
        if (!r.ok) { imprimir(QStringLiteral("error: %1").arg(r.mensajeError)); return; }
        imprimir(QStringLiteral("materias: %1").arg(r.valor.size()));
        for (const MateriaDTO& m : r.valor)
            imprimir(QStringLiteral("  %1 | %2 | requisitos=%3")
                         .arg(m.id).arg(m.nombre,
                                        m.requisitos.isEmpty() ? QStringLiteral("-") : m.requisitos));
        return;
    }
    if (accion == QStringLiteral("crear"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: materias crear \"<nombre>\" [\"<requisitos>\"]")); return; }
        const QString requisitos = args.size() > 3 ? args[3] : QString();
        imprimir(resultadoATexto(nucleo().crearMateria(args[2], requisitos),
                                 QStringLiteral("materia «%1» creada").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("actualizar"))
    {
        if (args.size() < 4) { imprimir(QStringLiteral("Uso: materias actualizar <id> \"<nombre>\" [\"<requisitos>\"]")); return; }
        bool okId = false; const int id = args[2].toInt(&okId);
        if (!okId) { imprimir(QStringLiteral("error: id inválido")); return; }
        const QString requisitos = args.size() > 4 ? args[4] : QString();
        imprimir(resultadoATexto(nucleo().actualizarMateria(id, args[3], requisitos),
                                 QStringLiteral("materia %1 actualizada").arg(id)));
        return;
    }
    if (accion == QStringLiteral("eliminar"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: materias eliminar <id>")); return; }
        bool okId = false; const int id = args[2].toInt(&okId);
        if (!okId) { imprimir(QStringLiteral("error: id inválido")); return; }
        imprimir(textoBool(nucleo().eliminarMateria(id), QStringLiteral("materia %1 eliminada").arg(id)));
        return;
    }
    imprimir(QStringLiteral("acción desconocida: %1").arg(accion));
}

// ─── Planes de estudio ─────────────────────────────────────────────────────

void ConsolaDatos::cmdPlanes(const Args& args)
{
    if (args.size() < 2) { imprimir(QStringLiteral("Uso: planes listar|crear|actualizar|eliminar ...")); return; }
    const QString accion = args[1].toLower();

    if (accion == QStringLiteral("listar"))
    {
        const auto r = nucleo().listarPlanes();
        if (!r.ok) { imprimir(QStringLiteral("error: %1").arg(r.mensajeError)); return; }
        imprimir(QStringLiteral("planes: %1").arg(r.valor.size()));
        for (const PlanDTO& p : r.valor)
            imprimir(QStringLiteral("  %1 | %2 | %3")
                         .arg(p.codigo, p.nombre,
                              p.descripcion.isEmpty() ? QStringLiteral("-") : p.descripcion));
        return;
    }
    if (accion == QStringLiteral("crear"))
    {
        if (args.size() < 4) { imprimir(QStringLiteral("Uso: planes crear <codigo> \"<nombre>\" [\"<descripcion>\"]")); return; }
        const QString descripcion = args.size() > 4 ? args[4] : QString();
        imprimir(resultadoATexto(nucleo().crearPlan(args[2], args[3], descripcion),
                                 QStringLiteral("plan %1 creado").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("actualizar"))
    {
        if (args.size() < 4) { imprimir(QStringLiteral("Uso: planes actualizar <codigo> \"<nombre>\" [\"<descripcion>\"]")); return; }
        const QString descripcion = args.size() > 4 ? args[4] : QString();
        imprimir(resultadoATexto(nucleo().actualizarPlan(args[2], args[3], descripcion),
                                 QStringLiteral("plan %1 actualizado").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("eliminar"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: planes eliminar <codigo>")); return; }
        imprimir(textoBool(nucleo().eliminarPlan(args[2]),
                           QStringLiteral("plan %1 eliminado").arg(args[2])));
        return;
    }
    imprimir(QStringLiteral("acción desconocida: %1").arg(accion));
}

// ─── Cursos ────────────────────────────────────────────────────────────────

void ConsolaDatos::cmdCursos(const Args& args)
{
    if (args.size() < 2) { imprimir(QStringLiteral("Uso: cursos listar|crear|actualizar|eliminar|asignar|quitar ...")); return; }
    const QString accion = args[1].toLower();

    if (accion == QStringLiteral("listar"))
    {
        const auto r = nucleo().listarCursos();
        if (!r.ok) { imprimir(QStringLiteral("error: %1").arg(r.mensajeError)); return; }
        imprimir(QStringLiteral("cursos: %1").arg(r.valor.size()));
        for (const CursoDTO& c : r.valor)
            imprimir(QStringLiteral("  %1 | %2 | turno=%3 | aulaFija=%4 | estudiantes=%5 | plan=%6 | materias=%7")
                         .arg(c.id).arg(c.nombre,
                                        c.turno.isEmpty() ? QStringLiteral("-") : c.turno)
                         .arg(c.aulaFija)
                         .arg(c.numEstudiantes)
                         .arg(c.codigoPlan.isEmpty() ? QStringLiteral("-") : c.codigoPlan)
                         .arg(c.materias.size()));
        return;
    }
    if (accion == QStringLiteral("crear"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: cursos crear \"<nombre>\" [turno] [aulaFija] [numEstudiantes] [codigoPlan]")); return; }
        const QString turno = args.size() > 3 ? args[3] : QString();
        const int aulaFija = args.size() > 4 ? args[4].toInt() : -1;
        const int numEstudiantes = args.size() > 5 ? args[5].toInt() : 0;
        const QString codigoPlan = args.size() > 6 ? args[6] : QString();
        imprimir(resultadoATexto(nucleo().crearCurso(args[2], turno, aulaFija, numEstudiantes, codigoPlan),
                                 QStringLiteral("curso «%1» creado").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("actualizar"))
    {
        if (args.size() < 4) { imprimir(QStringLiteral("Uso: cursos actualizar <id> \"<nombre>\" [turno] [aulaFija] [numEstudiantes] [codigoPlan]")); return; }
        bool okId = false; const int id = args[2].toInt(&okId);
        if (!okId) { imprimir(QStringLiteral("error: id inválido")); return; }
        const QString turno = args.size() > 4 ? args[4] : QString();
        const int aulaFija = args.size() > 5 ? args[5].toInt() : -1;
        const int numEstudiantes = args.size() > 6 ? args[6].toInt() : 0;
        const QString codigoPlan = args.size() > 7 ? args[7] : QString();
        imprimir(resultadoATexto(nucleo().actualizarCurso(id, args[3], turno, aulaFija, numEstudiantes, codigoPlan),
                                 QStringLiteral("curso %1 actualizado").arg(id)));
        return;
    }
    if (accion == QStringLiteral("eliminar"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: cursos eliminar <id>")); return; }
        bool okId = false; const int id = args[2].toInt(&okId);
        if (!okId) { imprimir(QStringLiteral("error: id inválido")); return; }
        imprimir(textoBool(nucleo().eliminarCurso(id), QStringLiteral("curso %1 eliminado").arg(id)));
        return;
    }
    if (accion == QStringLiteral("asignar"))
    {
        if (args.size() < 5) { imprimir(QStringLiteral("Uso: cursos asignar <idCurso> <idMateria> <horasSemanales>")); return; }
        bool okC = false, okM = false, okH = false;
        const int idCurso = args[2].toInt(&okC);
        const int idMateria = args[3].toInt(&okM);
        const int horas = args[4].toInt(&okH);
        if (!okC || !okM || !okH) { imprimir(QStringLiteral("error: parámetros inválidos")); return; }
        imprimir(resultadoATexto(nucleo().asignarMateriaACurso(idCurso, idMateria, horas),
                                 QStringLiteral("materia %1 asignada al curso %2").arg(idMateria).arg(idCurso)));
        return;
    }
    if (accion == QStringLiteral("quitar"))
    {
        if (args.size() < 4) { imprimir(QStringLiteral("Uso: cursos quitar <idCurso> <idMateria>")); return; }
        bool okC = false, okM = false;
        const int idCurso = args[2].toInt(&okC);
        const int idMateria = args[3].toInt(&okM);
        if (!okC || !okM) { imprimir(QStringLiteral("error: parámetros inválidos")); return; }
        imprimir(textoBool(nucleo().quitarMateriaDeCurso(idCurso, idMateria),
                           QStringLiteral("materia %1 quitada del curso %2").arg(idMateria).arg(idCurso)));
        return;
    }
    imprimir(QStringLiteral("acción desconocida: %1").arg(accion));
}

// ─── Turnos y recesos ──────────────────────────────────────────────────────

void ConsolaDatos::cmdTurnos(const Args& args)
{
    if (args.size() < 2) { imprimir(QStringLiteral("Uso: turnos listar|crear|actualizar|eliminar|receso-agregar|receso-eliminar ...")); return; }
    const QString accion = args[1].toLower();

    if (accion == QStringLiteral("listar"))
    {
        const auto r = nucleo().listarTurnos();
        if (!r.ok) { imprimir(QStringLiteral("error: %1").arg(r.mensajeError)); return; }
        imprimir(QStringLiteral("turnos: %1").arg(r.valor.size()));
        for (const TurnoDTO& t : r.valor)
            imprimir(QStringLiteral("  %1 | %2-%3 | slots=%4 | recesos=%5")
                         .arg(t.nombre, t.inicio.toString(QStringLiteral("HH:mm")),
                              t.fin.toString(QStringLiteral("HH:mm")))
                         .arg(t.numSlots).arg(t.recesos.size()));
        return;
    }
    if (accion == QStringLiteral("crear") || accion == QStringLiteral("actualizar"))
    {
        if (args.size() < 6) { imprimir(QStringLiteral("Uso: turnos %1 \"<nombre>\" <inicio HH:mm> <fin HH:mm> <numSlots>").arg(accion)); return; }
        const QTime inicio = QTime::fromString(args[3], QStringLiteral("HH:mm"));
        const QTime fin = QTime::fromString(args[4], QStringLiteral("HH:mm"));
        bool okSlots = false; const int numSlots = args[5].toInt(&okSlots);
        if (!inicio.isValid() || !fin.isValid() || !okSlots) { imprimir(QStringLiteral("error: hora o numSlots inválidos")); return; }
        if (accion == QStringLiteral("crear"))
            imprimir(resultadoATexto(nucleo().crearTurno(args[2], inicio, fin, numSlots),
                                     QStringLiteral("turno «%1» creado").arg(args[2])));
        else
            imprimir(resultadoATexto(nucleo().actualizarTurno(args[2], inicio, fin, numSlots),
                                     QStringLiteral("turno «%1» actualizado").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("eliminar"))
    {
        if (args.size() < 3) { imprimir(QStringLiteral("Uso: turnos eliminar \"<nombre>\"")); return; }
        imprimir(textoBool(nucleo().eliminarTurno(args[2]),
                           QStringLiteral("turno «%1» eliminado").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("receso-agregar"))
    {
        if (args.size() < 5) { imprimir(QStringLiteral("Uso: turnos receso-agregar \"<turno>\" <despuesDeSlot> <duracion>")); return; }
        bool okSlot = false, okDur = false;
        const int despuesDeSlot = args[3].toInt(&okSlot);
        const int duracion = args[4].toInt(&okDur);
        if (!okSlot || !okDur) { imprimir(QStringLiteral("error: parámetros inválidos")); return; }
        imprimir(resultadoATexto(nucleo().agregarReceso(args[2], despuesDeSlot, duracion),
                                 QStringLiteral("receso agregado al turno «%1»").arg(args[2])));
        return;
    }
    if (accion == QStringLiteral("receso-eliminar"))
    {
        if (args.size() < 4) { imprimir(QStringLiteral("Uso: turnos receso-eliminar \"<turno>\" <despuesDeSlot>")); return; }
        bool okSlot = false; const int despuesDeSlot = args[3].toInt(&okSlot);
        if (!okSlot) { imprimir(QStringLiteral("error: parámetro inválido")); return; }
        imprimir(textoBool(nucleo().eliminarReceso(args[2], despuesDeSlot),
                           QStringLiteral("receso del turno «%1» eliminado").arg(args[2])));
        return;
    }
    imprimir(QStringLiteral("acción desconocida: %1").arg(accion));
}

// ─── Cascada y dependientes ────────────────────────────────────────────────

void ConsolaDatos::cmdDependientes(const Args& args)
{
    if (args.size() < 3) { imprimir(QStringLiteral("Uso: dependientes <dominio> <id>")); return; }
    const QVector<Dependencia> deps = nucleo().dependientesDe(args[1], args[2]);
    imprimir(QStringLiteral("dependientes de %1/%2: %3").arg(args[1], args[2]).arg(deps.size()));
    for (const Dependencia& d : deps)
        imprimir(QStringLiteral("  - [%1] %2 (id=%3)").arg(d.dominio, d.descripcion).arg(d.id));
}

void ConsolaDatos::cmdCascada(const Args& args)
{
    if (args.size() < 3) { imprimir(QStringLiteral("Uso: cascada <dominio> <id> [--si]")); return; }

    const QVector<Dependencia> deps = nucleo().dependientesDe(args[1], args[2]);
    if (!deps.isEmpty())
    {
        imprimir(QStringLiteral("Se eliminarán en cascada %1 registro(s):").arg(deps.size()));
        for (const Dependencia& d : deps)
            imprimir(QStringLiteral("  - [%1] %2 (id=%3)").arg(d.dominio, d.descripcion).arg(d.id));

        if (!confirmar(QStringLiteral("¿Confirmar la eliminación en cascada?"), args))
        {
            imprimir(QStringLiteral("cancelado"));
            return;
        }
    }

    const bool ok = nucleo().eliminarConCascada(args[1], args[2]);
    imprimir(textoBool(ok, QStringLiteral("%1/%2 eliminado en cascada").arg(args[1], args[2])));
}

// ─── Cambios pendientes ────────────────────────────────────────────────────

void ConsolaDatos::cmdPendientes(const Args& args)
{
    Q_UNUSED(args);
    const QVector<OperacionPendiente> pendientes = nucleo().gestorPendientes().pendientes();
    imprimir(QStringLiteral("pendientes: %1 (hayPendientes=%2)")
                 .arg(pendientes.size())
                 .arg(nucleo().hayPendientes() ? QStringLiteral("sí") : QStringLiteral("no")));
    for (const OperacionPendiente& op : pendientes)
        imprimir(QStringLiteral("  id=%1 | dominio=%2 | registro=%3 | desc=%4")
                     .arg(op.id).arg(op.dominio, op.registroId, op.descripcion));
}

void ConsolaDatos::cmdReintentar(const Args& args)
{
    if (args.size() < 2) { imprimir(QStringLiteral("Uso: reintentar <id>")); return; }
    bool okId = false; const qint64 id = args[1].toLongLong(&okId);
    if (!okId) { imprimir(QStringLiteral("error: id inválido")); return; }
    imprimir(resultadoATexto(nucleo().reintentarPendiente(id),
                             QStringLiteral("cambio pendiente %1 reintentado").arg(id)));
}

// ─── Estado y ayuda ────────────────────────────────────────────────────────

void ConsolaDatos::cmdEstado(const Args& args)
{
    Q_UNUSED(args);
    imprimir(QStringLiteral("base: %1").arg(m_contexto.ruta()));
    imprimir(QStringLiteral("abierta: %1").arg(m_contexto.abierto() ? QStringLiteral("sí") : QStringLiteral("no")));
    imprimir(QStringLiteral("hayPendientes: %1").arg(nucleo().hayPendientes() ? QStringLiteral("sí") : QStringLiteral("no")));
}

void ConsolaDatos::cmdAyuda(const Args& args)
{
    Q_UNUSED(args);
    imprimirAyuda();
}
