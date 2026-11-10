/**
 * @file teacher_list_widget.hpp
 * @brief Listado de docentes con panel de disponibilidad, preferencias y materias.
 *
 * El panel derecho implementa las issues #94 y #95: pestañas reales, rejilla
 * construida con los turnos/recesos y la duración de franja de la base, y
 * gestión de materias titulares/suplentes. Todo se guarda en la base de datos
 * mediante `NucleoDatos::guardarFichaDocente`.
 */

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QSet>
#include <QStringList>
#include <QTime>
#include <QVector>

class NucleoDatos;
class InternalClient;
class QTabWidget;
class QListWidget;

class TeacherListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TeacherListWidget(QWidget *parent = nullptr);
    ~TeacherListWidget() override = default;

    void setNucleoDatos(NucleoDatos* nucleo);
    void setClient(InternalClient* client);
    void actualizarTema(bool modoOscuro);

public slots:
    void cargarDocentes();

private slots:
    void abrirFormularioNuevo();
    void abrirFormularioEditar(int fila);
    void eliminarDocente(int fila);
    void onDocenteSeleccionado();

    void onDisponibilidadCellClicked(int fila, int columna);
    void onPreferenciasCellClicked(int fila, int columna);
    void gestionarTitulares();
    void gestionarSuplentes();
    void guardarCambios();
    void descartarCambios();

private:
    /// Una fila de las rejillas: una hora académica o un evento/receso.
    struct FilaRejilla {
        bool receso = false;
        QString turno;
        int slot = -1;
        QTime inicioSlot;   ///< Hora del bloque según el modelo del solver.
        QTime finSlot;
        QString etiqueta;   ///< Hora real mostrada al usuario.
    };

    void setupUi();
    void construirRejillas();
    void repintarRejillas();
    void repintarAsignaturas();
    void actualizarEstadoCambios();
    void actualizarTitulosPestanas();

    void cargarDocenteActual();
    void limpiarPanelDocente();
    void seleccionarFilaPorCedula(const QString& cedula);
    bool pedirConfirmacionCambios();

    bool tieneCambios() const;
    bool celdaDisponible(int fila, int columna) const;

    NucleoDatos* m_nucleo = nullptr;
    InternalClient* m_client = nullptr;

    // Columna izquierda
    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;

    // Columna derecha
    QLabel* m_detalleNombre = nullptr;
    QTabWidget* m_tabs = nullptr;
    QTableWidget* m_gridDisponibilidad = nullptr;
    QTableWidget* m_gridPreferencias = nullptr;
    QListWidget* m_listTitulares = nullptr;
    QListWidget* m_listSuplentes = nullptr;
    QPushButton* m_btnGestionarTitulares = nullptr;
    QPushButton* m_btnGestionarSuplentes = nullptr;
    QLabel* m_avisoMaterias = nullptr;
    QLabel* m_estadoGuardado = nullptr;
    QWidget* m_panelGuardado = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnDescartar = nullptr;

    // Estado de la ficha seleccionada
    QVector<FilaRejilla> m_filas;
    QString m_cedulaActual;
    int m_filaActual = -1;
    bool m_gridConstruida = false;
    bool m_isDark = false;

    QSet<int> m_noDisponible;
    QSet<int> m_noDisponibleOriginal;
    QSet<int> m_preferidas;
    QSet<int> m_preferidasOriginal;
    QSet<int> m_evitadas;
    QSet<int> m_evitadasOriginal;

    QStringList m_titulares;
    QStringList m_titularesOriginal;
    QStringList m_suplentes;
    QStringList m_suplentesOriginal;
};
