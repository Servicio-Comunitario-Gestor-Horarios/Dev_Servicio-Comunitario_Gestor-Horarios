/******************************************************************************
 * Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        classroom_form_dialog.hpp
 * Autor:          Paola
 * Fecha:          19 de Julio de 2026
 * Descripción:    Cabecera del formulario de registro y edición de aulas.
 ******************************************************************************/

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QString>
#include <QVector>

#include "backend/services/ServicioTiposAula.hpp"

namespace gestor::frontend::forms {

/**
 * @brief Diálogo de formulario para el registro y edición de aulas.
 *
 * Permite ingresar nombre, capacidad, edificio y piso de un aula.
 * Emite la señal aulaGuardada con los datos validados al guardar.
 */
class ClassroomFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit ClassroomFormDialog(QWidget *parent = nullptr);
    ~ClassroomFormDialog() override = default;

    /** @brief Precarga el catálogo de tipos de aula y el tipo del aula. */
    void cargarTiposAula(const QVector<TipoAulaDTO>& tipos, int idSeleccionado = -1);

    /** @brief Precarga datos del aula para modo edición. */
    void cargarDatos(const QString& nombre, int capacidad, const QString& edificio,
                     const QString& piso, int idTipoAula);

signals:
    void aulaGuardada(const QString& nombre, int capacidad, const QString& edificio,
                      const QString& piso, int idTipoAula);

private slots:
    void guardarAula();

private:
    QLineEdit *campoNombre;
    QSpinBox *campoCapacidad;
    QComboBox *campoTipoAula;
    QLineEdit *campoEdificio;
    QLineEdit *campoPiso;
    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms
