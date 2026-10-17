#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>

namespace gestor::frontend::forms {

class TeacherFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit TeacherFormDialog(QWidget *parent = nullptr);
    ~TeacherFormDialog() override = default;

    // Se eliminó 'id' y se agregó 'materias'
    void cargarDatos(const QString& cedula, const QString& nombre,
                     const QString& email, const QString& telefono,
                     const QString& materias);

signals:
    // Se eliminó 'id' y se agregó 'materias' para enviar a la base de datos
    void profesorGuardado(const QString& cedula, const QString& nombre,
                          const QString& email, const QString& telefono,
                          const QString& materias);

private slots:
    void guardarProfesor();

private:
    bool validarCampos();

    QLineEdit *campoCedula;
    QLineEdit *campoNombre;
    QLineEdit *campoEmail;
    QLineEdit *campoTelefono;
    QLineEdit *campoMaterias; // <-- NUEVO: Campo para la materia

    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms