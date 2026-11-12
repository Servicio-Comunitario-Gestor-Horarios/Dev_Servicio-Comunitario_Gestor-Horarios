#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QMap>
#include <QStringList>
#include <QVector>

namespace gestor::frontend::forms {

class TeacherFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit TeacherFormDialog(QWidget *parent = nullptr);
    ~TeacherFormDialog() override = default;

    // Datos del docente (las materias se gestionan en el panel de asignaturas).
    void cargarDatos(const QString& cedula, const QString& nombre,
                     const QString& email, const QString& telefono);

    // Cédulas ya registradas (cédula -> nombre) para avisar de duplicados.
    void setDocentesExistentes(const QMap<QString, QString>& cedulaANombre,
                               const QString& cedulaActual = QString());

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

    // Configuración opcional del generador (no forma parte del dominio).
    QComboBox *comboTurno;
    QComboBox *comboPlan;
    QComboBox *comboAnio;

    QPushButton *botonGuardar;
    QPushButton *botonCancelar;

    QMap<QString, QString> m_docentesExistentes;
    QString m_cedulaOriginal;
};

} // namespace gestor::frontend::forms
