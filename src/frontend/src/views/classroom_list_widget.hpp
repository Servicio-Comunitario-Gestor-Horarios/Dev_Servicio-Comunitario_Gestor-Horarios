#ifndef CLASSROOM_LIST_WIDGET_HPP
#define CLASSROOM_LIST_WIDGET_HPP

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class ClassroomListWidget : public QWidget {
    Q_OBJECT

public:
    explicit ClassroomListWidget(QWidget *parent = nullptr);
    ~ClassroomListWidget() override = default;

private slots:
    void abrirFormularioNuevo();
    void agregarAulaATabla(const QString& nombre, int capacidad, const QString& edificio, const QString& piso);

private:
    void setupUi();
    QTableWidget *m_table;
    QPushButton *m_registerButton;
};

#endif // CLASSROOM_LIST_WIDGET_HPP
