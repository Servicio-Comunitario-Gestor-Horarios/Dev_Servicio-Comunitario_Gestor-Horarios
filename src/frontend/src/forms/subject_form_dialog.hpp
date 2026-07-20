#ifndef SUBJECT_FORM_DIALOG_HPP
#define SUBJECT_FORM_DIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

namespace gestor::frontend::forms {

class SubjectFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit SubjectFormDialog(QWidget *parent = nullptr);
    ~SubjectFormDialog() override = default;

    void cargarDatos(const QString& nombre, const QString& tipoAula);

signals:
    void materiaGuardada(const QString& nombre, const QString& tipoAula);

private slots:
    void guardarMateria();

private:
    bool validarCampos();

    QLineEdit   *m_campoNombre;
    QComboBox   *m_comboTipoAula;
    QPushButton *m_botonGuardar;
    QPushButton *m_botonCancelar;
};

} // namespace gestor::frontend::forms

#endif // SUBJECT_FORM_DIALOG_HPP