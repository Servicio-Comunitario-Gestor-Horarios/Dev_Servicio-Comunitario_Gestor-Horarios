#ifndef SUBJECT_FORM_DIALOG_HPP
#define SUBJECT_FORM_DIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVector>

#include "backend/services/ServicioTiposAula.hpp"

namespace gestor::frontend::forms {

class SubjectFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit SubjectFormDialog(QWidget *parent = nullptr);
    ~SubjectFormDialog() override = default;

    /** @brief Precarga el catálogo de tipos de aula y el tipo de la materia. */
    void cargarTiposAula(const QVector<TipoAulaDTO>& tipos, int idSeleccionado = -1);

    /**
     * @brief Precarga los datos de una asignatura existente.
     *
     * `requisitos` se conserva en el modelo y en la base de datos (el solver lo
     * usa para las precedencias), pero ya no se edita desde este formulario.
     */
    void cargarDatos(const QString& nombre, const QString& requisitos, int idTipoAula);

signals:
    void materiaGuardada(const QString& nombre, const QString& requisitos, int idTipoAula);

private slots:
    void guardarMateria();

private:
    bool validarCampos();

    QLineEdit   *m_campoNombre;
    QComboBox   *m_comboTipoAula;
    QPushButton *m_botonGuardar;
    QPushButton *m_botonCancelar;
    QString      m_requisitos; ///< Se preserva al editar; vacío en altas nuevas.
};

} // namespace gestor::frontend::forms

#endif // SUBJECT_FORM_DIALOG_HPP