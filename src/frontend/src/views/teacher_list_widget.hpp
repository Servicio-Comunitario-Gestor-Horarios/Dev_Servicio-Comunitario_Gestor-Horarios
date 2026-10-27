/**
 * @file teacher_list_widget.hpp
 * @brief Widget para el listado y gestión de docentes
 */

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QSet>
#include <QPair>

// Declaración adelantada
class NucleoDatos;
class InternalClient;

// Tipo auxiliar para identificar una celda (Fila, Columna) de la rejilla
typedef QPair<int, int> CeldaRejilla;

class TeacherListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TeacherListWidget(QWidget *parent = nullptr);
    ~TeacherListWidget() = default;

    void setNucleoDatos(NucleoDatos* nucleo);
    void setClient(InternalClient* client);
    void actualizarTema(bool modoOscuro);

public slots:
    void cargarDocentes();

private slots:
    // Flujo CRUD de Docentes
    void abrirFormularioNuevo();
    void abrirFormularioEditar(int fila);
    void eliminarDocente(int fila);

    // Selección principal
    void onDocenteSeleccionado();

    // Sistema de Pestañas
    void cambiarPestana(int indice);

    // Interacción dinámica en las rejillas
    void onDisponibilidadCellClicked(int fila, int columna);
    void onPreferenciasCellClicked(int fila, int columna);

    // Control de cambios
    void guardarCambiosRejilla();
    void descartarCambiosRejilla();

private:
    void setupUi();
    void setupPanelDerecho(class QVBoxLayout* rightLayout);

    // Funciones auxiliares para la rejilla dinámica
    void construirRejillasDinamicas();
    void repintarRejillas();
    void evaluarCambiosSinGuardar();

    // Punteros de interfaz izquierda
    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;

    // Punteros de interfaz derecha (Cabecera)
    QLabel* m_detalleNombre = nullptr;

    // Sistema de Pestañas
    QPushButton* m_btnTabDisponibilidad = nullptr;
    QPushButton* m_btnTabPreferencias = nullptr;
    QPushButton* m_btnTabAsignaturas = nullptr;
    QStackedWidget* m_stackedViews = nullptr;

    // Contenido de las pestañas
    QTableWidget* m_gridDisponibilidad = nullptr;
    QTableWidget* m_gridPreferencias = nullptr;
    QTableWidget* m_tableAsignaturas = nullptr; // O QListWidget

    // Controles de Guardado
    QWidget* m_panelGuardado = nullptr;
    QPushButton* m_btnGuardar = nullptr;
    QPushButton* m_btnDescartar = nullptr;

    // Punteros de datos
    NucleoDatos* m_nucleo = nullptr;
    InternalClient* m_client = nullptr;

    // --- ESTADO INTERNO DEL PANEL DERECHO ---
    bool m_isDark = false;
    int m_idDocenteActual = -1;

    // Control de estado para saber qué se ha pintado (Original vs Modificado)
    QSet<CeldaRejilla> m_noDisponibleOriginal;
    QSet<CeldaRejilla> m_noDisponibleModificado;

    QSet<CeldaRejilla> m_preferidoOriginal;
    QSet<CeldaRejilla> m_preferidoModificado;
};

// Necesario para que QSet pueda manejar nuestro typedef QPair
inline uint qHash(const CeldaRejilla &key, uint seed = 0) {
    return qHash(key.first, seed) ^ qHash(key.second, seed);
}