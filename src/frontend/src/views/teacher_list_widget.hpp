#ifndef TEACHER_LIST_WIDGET_HPP
#define TEACHER_LIST_WIDGET_HPP

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class TeacherListWidget : public QWidget {
    Q_OBJECT
public:
    explicit TeacherListWidget(QWidget *parent = nullptr);

private slots:
    void abrirFormularioNuevo();
    void agregarProfesorATabla(const QString& cedula, const QString& id,
                               const QString& nombre, const QString& email,
                               const QString& telefono);

private:
    QTableWidget *m_table;
    QPushButton *m_registerButton;
    void setupUi();
};

#endif // TEACHER_LIST_WIDGET_HPP
