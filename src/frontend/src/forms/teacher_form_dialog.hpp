#ifndef TEACHER_FORM_DIALOG_HPP
#define TEACHER_FORM_DIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QRegularExpressionValidator>
#include <QMessageBox>
#include <QString>

namespace gestor::frontend::forms {

class TeacherFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit TeacherFormDialog(QWidget *parent = nullptr);
    ~TeacherFormDialog() override = default;

    void cargarDatos(const QString& id, const QString& nombre, const QString& email, const QString& telefono, const QString& materias);

signals:
    void profesorGuardado(const QString& id, const QString& nombre, const QString& email, const QString& telefono, const QString& materias);

private slots:
    void guardarProfesor();

private:
    void configurarValidadores();

    QLineEdit *campoId;
    QLineEdit *campoNombre;
    QLineEdit *campoEmail;
    QLineEdit *campoTelefono;
    QLineEdit *campoMaterias;
    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms

#endif // TEACHER_FORM_DIALOG_HPP