#pragma once

#include <QString>

/// Definición única del tipo de aula general/ordinario del centro (015 RF-2, RNF-5).
///
/// Es el **único** punto del código de producción donde aparece el literal del
/// nombre. La creación de la base, la guardia, la protección, el cálculo, los
/// mensajes, los datos de prueba y la interfaz lo consumen desde aquí.
namespace tipo_general
{
    /// Nombre visible convenido del tipo general.
    inline QString nombre() { return QStringLiteral("Aula de estudio"); }

    /// Único algoritmo de clave de tipo: recorte y plegado Unicode.
    inline QString claveNormalizada(const QString& n) { return n.trimmed().toCaseFolded(); }

    /// Clave almacenada del tipo general, derivada del nombre (no se declara aparte).
    inline QString clave() { return claveNormalizada(nombre()); }

    /// ¿El nombre visible `n` es el del tipo general, según la normalización única?
    inline bool esNombreCanonico(const QString& n) { return claveNormalizada(n) == clave(); }
} // namespace tipo_general
