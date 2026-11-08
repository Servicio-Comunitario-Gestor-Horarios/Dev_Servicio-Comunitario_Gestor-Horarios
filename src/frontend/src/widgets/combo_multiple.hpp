#pragma once

/**
 * @file combo_multiple.hpp
 * @brief Desplegable con selección múltiple (casillas) para la vista de
 *        generación de horarios.
 *
 * Es un `QComboBox` cuyos ítems se marcan y desmarcan **sin cerrar el popup**;
 * el control muestra un resumen de lo elegido en lugar del ítem actual. Se usa,
 * por ejemplo, para elegir los años en que puede dictar un docente.
 */

#include <QComboBox>
#include <QStringList>
#include <QVariantList>

class QEvent;
class QPaintEvent;

class ComboMultiple : public QComboBox
{
    Q_OBJECT

public:
    explicit ComboMultiple(QWidget* parent = nullptr);

    /**
     * @brief Añade los ítems marcables.
     * @param textos Textos visibles, en el orden en que se quieren mostrar.
     * @param datos  Dato asociado a cada texto (p. ej. el año); opcional.
     */
    void agregarOpciones(const QStringList& textos, const QVariantList& datos = {});

    /// Textos marcados, en el orden en que se agregaron.
    QStringList seleccionados() const;
    /// Datos asociados a los ítems marcados.
    QVariantList datosSeleccionados() const;

    /// Marca exactamente los ítems de la lista (y desmarca el resto).
    void setSeleccion(const QStringList& textos);
    /// Marca exactamente los ítems cuyos datos estén en la lista.
    void setSeleccionDatos(const QVariantList& datos);

    /// Texto que se muestra cuando no hay nada marcado (por defecto, «Todos»).
    void setTextoVacio(const QString& texto);

    /// Texto que resume la selección actual (lo que dibuja el control).
    QString resumen() const;

signals:
    /// Cambió la selección (marcar o desmarcar un ítem).
    void seleccionCambiada();

protected:
    bool eventFilter(QObject* vigilado, QEvent* evento) override;
    void paintEvent(QPaintEvent* evento) override;

private:
    /// Reprograma el texto mostrado (no cambia la selección).
    void refrescar();

    QString m_textoVacio;
};
