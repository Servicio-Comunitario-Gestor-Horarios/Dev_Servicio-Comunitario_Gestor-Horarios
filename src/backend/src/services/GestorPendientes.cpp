#include "backend/services/GestorPendientes.hpp"

EstadoPendiente estadoPendienteDe(const OperacionPendiente& operacion) {
    if (!operacion.pendiente) {
        return EstadoPendiente::Guardado;
    }
    return operacion.tipo == TipoOperacion::Baja ? EstadoPendiente::PendienteEliminar
                                                 : EstadoPendiente::PendienteGuardar;
}

qint64 GestorPendientes::registrar(OperacionPendiente operacion) {
    operacion.id = m_siguienteId++;
    operacion.pendiente = true;
    m_pendientes.append(operacion);
    return operacion.id;
}

bool GestorPendientes::hayPendientes() const {
    return !m_pendientes.isEmpty();
}

QVector<OperacionPendiente> GestorPendientes::pendientes() const {
    return m_pendientes;
}

Resultado<bool> GestorPendientes::reintentar(qint64 id) {
    for (int i = 0; i < m_pendientes.size(); ++i) {
        if (m_pendientes.at(i).id != id) {
            continue;
        }

        const OperacionPendiente& operacion = m_pendientes.at(i);
        const bool exito = operacion.accion ? operacion.accion() : false;
        if (exito) {
            m_pendientes.removeAt(i);
            return Resultado<bool>::exito(true);
        }

        // El reintento volvió a fallar: el cambio sigue pendiente (RNF-3).
        return Resultado<bool>::error(
            QStringLiteral("El reintento falló; el cambio sigue pendiente."));
    }

    return Resultado<bool>::error(
        QStringLiteral("No existe un cambio pendiente con ese identificador."), -2);
}

EstadoPendiente GestorPendientes::estadoDe(const QString& dominio,
                                           const QString& registroId) const {
    const OperacionPendiente* operacion = pendienteDe(dominio, registroId);
    return operacion ? estadoPendienteDe(*operacion) : EstadoPendiente::Guardado;
}

const OperacionPendiente* GestorPendientes::pendienteDe(const QString& dominio,
                                                        const QString& registroId) const {
    for (const OperacionPendiente& operacion : m_pendientes) {
        if (operacion.dominio == dominio && operacion.registroId == registroId) {
            return &operacion;
        }
    }
    return nullptr;
}

int GestorPendientes::contar() const {
    return m_pendientes.size();
}

void GestorPendientes::limpiar() {
    m_pendientes.clear();
}
