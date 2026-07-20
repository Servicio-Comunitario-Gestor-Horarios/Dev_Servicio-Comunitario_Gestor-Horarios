#ifndef TEACHER_FORM_DIALOG_HPP
#define TEACHER_FORM_DIALOG_HPP

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

    void cargarDatos(const QString& cedula, const QString& id,
                     const QString& nombre, const QString& email,
                     const QString& telefono);

signals:
    void profesorGuardado(const QString& cedula, const QString& id,
                          const QString& nombre, const QString& email,
                          const QString& telefono);

private slots:
    void guardarProfesor();

private:
    bool validarCampos();

    QLineEdit *campoCedula;
    QLineEdit *campoId;
    QLineEdit *campoNombre;
    QLineEdit *campoEmail;
    QLineEdit *campoTelefono;
    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms

#endif // TEACHER_FORM_DIALOG_HPP