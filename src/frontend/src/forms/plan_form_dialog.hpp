#ifndef PLAN_FORM_DIALOG_HPP
#define PLAN_FORM_DIALOG_HPP

/**
 * @file plan_form_dialog.hpp
 * @brief Formulario de alta/edición de planes de estudio (Issue #97).
 *
 * El plan se compone de una lista única de materias con sus horas semanales.
 * Los planes nuevos son «globales»: las materias se guardan en el año 1 y la
 * composición las replica en los años 1–5 para el solver. Al editar un plan
 * existente con distribución por años se conserva el año de cada materia.
 */

#include <QDialog>
#include <QVector>

#include "backend/services/NucleoDatos.hpp"

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTextEdit;

namespace gestor::frontend::forms {

class PlanFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlanFormDialog(NucleoDatos* nucleo, QWidget* parent = nullptr);
    ~PlanFormDialog() override = default;

    /**
     * @brief Prepara el diálogo para crear un plan.
     *
     * @param codigoSugerido Código numérico propuesto (se puede editar).
     * @param nombreSugerido Nombre propuesto.
     * @param descripcion Descripción inicial.
     * @param materias Materias preseleccionadas (útil al duplicar).
     * @param global `true` para guardar una lista única replicada en 1–5.
     */
    void prepararCreacion(const QString& codigoSugerido = QString(),
                          const QString& nombreSugerido = QString(),
                          const QString& descripcion = QString(),
                          const QVector<CargaHorariaDTO>& materias = {},
                          bool global = true);

    /** @brief Prepara el diálogo para editar un plan existente. */
    void prepararEdicion(const PlanDTO& plan,
                         const QVector<CargaHorariaDTO>& materias);

signals:
    /// Se emite solo después de que la base de datos confirmó el guardado.
    void planGuardado();

private slots:
    void agregarMateriaSeleccionada();
    void quitarMateriaSeleccionada();
    void agregarPorDobleClic();
    void filtrarDisponibles(const QString& texto);
    void alternarModoGlobal(bool global);
    void actualizarTotales();
    void guardarPlan();

private:
    struct Seleccion {
        int idMateria = -1;
        QString nombre;
        int horas = 2;
        int curso = 1;
    };

    void setupUi();
    void aplicarTema();
    void cargarCatalogo();
    void refrescarDisponibles();
    void agregarFilaSeleccionada(const Seleccion& seleccion);
    QVector<Seleccion> seleccionActual() const;
    bool validarFormulario();
    bool esGlobal() const;
    QString sugerirCodigo() const;

    NucleoDatos* m_nucleo = nullptr;
    bool m_edicion = false;
    QString m_codigoOriginal;

    QLabel* m_titulo = nullptr;
    QLabel* m_subtitulo = nullptr;
    QLineEdit* m_codigo = nullptr;
    QLineEdit* m_nombre = nullptr;
    QTextEdit* m_descripcion = nullptr;
    QLineEdit* m_buscar = nullptr;

    QListWidget* m_disponibles = nullptr;
    QTableWidget* m_seleccionadas = nullptr;
    QPushButton* m_btnAgregar = nullptr;
    QPushButton* m_btnQuitar = nullptr;
    QCheckBox* m_global = nullptr;
    QLabel* m_horasTotales = nullptr;

    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnCancelar = nullptr;

    QVector<MateriaDTO> m_materias;
};

} // namespace gestor::frontend::forms

#endif // PLAN_FORM_DIALOG_HPP
