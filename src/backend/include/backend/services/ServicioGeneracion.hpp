#pragma once

/**
 * @file ServicioGeneracion.hpp
 * @brief Orquestación de una generación de horarios (RF-1, RF-3, RF-5).
 *
 * Construye/valida la entrada, la envía al proceso de cálculo por un puerto
 * inyectable, espera con un plazo (300 s en producción) y analiza la salida.
 * Marca «datos anteriores» si la instantánea de dominio cambió durante el
 * cálculo. Es UI-free: los desenlaces se exponen por `finalizada()` y por
 * `ultimoResultado()`, que consume el frontend.
 */

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

#include <functional>

#include "backend/data/horario_salida.hpp"
#include "backend/services/ConstructorEntradaSolver.hpp"
#include "backend/services/ValidadorSalidaSolver.hpp"
#include "backend/services/analisis_bloques.hpp"
#include "backend/services/preferencias_cumplimiento.hpp"
#include "backend/services/proyeccion_preferencias.hpp"

/// Huella determinista de una instantánea de dominio, de la duración efectiva de la hora
/// académica y de la proyección de las preferencias intervinientes (para detectar «datos
/// anteriores»). Pura.
///
/// Entran solo las preferencias que **intervienen**, ya proyectadas —docente por índice, día de
/// la generación, franjas y sentido—; el rango de reloj declarado **no** entra (RF-6 de la 013).
/// Sin intervinientes no se emite el bloque y la huella vale exactamente lo mismo que antes de
/// la 013. El orden de la lista y el de las franjas no la altera: se canonizan antes de hashear.
QString huellaDatos(const DatosDominio& datos, int duracionMinutos,
                    const QVector<PreferenciaProyectada>& intervinientes);

/// Devuelve una copia del horario con `metadata.fecha_generacion` estampada con
/// `ahora` (inyectado). Pura.
HorarioSalida estamparFechaGeneracion(const HorarioSalida& horario, const QDateTime& ahora);

/// Respuesta del proceso de cálculo (abstraída del transporte IPC).
struct RespuestaSolver
{
    bool ok = false;          ///< El proceso respondió de forma válida.
    bool factible = false;    ///< Existe solución.
    QJsonObject salida;       ///< HorarioSalida JSON (válido solo si `ok && factible`).
    QString error;            ///< Motivo si `!ok`.
};

/// Puerto hacia el proceso de cálculo. El transporte real (IPC) lo implementa
/// `src/app`/`middleware`; los tests usan un doble. `resolver` debe invocar
/// `alResponder` exactamente una vez (o nunca, para simular una caída/timeout).
class PuertoSolver
{
public:
    virtual ~PuertoSolver() = default;
    virtual void resolver(const QJsonObject& entrada,
                          std::function<void(const RespuestaSolver&)> alResponder) = 0;
};

/// Desenlace de una generación.
struct ResultadoGeneracion
{
    enum class Estado
    {
        Inactivo,         ///< Sin generación.
        Calculando,       ///< En curso.
        Listo,            ///< Solución presentable.
        DatosAnteriores,  ///< Solución válida pero la instantánea cambió; requiere confirmación.
        NoFactible,       ///< El proceso indicó que no hay solución.
        ContratoInvalido, ///< La respuesta no corresponde al contrato.
        CalculoFallido,   ///< La validación posterior (P4) detectó solapamientos.
        TiempoAgotado,    ///< Se superó el plazo o se perdió la comunicación.
        EntradaInvalida,  ///< La entrada no superó las validaciones previas.
        BaseInvalida      ///< La base no es válida: no se valida la entrada ni se envía.
    };

    Estado        estado = Estado::Inactivo;
    QString       mensaje;
    bool          datosAnteriores = false;
    HorarioSalida horario;      ///< Válido si `presentable`.
    AnalisisSalida analisis;    ///< Avisos P1–P3 y conflictos P4.
    bool          presentable = false;
    /// Estado por preferencia y cumplimiento agregado (013, RF-8). Aditivo: no viaja en el
    /// contrato de salida del solver, se calcula aquí sobre el horario y la proyección.
    CumplimientoPreferencias cumplimiento;
    /// Informe de bloques, desvío, huecos y separación (014, RF-12). Aditivo: no cambia el
    /// contrato de salida del solver.
    AnalisisBloques bloques;
};

/// Orquesta una generación. Una sola en curso a la vez.
class ServicioGeneracion : public QObject
{
    Q_OBJECT

public:
    /// @param puerto       Transporte hacia el proceso de cálculo (no se posee).
    /// @param plazoMs      Plazo de espera (300 s en producción; corto en tests).
    /// @param huellaActual Devuelve la huella de la instantánea actual; si al
    ///                     recibir la respuesta difiere de la enviada, el
    ///                     resultado se marca «datos anteriores». Vacío = nunca.
    /// @param reloj        Reloj para `metadata.fecha_generacion` (inyectable).
    /// @param revisarBase  Comprobación de validez de la base (016 RF-1): devuelve
    ///                     el mensaje con causa y acción, o vacío si es válida. Se
    ///                     comprueba antes de validar la entrada y de enviar. Va
    ///                     antes de `padre` para que olvidar el cableado no compile.
    /// @param padre        Objeto Qt padre.
    explicit ServicioGeneracion(PuertoSolver& puerto,
                                int plazoMs = 300000,
                                std::function<QString()> huellaActual = {},
                                std::function<QDateTime()> reloj = {},
                                std::function<QString()> revisarBase = {},
                                QObject* padre = nullptr);

    /// Inicia una generación. Si ya hay una en curso, no hace nada.
    ///
    /// `noAplicadas` es el diagnóstico de la construcción (preferencias que no intervinieron,
    /// con su motivo); se usa para informar el cumplimiento del resultado (013, RF-8).
    void generar(const QJsonObject& entrada, const QString& huella,
                 const QVector<PreferenciaNoAplicada>& noAplicadas = {});

    bool enCurso() const { return m_enCurso; }
    const ResultadoGeneracion& ultimoResultado() const { return m_ultimo; }

signals:
    /// La generación terminó (o fue rechazada). El desenlace está en `ultimoResultado()`.
    void finalizada();

private:
    void alResponder(const RespuestaSolver& respuesta);

    QDateTime ahora() const;

    PuertoSolver& m_puerto;
    int m_plazoMs;
    std::function<QString()> m_huellaActual;
    std::function<QDateTime()> m_reloj;
    std::function<QString()> m_revisarBase;

    bool m_enCurso = false;
    quint64 m_generacion = 0;
    QString m_huella;
    QVector<PreferenciaNoAplicada> m_noAplicadas;   ///< diagnóstico de la última generación
    SolverConfig m_config;
    ResultadoGeneracion m_ultimo;
};
