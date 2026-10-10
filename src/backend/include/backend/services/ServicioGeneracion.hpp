#pragma once

/**
 * @file ServicioGeneracion.hpp
 * @brief Orquestación de una generación de horarios (RF-1, RF-3, RF-5).
 *
 * Construye/valida la entrada, la envía al proceso de cálculo por un puerto
 * inyectable, espera con un plazo (60 s en producción) y analiza la salida.
 * Marca «datos anteriores» si la instantánea de dominio cambió durante el
 * cálculo. Es UI-free: los desenlaces se exponen por `finalizada()` y por
 * `ultimoResultado()`, que consume el frontend.
 */

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include <functional>

#include "backend/data/horario_salida.hpp"
#include "backend/services/ConstructorEntradaSolver.hpp"
#include "backend/services/ValidadorSalidaSolver.hpp"

/// Huella determinista de una instantánea de dominio (para detectar «datos
/// anteriores»). Pura.
QString huellaDatos(const DatosDominio& datos);

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
        EntradaInvalida   ///< La entrada no superó las validaciones previas.
    };

    Estado        estado = Estado::Inactivo;
    QString       mensaje;
    bool          datosAnteriores = false;
    HorarioSalida horario;      ///< Válido si `presentable`.
    AnalisisSalida analisis;    ///< Avisos P1–P3 y conflictos P4.
    bool          presentable = false;
};

/// Orquesta una generación. Una sola en curso a la vez.
class ServicioGeneracion : public QObject
{
    Q_OBJECT

public:
    /// @param puerto       Transporte hacia el proceso de cálculo (no se posee).
    /// @param plazoMs      Plazo de espera (60 s en producción; corto en tests).
    /// @param huellaActual Devuelve la huella de la instantánea actual; si al
    ///                     recibir la respuesta difiere de la enviada, el
    ///                     resultado se marca «datos anteriores». Vacío = nunca.
    /// @param reloj        Reloj para `metadata.fecha_generacion` (inyectable).
    /// @param padre        Objeto Qt padre.
    explicit ServicioGeneracion(PuertoSolver& puerto,
                                int plazoMs = 60000,
                                std::function<QString()> huellaActual = {},
                                std::function<QDateTime()> reloj = {},
                                QObject* padre = nullptr);

    /// Inicia una generación. Si ya hay una en curso, no hace nada.
    void generar(const QJsonObject& entrada, const QString& huella);

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

    bool m_enCurso = false;
    quint64 m_generacion = 0;
    QString m_huella;
    SolverConfig m_config;
    ResultadoGeneracion m_ultimo;
};
