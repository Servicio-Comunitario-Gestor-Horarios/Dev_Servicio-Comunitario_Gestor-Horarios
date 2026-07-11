/* Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        [teacher_list_widget.hpp]
 * Autor:          Paola Peña
 * Fecha:          10 de Julio de 2026
 * Descripción:    Implementación de la interfaz gráfica (Frontend) utilizando
 *                 C++ y Qt6 para el módulo de gestión de docentes. Incluye el
 *                 diseño de la vista principal, la tabla de listado de docentes,
 *                 y el formulario de registro con validación de datos.
 */

#ifndef TEACHER_LIST_WIDGET_HPP
#define TEACHER_LIST_WIDGET_HPP

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>

class TeacherListWidget : public QWidget {
    Q_OBJECT
public:
    explicit TeacherListWidget(QWidget *parent = nullptr);

    // 1. AGREGAMOS ESTA SECCIÓN PARA EL EVENTO DEL BOTÓN
private slots:
    void abrirFormularioNuevo();
    void agregarProfesorATabla(const QString& id, const QString& nombre, const QString& email, const QString& telefono, const QString& materias);

private:
    QTableWidget *m_table;
    QPushButton *m_registerButton; // Este es el botón que conectaremos
    void setupUi();
};

#endif // TEACHER_LIST_WIDGET_HPP