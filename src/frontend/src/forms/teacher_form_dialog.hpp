#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QMap>

namespace gestor::frontend::forms {

class TeacherFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit TeacherFormDialog(QWidget *parent = nullptr);
    ~TeacherFormDialog() override = default;

    // Cargar datos sin materias
    void cargarDatos(const QString& cedula, const QString& nombre,
                     const QString& email, const QString& telefono);

    // Método para validar cédulas duplicadas
    void setDocentesExistentes(const QMap<QString, QString>& cedulaANombre, const QString& cedulaActual = QString());

signals:
    // Señal sin materias y sin ID
    void profesorGuardado(const QString& cedula, const QString& nombre,
                          const QString& email, const QString& telefono);

private slots:
    void guardarProfesor();

private:
    bool validarCampos();

    QLineEdit *campoCedula;
    QLineEdit *campoNombre;
    QLineEdit *campoEmail;
    QLineEdit *campoTelefono;

    QPushButton *botonGuardar;
    QPushButton *botonCancelar;

    QMap<QString, QString> m_docentesExistentes;
    QString m_cedulaOriginal;
};

} // namespace gestor::frontend::forms