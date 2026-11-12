#pragma once

/**
 * @file preferencias_generacion.hpp
 * @brief Preferencias de generación compartidas entre Configuración y la vista
 *        de generación.
 *
 * En el país del centro los liceos regulares llegan hasta 5to año y los
 * técnicos hasta 6to. El máximo se elige en Configuración → Avanzado y se
 * guarda en QSettings para que el diálogo de años y secciones solo permita
 * crear cursos dentro de ese rango.
 */

#include <QSettings>
#include <QString>
#include <QtGlobal>

namespace PreferenciasGeneracion {

inline constexpr int kMaxAniosDefecto = 5;
inline constexpr int kMaxAniosMinimo = 5;
inline constexpr int kMaxAniosMaximo = 6;

inline const char* kClaveMaxAnios = "generacion/maxAnios";

/// Máximo de años configurado (5 o 6), siempre dentro del rango válido.
inline int maxAnios()
{
    const QSettings settings(QStringLiteral("LiceoRobertSerra"),
                             QStringLiteral("GestorHorarios"));
    return qBound(kMaxAniosMinimo,
                  settings.value(QLatin1String(kClaveMaxAnios), kMaxAniosDefecto).toInt(),
                  kMaxAniosMaximo);
}

/// Guarda el máximo de años, acotándolo al rango permitido.
inline void guardarMaxAnios(int valor)
{
    QSettings settings(QStringLiteral("LiceoRobertSerra"),
                       QStringLiteral("GestorHorarios"));
    settings.setValue(QLatin1String(kClaveMaxAnios),
                      qBound(kMaxAniosMinimo, valor, kMaxAniosMaximo));
}

} // namespace PreferenciasGeneracion
