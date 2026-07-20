/******************************************************************************
 * Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        classroom_form_dialog.hpp
 * Autor:          Paola
 * Fecha:          19 de Julio de 2026
 * Descripción:    Cabecera del formulario de registro y edición de aulas.
 ******************************************************************************/

#ifndef CLASSROOM_FORM_DIALOG_H
#define CLASSROOM_FORM_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QString>

namespace gestor::frontend::forms {

class ClassroomFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit ClassroomFormDialog(QWidget *parent = nullptr);
    ~ClassroomFormDialog() override = default;

    // Método para pre-cargar datos al editar
    void cargarDatos(const QString& nombre, int capacidad, const QString& edificio, const QString& piso);

signals:
    // Señal con los campos exactos del ticket
    void aulaGuardada(const QString& nombre, int capacidad, const QString& edificio, const QString& piso);

private slots:
    void guardarAula();

private:
    QLineEdit *campoNombre;
    QSpinBox *campoCapacidad;
    QLineEdit *campoEdificio;
    QLineEdit *campoPiso;
    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms

#endif // CLASSROOM_FORM_DIALOG_H
