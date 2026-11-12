#include "backend/services/ServicioGeneracion.hpp"

#include "backend/services/ServicioTurnosEventos.hpp"
#include "backend/services/ValidadorEntradaSolver.hpp"
#include "backend/services/preferencias.hpp"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QStringList>
#include <QTimer>

#include <algorithm>

namespace
{
    /// Docentes sin capacidad —tope duro 0, declarado o ausente (el parser lo lee como 0)— para
    /// informarlos en el mensaje de NoFactible (020, RF-1). Nunca deben deshabilitarse en
    /// silencio.
    QStringList nombresSinCapacidad(const SolverConfig& config)
    {
        QStringList nombres;
        for (int p = 0; p < config.profesores.size(); ++p)
        {
            if (config.profesores[p].horas_aula > 0)
                continue;
            const QString nombre = config.profesores[p].nombre.trimmed();
            nombres.append(nombre.isEmpty() ? QStringLiteral("docente %1").arg(p) : nombre);
        }
        return nombres;
    }
} // namespace

QString huellaDatos(const DatosDominio& datos, int duracionMinutos,
                    const QVector<PreferenciaProyectada>& intervinientes)
{
    QString canon;

    // La hora académica entra en la huella (RF-6): un cambio de D entre el envío y la
    // respuesta marca «datos anteriores».
    canon += QStringLiteral("dur:%1;").arg(duracionMinutos);

    // Preferencias intervinientes (013, RF-6): solo las que intervienen, ya proyectadas —
    // docente, día de la generación, sentido y franjas—; nunca el rango de reloj. Sin
    // intervinientes no se emite el bloque, así que la huella vale lo mismo que antes de la 013.
    if (!intervinientes.isEmpty())
    {
        // Canonización: ni el orden de la lista ni el de las franjas alteran la huella (RNF-2).
        QVector<PreferenciaProyectada> ordenadas = intervinientes;
        for (PreferenciaProyectada& p : ordenadas)
            std::sort(p.slot.begin(), p.slot.end());
        std::sort(ordenadas.begin(), ordenadas.end(),
                  [](const PreferenciaProyectada& a, const PreferenciaProyectada& b) {
                      if (a.docente != b.docente) return a.docente < b.docente;
                      if (a.dia != b.dia) return a.dia < b.dia;
                      if (a.sentido != b.sentido)
                          return static_cast<int>(a.sentido) < static_cast<int>(b.sentido);
                      return a.slot < b.slot;
                  });

        for (const PreferenciaProyectada& p : ordenadas)
        {
            QStringList franjas;
            for (int slot : p.slot)
                franjas << QString::number(slot);
            canon += QStringLiteral("pref:%1,%2,%3,%4;")
                         .arg(p.docente)
                         .arg(p.dia)
                         .arg(textoDeSentido(p.sentido), franjas.join(QLatin1Char(',')));
        }
    }

    for (const ProfesorDTO& d : datos.docentes)
    {
        canon += QStringLiteral("D:%1|%2;").arg(d.id, d.nombre);
        for (const DisponibilidadDTO& disp : d.disponibilidad)
            canon += QStringLiteral("disp:%1,%2,%3;")
                         .arg(disp.franja.dia)
                         .arg(disp.franja.inicio.toString(QStringLiteral("HH:mm")),
                              disp.franja.fin.toString(QStringLiteral("HH:mm")));
        for (const MateriaAsignadaDTO& m : d.materias)
            canon += QStringLiteral("mat:%1;").arg(m.id);
        for (const MateriaAsignadaDTO& m : d.materiasSuplentes)
            canon += QStringLiteral("sup:%1;").arg(m.id);
    }
    for (const AulaDTO& a : datos.aulas)
        canon += QStringLiteral("A:%1|%2|%3|%4;")
                     .arg(a.id)
                     .arg(a.nombre)
                     .arg(a.capacidad)
                     .arg(a.idTipoAula);
    for (const MateriaDTO& m : datos.materias)
        canon += QStringLiteral("M:%1|%2|%3;").arg(m.id).arg(m.nombre).arg(m.idTipoAula);
    for (const PlanEstudio& p : datos.planes)
    {
        canon += QStringLiteral("P:%1|%2;").arg(p.codigo, p.nombre);
        for (const MateriaEnPlan& m : p.materias)
            canon += QStringLiteral("pm:%1|%2|%3;").arg(m.curso).arg(m.materia.nombre).arg(m.horas);
    }
    for (const CursoDTO& c : datos.cursos)
    {
        // Solo los campos persistidos del curso (RF-5): sin materias, sin plan ni
        // numero de estudiantes (viven en la configuracion del solver).
        canon += QStringLiteral("C:%1|%2|%3|%4|%5;")
                     .arg(c.id)
                     .arg(c.anio)
                     .arg(c.seccion, c.turno)
                     .arg(c.aula);
    }
    for (const TurnoDTO& t : datos.turnos)
    {
        canon += QStringLiteral("T:%1|%2|%3|%4;")
                     .arg(t.nombre)
                     .arg(t.inicio.toString(QStringLiteral("HH:mm")),
                          t.fin.toString(QStringLiteral("HH:mm")))
                     .arg(t.numSlots);
        for (const EventoDTO& e : t.eventos)
            canon += QStringLiteral("e:%1|%2|%3|%4|%5|%6|%7;")
                         .arg(e.id)
                         .arg(e.nombre)
                         .arg(e.despuesDeSlot)
                         .arg(e.orden)
                         .arg(e.duracion)
                         .arg(e.inicio.isValid() ? e.inicio.toString(QStringLiteral("HH:mm"))
                                                 : QString(),
                              e.fin.isValid() ? e.fin.toString(QStringLiteral("HH:mm"))
                                              : QString());
    }

    return QString::fromLatin1(
        QCryptographicHash::hash(canon.toUtf8(), QCryptographicHash::Sha256).toHex());
}

HorarioSalida estamparFechaGeneracion(const HorarioSalida& horario, const QDateTime& ahora)
{
    HorarioSalida copia = horario;
    if (ahora.isValid())
        copia.metadata.fecha_generacion = ahora.toString(Qt::ISODate);
    return copia;
}

ServicioGeneracion::ServicioGeneracion(PuertoSolver& puerto,
                                       int plazoMs,
                                       std::function<QString()> huellaActual,
                                       std::function<QDateTime()> reloj,
                                       std::function<QString()> revisarBase,
                                       QObject* padre)
    : QObject(padre)
    , m_puerto(puerto)
    , m_plazoMs(plazoMs)
    , m_huellaActual(std::move(huellaActual))
    , m_reloj(std::move(reloj))
    , m_revisarBase(std::move(revisarBase))
{
}

QDateTime ServicioGeneracion::ahora() const
{
    return m_reloj ? m_reloj() : QDateTime::currentDateTime();
}

void ServicioGeneracion::generar(const QJsonObject& entrada, const QString& huella,
                                 const QVector<PreferenciaNoAplicada>& noAplicadas)
{
    // Segunda generación concurrente: no admitida.
    if (m_enCurso)
        return;

    // Diagnóstico de esta generación (013, RF-8): preferencias que no intervinieron.
    m_noAplicadas = noAplicadas;

    // Validez de la base (016 RF-1): va antes de validar la entrada y de enviar. Con una
    // base inválida el diagnóstico útil es el de la base, no el de los datos.
    if (m_revisarBase)
    {
        const QString mensajeBase = m_revisarBase();
        if (!mensajeBase.isEmpty())
        {
            m_ultimo = ResultadoGeneracion{};
            m_ultimo.estado  = ResultadoGeneracion::Estado::BaseInvalida;
            m_ultimo.mensaje = mensajeBase;
            emit finalizada();
            return;
        }
    }

    // Validaciones previas (RF-1): si no pasan, no se envía.
    const Resultado<SolverConfig> validacion = validarEntradaSolver(entrada);
    if (!validacion.ok)
    {
        m_ultimo = ResultadoGeneracion{};
        m_ultimo.estado  = ResultadoGeneracion::Estado::EntradaInvalida;
        m_ultimo.mensaje = validacion.mensajeError;
        emit finalizada();
        return;
    }

    m_config = validacion.valor;
    m_huella = huella;
    m_enCurso = true;
    m_ultimo = ResultadoGeneracion{};
    m_ultimo.estado = ResultadoGeneracion::Estado::Calculando;

    const quint64 generacion = ++m_generacion;

    if (m_plazoMs > 0)
    {
        QTimer::singleShot(m_plazoMs, this, [this, generacion]() {
            if (generacion != m_generacion || !m_enCurso)
                return;
            m_enCurso = false;
            m_ultimo = ResultadoGeneracion{};
            m_ultimo.estado  = ResultadoGeneracion::Estado::TiempoAgotado;
            m_ultimo.mensaje = QStringLiteral(
                "El proceso de cálculo no respondió en el plazo previsto. La petición se abandonó;"
                " puede seguir operando con normalidad.");
            emit finalizada();
        });
    }

    m_puerto.resolver(entrada, [this, generacion](const RespuestaSolver& respuesta) {
        // Respuesta de una generación ya abandonada (timeout): se ignora (RF-1).
        if (generacion != m_generacion || !m_enCurso)
            return;
        alResponder(respuesta);
    });
}

void ServicioGeneracion::alResponder(const RespuestaSolver& respuesta)
{
    m_enCurso = false;

    // 020 (RF-2/RF-3): el desenlace del cálculo es la verdad; `ok`/`factible` son derivados.
    switch (respuesta.desenlace)
    {
        case DesenlaceSolver::Infactible:
        {
            // H3 mínimo (D-5): se conserva la causa del motor; nunca un texto genérico. Sin
            // diagnóstico, se declara que no hay causa atribuible.
            m_ultimo = ResultadoGeneracion{};
            m_ultimo.estado = ResultadoGeneracion::Estado::NoFactible;
            QString mensaje = respuesta.error.isEmpty()
                                  ? QStringLiteral("El motor demostró que no hay solución y no hay"
                                                   " una causa atribuible.")
                                  : respuesta.error;

            // 020 (RF-1): se informa también de qué docentes quedan sin capacidad para que no
            // sea un deshabilitado silencioso.
            const QStringList sinCapacidad = nombresSinCapacidad(m_config);
            if (!sinCapacidad.isEmpty())
                mensaje += QStringLiteral("\nDocentes sin capacidad (horas de aula 0 o ausentes): %1.")
                               .arg(sinCapacidad.join(QStringLiteral(", ")));

            m_ultimo.mensaje = mensaje;
            emit finalizada();
            return;
        }

        case DesenlaceSolver::NoConcluyente:
            // Presupuesto agotado sin prueba: distinto de NoFactible y sin horario.
            m_ultimo = ResultadoGeneracion{};
            m_ultimo.estado = ResultadoGeneracion::Estado::NoConcluyente;
            m_ultimo.mensaje = respuesta.error.isEmpty()
                                   ? QStringLiteral("El cálculo agotó el presupuesto sin encontrar"
                                                    " solución y sin demostrar que no exista: el"
                                                    " resultado no es concluyente.")
                                   : respuesta.error;
            emit finalizada();
            return;

        case DesenlaceSolver::SinRespuesta:
            // Pérdida de comunicación: no es una respuesta inválida ni NoFactible.
            m_ultimo = ResultadoGeneracion{};
            m_ultimo.estado = ResultadoGeneracion::Estado::SinRespuesta;
            m_ultimo.mensaje = respuesta.error.isEmpty()
                                   ? QStringLiteral("No hubo respuesta del proceso de cálculo: se"
                                                    " perdió la comunicación. No se puede afirmar"
                                                    " que no exista solución.")
                                   : respuesta.error;
            emit finalizada();
            return;

        case DesenlaceSolver::ErrorContrato:
            // Modelo inválido, error interno o respuesta fuera de contrato: camino propio.
            m_ultimo = ResultadoGeneracion{};
            m_ultimo.estado = ResultadoGeneracion::Estado::ContratoInvalido;
            m_ultimo.mensaje = respuesta.error.isEmpty()
                                   ? QStringLiteral("El proceso de cálculo devolvió una respuesta o"
                                                    " un modelo inválido (error de entrada o de"
                                                    " contrato).")
                                   : respuesta.error;
            emit finalizada();
            return;

        case DesenlaceSolver::Exito:
            break;   // sigue el análisis del horario
    }

    if (!respuesta.salida.value(QStringLiteral("horarios")).isObject())
    {
        m_ultimo = ResultadoGeneracion{};
        m_ultimo.estado  = ResultadoGeneracion::Estado::ContratoInvalido;
        m_ultimo.mensaje = QStringLiteral("El resultado recibido no corresponde al contrato del"
                                          " solver.");
        emit finalizada();
        return;
    }

    const HorarioSalida horario =
        estamparFechaGeneracion(HorarioSalida::fromJson(respuesta.salida), ahora());
    const AnalisisSalida analisis = analizarSalidaSolver(horario, m_config);

    // 020 (D8): la optimalidad por fase llega en la metadata de la salida (la inyecta
    // `servicio_solver`); ausente = todas las fases ejecutadas se probaron óptimas.
    QStringList fasesNoOptimas;
    const QJsonArray fasesJson =
        respuesta.salida.value(QStringLiteral("metadata")).toObject()
            .value(QStringLiteral("fases_sin_optimalidad")).toArray();
    for (const QJsonValue& fase : fasesJson)
        if (fase.isString())
            fasesNoOptimas.append(fase.toString());

    m_ultimo = ResultadoGeneracion{};
    m_ultimo.horario  = horario;
    m_ultimo.analisis = analisis;
    m_ultimo.fasesNoOptimas = fasesNoOptimas;
    // 013, RF-8: estado por preferencia y agregado, sobre el horario y la proyección ya
    // calculada. No cambia el contrato de salida del solver.
    m_ultimo.cumplimiento = analizarPreferencias(m_config, horario, m_noAplicadas);
    // 014, RF-12: informe aditivo de bloques, desvío, huecos y separación sobre el horario.
    m_ultimo.bloques = analizarBloques(horario, m_config);

    // 020 (RF-5): P4 (solapamientos) y P5 (exceso del tope de horas de aula) son fallos
    // distintos. Un exceso de tope NUNCA se informa como solapamiento.
    if (!analisis.presentable())
    {
        m_ultimo.estado = ResultadoGeneracion::Estado::CalculoFallido;
        if (analisis.hayConflictoSolapamiento())
            m_ultimo.mensaje = QStringLiteral("El cálculo produjo conflictos de solapamiento; el"
                                              " resultado no es válido.");
        else if (!analisis.excesosTopeHoras.isEmpty())
            m_ultimo.mensaje = QStringLiteral("El cálculo superó el tope de horas de aula de algún"
                                              " docente; el resultado no es presentable.")
                                   + QLatin1Char('\n')
                                   + analisis.excesosTopeHoras.join(QLatin1Char('\n'));
        else
            m_ultimo.mensaje = QStringLiteral("El cálculo produjo un resultado no presentable.");
        emit finalizada();
        return;
    }

    m_ultimo.presentable = true;
    if (m_huellaActual && m_huellaActual() != m_huella)
    {
        m_ultimo.estado          = ResultadoGeneracion::Estado::DatosAnteriores;
        m_ultimo.datosAnteriores = true;
        m_ultimo.mensaje         = QStringLiteral("El horario corresponde a datos anteriores a"
                                                  " cambios recientes; confirme antes de guardarlo.");
    }
    else
    {
        m_ultimo.estado  = ResultadoGeneracion::Estado::Listo;
        m_ultimo.mensaje = QStringLiteral("Horario generado.");
    }

    emit finalizada();
}
