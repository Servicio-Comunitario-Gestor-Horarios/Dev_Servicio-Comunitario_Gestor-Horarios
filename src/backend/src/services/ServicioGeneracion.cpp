#include "backend/services/ServicioGeneracion.hpp"

#include "backend/services/ValidadorEntradaSolver.hpp"

#include <QCryptographicHash>
#include <QTimer>

QString huellaDatos(const DatosDominio& datos)
{
    QString canon;

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
    }
    for (const AulaDTO& a : datos.aulas)
        canon += QStringLiteral("A:%1|%2|%3;").arg(a.id).arg(a.nombre).arg(a.capacidad);
    for (const MateriaDTO& m : datos.materias)
        canon += QStringLiteral("M:%1|%2;").arg(m.id).arg(m.nombre);
    for (const PlanDTO& p : datos.planes)
        canon += QStringLiteral("P:%1|%2;").arg(p.codigo, p.nombre);
    for (const CursoDTO& c : datos.cursos)
    {
        canon += QStringLiteral("C:%1|%2|%3|%4|%5|%6;")
                     .arg(c.id)
                     .arg(c.nombre, c.turno)
                     .arg(c.aulaFija)
                     .arg(c.numEstudiantes)
                     .arg(c.codigoPlan);
        for (const CursoMateriaDTO& cm : c.materias)
            canon += QStringLiteral("cm:%1,%2;").arg(cm.idMateria).arg(cm.horasSemanales);
    }
    for (const TurnoDTO& t : datos.turnos)
    {
        canon += QStringLiteral("T:%1|%2|%3|%4;")
                     .arg(t.nombre)
                     .arg(t.inicio.toString(QStringLiteral("HH:mm")),
                          t.fin.toString(QStringLiteral("HH:mm")))
                     .arg(t.numSlots);
        for (const RecesoDTO& r : t.recesos)
            canon += QStringLiteral("r:%1,%2;").arg(r.despuesDeSlot).arg(r.duracion);
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
                                       QObject* padre)
    : QObject(padre)
    , m_puerto(puerto)
    , m_plazoMs(plazoMs)
    , m_huellaActual(std::move(huellaActual))
    , m_reloj(std::move(reloj))
{
}

QDateTime ServicioGeneracion::ahora() const
{
    return m_reloj ? m_reloj() : QDateTime::currentDateTime();
}

void ServicioGeneracion::generar(const QJsonObject& entrada, const QString& huella)
{
    // Segunda generación concurrente: no admitida.
    if (m_enCurso)
        return;

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

    if (!respuesta.ok)
    {
        m_ultimo = ResultadoGeneracion{};
        m_ultimo.estado  = ResultadoGeneracion::Estado::ContratoInvalido;
        m_ultimo.mensaje = respuesta.error.isEmpty()
                               ? QStringLiteral("El proceso de cálculo devolvió una respuesta"
                                                " inválida.")
                               : respuesta.error;
        emit finalizada();
        return;
    }

    if (!respuesta.factible)
    {
        m_ultimo = ResultadoGeneracion{};
        m_ultimo.estado  = ResultadoGeneracion::Estado::NoFactible;
        m_ultimo.mensaje = QStringLiteral(
            "No existe una solución factible para los datos y la configuración actuales.");
        emit finalizada();
        return;
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

    m_ultimo = ResultadoGeneracion{};
    m_ultimo.horario  = horario;
    m_ultimo.analisis = analisis;

    // P4: conflictos de solapamiento ⇒ el cálculo falló.
    if (!analisis.presentable())
    {
        m_ultimo.estado  = ResultadoGeneracion::Estado::CalculoFallido;
        m_ultimo.mensaje = QStringLiteral("El cálculo produjo conflictos de solapamiento; el"
                                          " resultado no es válido.");
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
