#pragma once

/**
 * @file preferencias_docente_ui.hpp
 * @brief Preferencias del docente para la generación (turno, plan y año).
 *
 * Datos **de configuración del generador**, no de dominio: el contrato 2.0 los
 * lleva en el preset del docente (`turno`, `plan`, `anios`). La interfaz los
 * captura en el formulario del docente y los conserva en `QSettings` para
 * precargar la tabla de docentes de «Generación de horario».
 *
 * La organización/aplicación de `QSettings` se puede cambiar (los tests lo
 * aíslan en un espacio propio) para no tocar la configuración real.
 */

#include <QSettings>
#include <QString>

namespace PreferenciasDocenteUI {

struct Preferencia {
    QString turno;   ///< "" = sin preferencia.
    QString plan;    ///< "" = sin preferencia.
    int anio = 0;    ///< 0 = sin preferencia.
};

struct Organizacion {
    QString nombre = QStringLiteral("LiceoRobertSerra");
    QString aplicacion = QStringLiteral("GestorHorarios");
};

inline Organizacion& organizacion()
{
    static Organizacion actual;
    return actual;
}

/// Cambia el espacio de `QSettings` (útil para aislar pruebas).
inline void usarOrganizacion(const QString& nombre, const QString& aplicacion)
{
    organizacion().nombre = nombre;
    organizacion().aplicacion = aplicacion;
}

inline QString clave(const QString& idDocente, const QString& campo)
{
    return QStringLiteral("docentes/%1/%2").arg(idDocente, campo);
}

inline Preferencia leer(const QString& idDocente)
{
    Preferencia p;
    if (idDocente.isEmpty())
        return p;
    const QSettings settings(organizacion().nombre, organizacion().aplicacion);
    p.turno = settings.value(clave(idDocente, QStringLiteral("turno"))).toString();
    p.plan = settings.value(clave(idDocente, QStringLiteral("plan"))).toString();
    p.anio = settings.value(clave(idDocente, QStringLiteral("anio")), 0).toInt();
    if (p.anio < 0)
        p.anio = 0;
    return p;
}

inline void guardar(const QString& idDocente, const Preferencia& p)
{
    if (idDocente.isEmpty())
        return;
    QSettings settings(organizacion().nombre, organizacion().aplicacion);
    settings.setValue(clave(idDocente, QStringLiteral("turno")), p.turno);
    settings.setValue(clave(idDocente, QStringLiteral("plan")), p.plan);
    settings.setValue(clave(idDocente, QStringLiteral("anio")), p.anio);
}

inline void eliminar(const QString& idDocente)
{
    if (idDocente.isEmpty())
        return;
    QSettings settings(organizacion().nombre, organizacion().aplicacion);
    settings.remove(QStringLiteral("docentes/%1").arg(idDocente));
}

inline void limpiar()
{
    QSettings settings(organizacion().nombre, organizacion().aplicacion);
    settings.clear();
}

} // namespace PreferenciasDocenteUI
