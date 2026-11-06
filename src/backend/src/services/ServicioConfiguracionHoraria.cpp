#include "backend/services/ServicioConfiguracionHoraria.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include "backend/services/duracion_horaria.hpp"

ServicioConfiguracionHoraria::ServicioConfiguracionHoraria(QSqlDatabase& db) : m_db(db) {}

Resultado<ConfiguracionHoraria> ServicioConfiguracionHoraria::consultar() const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT duracion_minutos FROM ConfiguracionHoraria WHERE id = 1"));

    if (!query.exec())
    {
        qCritical() << "Error al consultar la duración de la hora académica:"
                    << query.lastError().text();
        return Resultado<ConfiguracionHoraria>::error(
            QStringLiteral("No se pudo consultar la duración de la hora académica. Compruebe que"
                           " la base esté disponible e intente de nuevo."));
    }

    ConfiguracionHoraria configuracion;

    // Sin fila no es error: el valor efectivo es el defecto (RF-2).
    if (!query.next())
    {
        configuracion.duracionMinutos = duracion_horaria::POR_DEFECTO;
        configuracion.porDefecto = true;
        return Resultado<ConfiguracionHoraria>::exito(configuracion);
    }

    configuracion.duracionMinutos = query.value(0).toInt();
    configuracion.porDefecto = false;
    return Resultado<ConfiguracionHoraria>::exito(configuracion);
}

Resultado<ConfiguracionHoraria> ServicioConfiguracionHoraria::guardar(int duracionMinutos)
{
    // Punto único de validación del rango (RNF-4). Si falla, no se escribe nada.
    const QString error = duracion_horaria::errorDeRango(duracionMinutos);
    if (!error.isEmpty())
        return Resultado<ConfiguracionHoraria>::error(error);

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO ConfiguracionHoraria (id, duracion_minutos) VALUES (1, :duracion) "
        "ON CONFLICT(id) DO UPDATE SET duracion_minutos = excluded.duracion_minutos, "
        "fecha_modificacion = CURRENT_TIMESTAMP"));
    query.bindValue(QStringLiteral(":duracion"), duracionMinutos);

    if (!query.exec())
    {
        qCritical() << "Error al guardar la duración de la hora académica:"
                    << query.lastError().text();
        return Resultado<ConfiguracionHoraria>::error(
            QStringLiteral("No se pudo guardar la duración de la hora académica; intente de nuevo."));
    }

    return consultar();
}
