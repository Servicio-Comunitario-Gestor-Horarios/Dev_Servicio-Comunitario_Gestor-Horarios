#pragma once

/**
 * @file selector_materias.hpp
 * @brief Selector de materias suplentes: botón de celda y diálogo con rejilla.
 *
 * La vista de generación ya no pide las materias suplentes como texto libre:
 * cada docente lleva un botón que resume su selección y abre un diálogo con una
 * rejilla de las materias existentes en la base, donde se marcan con un clic.
 */

#include <QDialog>
#include <QList>
#include <QPushButton>
#include <QStringList>

class QAbstractButton;

/**
 * @brief Botón de celda que resume las materias suplentes elegidas.
 *
 * Sin selección muestra una invitación a elegir; con pocas materias, sus
 * nombres; con muchas, el total. Al pulsarlo abre `SelectorMateriasDialog`.
 */
class BotonMateriasSuplentes : public QPushButton
{
    Q_OBJECT

public:
    explicit BotonMateriasSuplentes(QWidget* parent = nullptr);

    /// Materias entre las que se puede elegir (las existentes en la base).
    void setDisponibles(const QStringList& materias);
    /// Selección actual; se refleja en el texto del botón.
    void setSeleccion(const QStringList& materias);

    /// Materias elegidas.
    QStringList seleccion() const { return m_seleccion; }

signals:
    /// El usuario confirmó una selección distinta.
    void seleccionCambiada(const QStringList& materias);

private:
    void abrirSelector();
    void refrescarTexto();

    QStringList m_disponibles;
    QStringList m_seleccion;
};

/**
 * @brief Diálogo estilizado con una rejilla de materias marcables.
 *
 * Presenta las materias disponibles como botones conmutables en una rejilla y
 * devuelve la lista marcada al aceptar.
 */
class SelectorMateriasDialog : public QDialog
{
    Q_OBJECT

public:
    SelectorMateriasDialog(const QStringList& disponibles,
                           const QStringList& seleccionadas,
                           QWidget* parent = nullptr);

    /// Materias marcadas al aceptar (o al consultar en cualquier momento).
    QStringList seleccionadas() const;

private:
    QList<QAbstractButton*> m_botones;
};
