#pragma once

#include <QDateTime>
#include <QString>

#include "seed/perfil.hpp"

namespace seed {

/// Desenlace de aplicar el perfil a la base de destino (RF-1, RNF-5).
enum class EstadoAplicacion {
    Ok,                    ///< Base poblada correctamente.
    UsoInvalido,           ///< Argumentos inválidos (ruta vacía).
    BaseConDatos,          ///< La base ya tiene datos de dominio y no se pidió reemplazo.
    ConfirmacionRequerida, ///< Operación destructiva sin confirmación explícita (--si).
    RespaldoFallido,       ///< No se pudo crear el respaldo previo.
    EsquemaNoEsperado,     ///< La base no tiene el esquema esperado.
    FalloApertura,         ///< No se pudo abrir o crear la base.
    FalloEscritura,        ///< Fallo al escribir (la transacción se revierte).
    PerfilIncoherente,     ///< El perfil o los docentes no pasan la validación.
};

/// Opciones de aplicación del conjunto único del centro (RF-1).
struct OpcionesAplicacion {
    QString rutaDb;                            ///< Obligatoria (--db).
    bool reemplazarTodo = false;               ///< --reemplazar (reemplazo total).
    bool confirmado     = false;               ///< --si (confirma operación destructiva).
    QDateTime ahora;                           ///< nombre del respaldo; vacío = reloj.
    bool simularFalloTrasDocentes = false;     ///< solo tests (atomicidad).
};

/// Resultado de aplicar el perfil: estado, detalle legible, ruta de respaldo y recuentos.
struct ResultadoAplicacion {
    EstadoAplicacion estado = EstadoAplicacion::FalloApertura;
    QString detalle;
    QString rutaRespaldo;
    Recuentos recuentos;   ///< incluye la duración efectiva y su fila de configuración

    bool ok() const { return estado == EstadoAplicacion::Ok; }
};

/// Aplica el conjunto del centro a la base `opciones.rutaDb` (la crea si no existe). Es
/// atómico: o se escriben todos los datos del perfil, o no queda ninguno (RF-1).
ResultadoAplicacion aplicar(const OpcionesAplicacion& opciones);

/// Ruta de respaldo disponible para la base `rutaDb` en el instante `ahora` (RF-1).
///
/// Parte de `Respaldo::construirNombreRespaldo(rutaDb, ahora)` y, si el archivo ya
/// existe, prueba `<ruta>-2`, `<ruta>-3`… hasta 99. Función pura y determinista:
/// no escribe la base ni el respaldo; solo decide el nombre.
QString rutaRespaldoLibre(const QString& rutaDb, const QDateTime& ahora);

/// Informe determinista de la aplicación (RF-8): hora académica efectiva y su recuento,
/// recuentos por entidad (leídos de la base), materias por año y totales semanales por plan
/// en horas académicas (= bloques). No incluye la ruta de la base ni la del respaldo: las
/// imprime la CLI, que dispone de ellas. La configuración viaja dentro de `recuentos` y no
/// se recalcula aquí. Pura.
QString resumenTexto(const Perfil& perfil, const Recuentos& recuentos);

} // namespace seed
