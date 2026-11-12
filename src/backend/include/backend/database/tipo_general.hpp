#pragma once

#include <QString>
#include <QVector>

/// Predicado de validez de la fila del tipo general, traducción del rechazo del
/// almacenamiento y textos compartidos por el esquema y los servicios (015).
namespace tipo_general
{
    /// Operación sobre el tipo general que está protegida.
    enum class OperacionProtegida
    {
        Borrado,
        Renombrado,
    };

    /// Variante **fija** de los mensajes de protección, para el `RAISE(ABORT, …)` de los
    /// disparadores (015 RF-3/RF-4): `CREATE TRIGGER` no admite parámetros y el texto no
    /// puede llevar el valor recibido (plan §2.3, P12). La variante del servicio, que sí
    /// incluye el valor, es `mensajeProteccionServicio`.
    inline QString mensajeProteccionAlmacenamiento(OperacionProtegida operacion)
    {
        switch (operacion)
        {
        case OperacionProtegida::Borrado:
            return QStringLiteral("No se puede eliminar el tipo de aula general del centro.");
        case OperacionProtegida::Renombrado:
            return QStringLiteral("No se puede renombrar el tipo de aula general del centro.");
        }
        return {};
    }

    /// Variante del **servicio**: incluye el valor recibido cuando la operación lo tiene
    /// (plan §2.3, P12). Es la única que cumple el mensaje de RNF-1 con valor.
    QString mensajeProteccionServicio(OperacionProtegida operacion,
                                      const QString& valorRecibido);

    /// Una fila del catálogo `TiposAula`, tal y como se lee de la base.
    struct Fila
    {
        int id = 0;
        QString nombre;
        QString clave;
    };

    /// Resultado del predicado único de validez de la fila (RF-2/RF-6).
    enum class EstadoFila
    {
        SinTipoGeneral, ///< No hay ninguna fila con el nombre normalizado canónico.
        VariasFilas,    ///< Hay más de una fila con el nombre normalizado canónico.
        ClaveDistinta,  ///< Hay una sola fila canónica, pero su clave almacenada no es la exigida.
        Valida,         ///< Exactamente una fila canónica con la clave almacenada exigida.
    };

    /// Evalúa la fila del tipo general sobre las filas leídas del catálogo (RF-2/RF-6).
    ///
    /// Puro y determinista: no toca la base, no depende del reloj ni del orden de entrada.
    /// Si se pasa `motivo`, recibe una explicación en español con el conteo y el valor
    /// esperado; en `Valida` queda vacío.
    EstadoFila evaluar(const QVector<Fila>& filas, QString* motivo = nullptr);

    /// Estado de la fila implicada en una escritura rechazada por el almacenamiento.
    struct EstadoFilaProtegida
    {
        bool existe = false;
        bool esGeneral = false;
    };

    /// Traduce un fallo del almacenamiento al mensaje del catálogo (RF-7, P4).
    ///
    /// Puro: `""` si no hay nada que traducir (no hubo fallo, la fila no existe o es de
    /// otra fila); si el fallo es de la fila general, el mensaje del servicio en español,
    /// que en el renombrado incluye el valor recibido.
    QString traducirRechazo(OperacionProtegida operacion, bool fallo,
                            const EstadoFilaProtegida& fila, const QString& valorRecibido);
} // namespace tipo_general
