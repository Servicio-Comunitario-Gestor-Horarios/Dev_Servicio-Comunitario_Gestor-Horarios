#pragma once

#include <QString>

#include "common/tipo_general.hpp"

/// Aforo simultáneo de un tipo de aula (spec 018, RF-1/RF-2).
///
/// Es el **único** punto del código de producción donde viven el literal del tipo de la
/// cancha y su aforo. El motor, la capacidad, la comprobación de la salida y el seed lo
/// consumen desde aquí: nadie más compara el nombre del tipo.
///
/// El aforo es una **regla del sistema por tipo de aula** (cancha → 2, resto → 1),
/// **hardcodeada**: no viaja en el contrato, no toca el esquema, no entra en los presets y
/// no es configurable. Es de **cursos simultáneos**, no la capacidad física de alumnos.
namespace aforo_aula
{
    /// Nombre visible convenido del tipo de la cancha.
    inline QString tipoCancha() { return QStringLiteral("Cancha"); }

    /// Clave almacenada del tipo de la cancha, derivada del nombre con la normalización única.
    inline QString claveCancha() { return tipo_general::claveNormalizada(tipoCancha()); }

    /// Aforo del tipo por su clave normalizada: cancha → 2; cualquier otro → 1.
    inline int aforoDeClave(const QString& clave) { return clave == claveCancha() ? 2 : 1; }
} // namespace aforo_aula
