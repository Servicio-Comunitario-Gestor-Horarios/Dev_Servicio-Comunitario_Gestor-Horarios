#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QStringList>
#include <QList>

namespace gestor::frontend::forms {

struct MateriaPlanInfo {
    QString nombre;
    int horas;
};

class PlanFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlanFormDialog(const QStringList& materiasTotales, QWidget *parent = nullptr);

    void cargarDatos(const QString& codigo, const QString& nombre, const QString& descripcion, const QList<MateriaPlanInfo>& materiasSeleccionadas);

signals:
    void planGuardado(const QString& codigo, const QString& nombre, const QString& descripcion, const QList<MateriaPlanInfo>& materias, int totalHoras);

private slots:
    void agregarMateria();
    void quitarMateria();
    void guardarPlan();
    void actualizarTotalHoras();

private:
    bool validarCampos();

    QLineEdit *m_campoCodigo;
    QLineEdit *m_campoNombre;
    QTextEdit *m_campoDescripcion;

    QListWidget *m_listaDisponibles;
    QTableWidget *m_tableSeleccionadas;

    QPushButton *m_btnAgregar;
    QPushButton *m_btnQuitar;

    QPushButton *m_botonGuardar;
    QPushButton *m_botonCancelar;
};

} // namespace gestor::frontend::forms