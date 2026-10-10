#pragma once

/**
 * @file ordenacion.hpp
 * @brief Comparación de textos en español (acentos, ñ y mayúsculas).
 *
 * `QString::localeAwareCompare` depende del locale del sistema; en entornos con
 * locale C/POSIX degrada al orden por codepoint, donde «Á» (U+00C1) queda después
 * de «Z». Este helper usa `QCollator` con locale español (parte de Qt6Core, ya
 * enlazado por el ejecutable), de modo que el orden es correcto y determinista
 * con independencia del locale del sistema.
 *
 * Header-only y sin estado propio: se compila dentro de la librería `backend`,
 * que a su vez forma parte del único binario `gestor-horarios`.
 */

#include <QCollator>
#include <QLocale>
#include <QString>

/// Colador español compartido (singleton de función, inicialización thread-safe).
inline const QCollator& coladorEspanol()
{
    static const QCollator colador = [] {
        QCollator c(QLocale(QLocale::Spanish, QLocale::Spain));
        c.setCaseSensitivity(Qt::CaseInsensitive);
        c.setNumericMode(false);
        c.setIgnorePunctuation(false);
        return c;
    }();
    return colador;
}

/// Ordena `a` antes que `b` según la collation española, con desempate estable
/// por el texto original cuando la collation los considera iguales.
inline bool nombreAntes(const QString& a, const QString& b)
{
    const int comparacion = coladorEspanol().compare(a, b);
    return comparacion != 0 ? comparacion < 0 : a < b;
}
