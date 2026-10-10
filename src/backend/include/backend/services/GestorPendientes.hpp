#pragma once

/**
 * @file GestorPendientes.hpp
 * @brief Cambios pendientes y reintento (RF-3, RNF-3, RNF-4).
 *
 * Cuando una operación de escritura (alta, modificación o baja) falla, el
 * cambio no se aplica pero tampoco se descarta: queda registrado como cambio
 * pendiente en memoria y puede reintentarse. El frontend usa el estado por
 * registro para marcar la fila (guardada / pendiente de guardar / pendiente de
 * eliminar) y la consulta de pendientes para la guardia de cierre (RF-6).
 */

#include <functional>

#include <QString>
#include <QVector>

#include "backend/resultado.hpp"

/// Tipo de escritura que originó un cambio pendiente.
enum class TipoOperacion {
    Alta,         ///< creación de un registro
    Modificacion, ///< modificación de un registro
    Baja          ///< eliminación de un registro
};

/// Estado de persistencia de un registro (RF-3, RNF-4).
enum class EstadoPendiente {
    Guardado,          ///< persistido: fila normal
    PendienteGuardar,  ///< alta/modificación fallida: pendiente de guardar
    PendienteEliminar  ///< baja fallida: pendiente de eliminar
};

/**
 * @brief Operación de escritura fallida (cambio pendiente).
 *
 * Lleva lo necesario para (a) reintentarla (`accion`) y (b) describirla
 * (`dominio`, `registroId`, `tipo`, `descripcion`). Un valor por defecto
 * (`pendiente == false`) representa un registro guardado, sin cambio pendiente.
 */
struct OperacionPendiente {
    qint64 id = 0;                        ///< identidad de la operación pendiente (asignada por el gestor)
    QString dominio;                      ///< dominio lógico afectado ("docente", "aula", "curso", …)
    QString registroId;                   ///< clave del registro afectado (para el estado por registro)
    TipoOperacion tipo = TipoOperacion::Alta; ///< alta, modificación o baja
    bool pendiente = false;               ///< true si la escritura falló y el cambio sigue pendiente
    QString descripcion;                  ///< texto en español para la interfaz
    std::function<bool()> accion;         ///< repite la escritura; true = éxito
};

/**
 * @brief Función pura: estado de persistencia que representa una operación.
 *
 * Un valor por defecto (sin cambio pendiente) es `Guardado`; una alta o
 * modificación pendiente es `PendienteGuardar`; una baja pendiente es
 * `PendienteEliminar` (RF-3, RNF-4).
 */
EstadoPendiente estadoPendienteDe(const OperacionPendiente& operacion);

/**
 * @brief Estado en memoria de los cambios pendientes de reintento (RF-3).
 *
 * No persiste nada: los pendientes viven mientras dure la sesión. No impone
 * orden ni límite de acumulación (fuera de alcance de la spec).
 */
class GestorPendientes {
public:
    /**
     * @brief Registra una operación fallida como cambio pendiente.
     *
     * Asigna la identidad de la operación y la marca como pendiente.
     * @return Identificador con el que reintentarla o retirarla.
     */
    qint64 registrar(OperacionPendiente operacion);

    /// Indica si hay algún cambio pendiente (guardia de cierre, RF-6).
    bool hayPendientes() const;

    /// Lista los cambios pendientes, para mostrarlos y reintentarlos.
    QVector<OperacionPendiente> pendientes() const;

    /**
     * @brief Reintenta la operación pendiente `id`.
     *
     * Si tiene éxito, deja de estar pendiente; si vuelve a fallar, el pendiente
     * se conserva (no se pierde de pantalla, RNF-3).
     */
    Resultado<bool> reintentar(qint64 id);

    /// Estado por registro (dominio + clave) para pintar la fila (RNF-4).
    EstadoPendiente estadoDe(const QString& dominio, const QString& registroId) const;

    /// Busca el cambio pendiente de un registro; nullptr si está guardado.
    const OperacionPendiente* pendienteDe(const QString& dominio,
                                          const QString& registroId) const;

    /// Número de cambios pendientes.
    int contar() const;

    /// Descarta todos los pendientes (por ejemplo, al cerrar sin reintentar).
    void limpiar();

private:
    QVector<OperacionPendiente> m_pendientes;
    qint64 m_siguienteId = 1;
};
