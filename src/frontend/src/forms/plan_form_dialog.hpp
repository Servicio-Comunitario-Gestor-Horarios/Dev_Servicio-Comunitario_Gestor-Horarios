#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>

namespace gestor::frontend::forms {

class PlanFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlanFormDialog(const QStringList& materiasTotales, QWidget *parent = nullptr);

    // Método para precargar datos en caso de edición
    void cargarDatos(const QString& codigo, const QString& nombre, const QString& descripcion, const QStringList& materiasSeleccionadas);

signals:
    void planGuardado(const QString& codigo, const QString& nombre, const QString& descripcion, const QStringList& materias);

private slots:
    void agregarMateria();
    void quitarMateria();
    void guardarPlan();

private:
    bool validarCampos();

    QLineEdit *m_campoCodigo;
    QLineEdit *m_campoNombre;
    QTextEdit *m_campoDescripcion;

    QListWidget *m_listaDisponibles;
    QListWidget *m_listaSeleccionadas;

    QPushButton *m_btnAgregar;
    QPushButton *m_btnQuitar;

    QPushButton *m_botonGuardar;
    QPushButton *m_botonCancelar;
};

} // namespace gestor::frontend::forms