#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

/**
 * @brief Widget para visualizar y gestionar la lista de aulas.
 *
 * Este componente muestra una tabla con las aulas registradas,
 * un panel de resumen lateral y permite abrir el formulario para
 * agregar o editar aulas.
 */
class ClassroomListWidget : public QWidget {
    Q_OBJECT

public:
    explicit ClassroomListWidget(QWidget *parent = nullptr);
    ~ClassroomListWidget() override = default;

private slots:
    /** @brief Abre el diálogo para registrar una nueva aula. */
    void abrirFormularioNuevo();

    /**
     * @brief Agrega una nueva fila a la tabla con los datos del aula.
     * @param nombre Nombre del aula.
     * @param capacidad Capacidad máxima de estudiantes.
     * @param edificio Edificio donde se encuentra.
     * @param piso Piso donde se ubica.
     */
    void agregarAulaATabla(const QString& nombre, int capacidad, const QString& edificio, const QString& piso);

private:
    void setupUi();
    QTableWidget *m_table;
    QPushButton *m_registerButton;
};
