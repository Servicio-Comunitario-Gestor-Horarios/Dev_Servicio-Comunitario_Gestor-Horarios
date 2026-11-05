#include "backend/database/tipo_general.hpp"

#include "common/tipo_general.hpp"

namespace tipo_general
{
namespace
{
    /// Motivo para el caso sin ninguna fila canónica: conteo y valor esperado.
    QString motivoSinTipoGeneral()
    {
        return QStringLiteral("hay 0 filas cuyo nombre normalizado es «%1»; debe haber "
                              "exactamente una con la clave almacenada «%2».")
            .arg(nombre(), clave());
    }

    /// Motivo para el caso de varias filas canónicas: conteo y valor esperado.
    QString motivoVariasFilas(int cuantas)
    {
        return QStringLiteral("hay %1 filas cuyo nombre normalizado es «%2»; debe haber "
                              "exactamente una con la clave almacenada «%3».")
            .arg(cuantas)
            .arg(nombre(), clave());
    }

    /// Motivo para la fila única cuya clave almacenada no es la exigida.
    QString motivoClaveDistinta(const QString& almacenada)
    {
        return QStringLiteral("hay 1 fila cuyo nombre normalizado es «%1», pero su clave "
                              "almacenada es «%2»; se esperaba «%3».")
            .arg(nombre(), almacenada, clave());
    }
} // namespace

QString mensajeProteccionServicio(OperacionProtegida operacion, const QString& valorRecibido)
{
    switch (operacion)
    {
    case OperacionProtegida::Borrado:
        return QStringLiteral("No se puede eliminar el tipo de aula general del centro: "
                              "no admite borrado.");
    case OperacionProtegida::Renombrado:
        return QStringLiteral("No se puede renombrar el tipo de aula general del centro: "
                              "su nombre no admite cambios (recibido «%1»).")
            .arg(valorRecibido);
    }
    return {};
}

EstadoFila evaluar(const QVector<Fila>& filas, QString* motivo)
{
    int canonicas = 0;
    QString claveAlmacenada;
    for (const Fila& fila : filas)
    {
        if (claveNormalizada(fila.nombre) == clave())
        {
            ++canonicas;
            claveAlmacenada = fila.clave;
        }
    }

    EstadoFila estado = EstadoFila::Valida;
    QString texto;
    if (canonicas == 0)
    {
        estado = EstadoFila::SinTipoGeneral;
        texto = motivoSinTipoGeneral();
    }
    else if (canonicas > 1)
    {
        estado = EstadoFila::VariasFilas;
        texto = motivoVariasFilas(canonicas);
    }
    else if (claveAlmacenada != clave())
    {
        estado = EstadoFila::ClaveDistinta;
        texto = motivoClaveDistinta(claveAlmacenada);
    }

    if (motivo)
        *motivo = texto;
    return estado;
}

QString traducirRechazo(OperacionProtegida operacion, bool fallo,
                        const EstadoFilaProtegida& fila, const QString& valorRecibido)
{
    if (!fallo || !fila.existe || !fila.esGeneral)
        return {};

    return mensajeProteccionServicio(operacion, valorRecibido);
}
} // namespace tipo_general
