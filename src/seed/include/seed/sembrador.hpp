#pragma once

#include <QDateTime>
#include <QString>

#include "seed/docentes.hpp"   // VarianteDocentes
#include "seed/perfil.hpp"     // Perfil, Recuentos

namespace seed {

/// Desenlace de aplicar el perfil a la base de destino (RF-10, RNF-3).
enum class EstadoAplicacion {
    Ok,                    ///< Base poblada correctamente.
    UsoInvalido,           ///< Argumentos inválidos (ruta vacía u operaciones contradictorias).
    BaseConDatos,          ///< La base ya tiene datos de dominio y no se pidió reemplazo.
    VarianteDesconocida,   ///< Variante de docentes inexistente.
    ConfirmacionRequerida, ///< Operación destructiva sin confirmación explícita (--si).
    RespaldoFallido,       ///< No se pudo crear el respaldo previo.
    EsquemaNoEsperado,     ///< La base no tiene el esquema esperado.
    FalloApertura,         ///< No se pudo abrir o crear la base.
    FalloEscritura,        ///< Fallo al escribir (la transacción se revierte).
    PerfilIncoherente,     ///< El perfil no pasa la validación.
};

/// Opciones de aplicación del conjunto de datos de prueba (RF-10).
struct OpcionesAplicacion {
    QString rutaDb;                                  ///< Obligatoria (--db).
    VarianteDocentes variante = VarianteDocentes::Holgada;
    bool reemplazarTodo = false;                     ///< --reemplazar (reemplazo total, T6).
    bool soloDocentes   = false;                     ///< --solo-docentes (conmutación, T6).
    bool confirmado     = false;                     ///< --si (confirma operación destructiva, T6).
    QDateTime ahora;                                 ///< nombre del respaldo; vacío = reloj (T6).
    bool simularFalloTrasDocentes = false;           ///< solo tests (atomicidad, T5b).
};

/// Resultado de aplicar el perfil: estado, detalle legible, ruta de respaldo y recuentos.
struct ResultadoAplicacion {
    EstadoAplicacion estado = EstadoAplicacion::FalloApertura;
    QString detalle;
    QString rutaRespaldo;
    Recuentos recuentos;

    /// Valor efectivo de la hora académica que queda tras la siembra (35, RF-8).
    int duracionMinutos = 0;
    /// Filas de `ConfiguracionHoraria` tras la siembra (siempre 1, RF-8).
    int configuraciones = 0;

    bool ok() const { return estado == EstadoAplicacion::Ok; }
};

/// Aplica el perfil a la base `opciones.rutaDb` (la crea si no existe). Es atómico:
/// o se escriben todos los datos del perfil, o no queda ninguno (RF-10, RNF-6).
ResultadoAplicacion aplicar(const OpcionesAplicacion& opciones);

/// Ruta de respaldo disponible para la base `rutaDb` en el instante `ahora` (RF-10).
///
/// Parte de `Respaldo::construirNombreRespaldo(rutaDb, ahora)` y, si el archivo ya
/// existe, prueba `<ruta>-2`, `<ruta>-3`… hasta 99. Función pura y determinista:
/// no escribe la base ni el respaldo; solo decide el nombre.
QString rutaRespaldoLibre(const QString& rutaDb, const QDateTime& ahora);

/// Informe determinista de la aplicación (RF-12, plan §8): variante, hora académica efectiva
/// y su recuento, recuentos por entidad (leídos de la base), materias por año y totales
/// semanales por plan en horas académicas (= bloques). No incluye la ruta de la base ni la del
/// respaldo: las imprime la CLI, que dispone de ellas. La variante y la configuración viajan
/// como parámetros y no forman parte de `Recuentos`.
QString resumenTexto(const Perfil& perfil, VarianteDocentes variante, const Recuentos& recuentos,
                     int duracionMinutos, int configuraciones);

} // namespace seed
