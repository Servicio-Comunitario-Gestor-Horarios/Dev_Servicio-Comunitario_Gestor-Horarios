/**
 * @file teacher_list_widget.hpp
 * @brief Widget para el listado y gestión de docentes
 */

#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

// Declaración adelantada
class NucleoDatos;
class InternalClient;

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
    void abrirFormularioNuevo();
    void abrirFormularioEditar(int fila);
    void eliminarDocente(int fila);

    // Función que detecta el clic en un profesor
    void onDocenteSeleccionado();

private:
    void setupUi();

    // Punteros de interfaz izquierda
    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;

    // Punteros de interfaz derecha (Panel de Detalles)
    QTableWidget* m_gridDisponibilidad = nullptr;
    QLabel* m_detalleNombre = nullptr;

    // Punteros de datos
    NucleoDatos* m_nucleo = nullptr;
    InternalClient* m_client = nullptr;
};