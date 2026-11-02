#pragma once

/**
 * @file preferencias_docente_ui.hpp
 * @brief Preferencias del docente para la generación (turno, plan y año).
 *
 * Datos **de configuración del generador**, no de dominio: el contrato 2.0 los
 * lleva en el preset del docente (`turno`, `plan`, `anios`). La interfaz los
 * captura en el formulario del docente y los conserva en `QSettings` para
 * precargar la tabla de docentes de «Generación de horario».
 */

#include <QSettings>
#include <QString>

namespace PreferenciasDocenteUI {

struct Preferencia {
    QString turno;   ///< "" = sin preferencia.
    QString plan;    ///< "" = sin preferencia.
    int anio = 0;    ///< 0 = sin preferencia.
};

inline QString clave(const QString& idDocente, const QString& campo)
{
    return QStringLiteral("docentes/%1/%2").arg(idDocente, campo);
}

inline Preferencia leer(const QSettings& settings, const QString& idDocente)
{
    Preferencia p;
    if (idDocente.isEmpty())
        return p;
    p.turno = settings.value(clave(idDocente, QStringLiteral("turno"))).toString();
    p.plan = settings.value(clave(idDocente, QStringLiteral("plan"))).toString();
    p.anio = settings.value(clave(idDocente, QStringLiteral("anio")), 0).toInt();
    if (p.anio < 0)
        p.anio = 0;
    return p;
}

inline void guardar(QSettings& settings, const QString& idDocente,
                    const Preferencia& p)
{
    if (idDocente.isEmpty())
        return;
    settings.setValue(clave(idDocente, QStringLiteral("turno")), p.turno);
    settings.setValue(clave(idDocente, QStringLiteral("plan")), p.plan);
    settings.setValue(clave(idDocente, QStringLiteral("anio")), p.anio);
}

inline void eliminar(QSettings& settings, const QString& idDocente)
{
    if (!idDocente.isEmpty())
        settings.remove(QStringLiteral("docentes/%1").arg(idDocente));
}

} // namespace PreferenciasDocenteUI
