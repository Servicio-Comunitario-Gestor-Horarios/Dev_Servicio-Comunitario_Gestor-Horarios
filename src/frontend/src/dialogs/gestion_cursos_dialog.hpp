#pragma once

/**
 * @file gestion_cursos_dialog.hpp
 * @brief Diálogo para crear y eliminar años y secciones de los cursos.
 *
 * Un año no existe por sí solo: en la base de datos un curso es la pareja
 * (año, sección). Por eso «crear un año» consiste en crear su primera sección y
 * «eliminar un año» borra todas las secciones de ese año. El diálogo no edita
 * turno, plan ni aula; eso se configura en la tarjeta «Cursos» de la vista de
 * generación.
 */

#include <QDialog>
#include <QVector>

class NucleoDatos;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTreeWidget;
class QTreeWidgetItem;

class GestionCursosDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param maxAnios Año máximo permitido (5 o 6). Si es 0 se lee de
     *                 Configuración → Avanzado.
     */
    explicit GestionCursosDialog(NucleoDatos* nucleo, QWidget* parent = nullptr,
                                 int maxAnios = 0);

signals:
    /// Se creó o eliminó al menos un curso (la vista de generación debe recargar).
    void cursosCambiados();

private slots:
    void agregarCurso();
    void eliminarSeleccionado();
    void actualizarSegunSeleccion();

private:
    void construirUi();
    void aplicarEstilo();
    /// Relee los cursos de la base y reconstruye el árbol.
    void recargar();
    /// Ajusta año y sección sugeridos a partir de lo seleccionado.
    void sugerirIdentidad();
    /// Habilita solo las letras libres del año elegido y conserva una válida.
    void actualizarSeccionesDisponibles();
    /// Selecciona la siguiente letra libre del año actual, si queda alguna.
    void seleccionarSiguienteLibre();
    /// Siguiente letra libre del año indicado (A, B, C…).
    QString siguienteSeccion(int anio) const;
    /// Año del elemento seleccionado, o 0 si no hay selección.
    int anioSeleccionado() const;
    void establecerEstado(const QString& texto, bool error = false);
    /// Confirma y elimina los cursos indicados; devuelve si se intentó eliminar.
    bool confirmarYEliminar(const QVector<int>& ids, const QString& titulo,
                            const QString& detalle);

    NucleoDatos* m_nucleo = nullptr;
    int m_maxAnios = 5;
    QTreeWidget* m_arbol = nullptr;
    QSpinBox* m_spinAnio = nullptr;
    QComboBox* m_comboSeccion = nullptr;
    QPushButton* m_btnAgregar = nullptr;
    QPushButton* m_btnEliminar = nullptr;
    QLabel* m_lblEstado = nullptr;
};
