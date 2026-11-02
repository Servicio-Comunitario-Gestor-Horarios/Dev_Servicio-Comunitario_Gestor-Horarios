#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QStringList>
#include <QVector>

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

    /**
     * @brief Rellena los desplegables de configuración del generador.
     *
     * Todos son opcionales: el primer ítem de cada uno es «sin preferencia».
     */
    void cargarOpcionesGeneracion(const QStringList& turnos,
                                  const QStringList& planes,
                                  const QVector<int>& anios);

    /** @brief Precarga las preferencias guardadas del docente (opcionales). */
    void cargarPreferencias(const QString& turno, const QString& plan, int anio);

    QString cedula() const;
    QString turnoSeleccionado() const;
    QString planSeleccionado() const;
    int anioSeleccionado() const;

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

    // Configuración opcional del generador (no forma parte del dominio).
    QComboBox *comboTurno;
    QComboBox *comboPlan;
    QComboBox *comboAnio;

    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms
