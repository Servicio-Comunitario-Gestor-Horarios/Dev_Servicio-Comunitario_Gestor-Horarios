#ifndef SUBJECT_LIST_WIDGET_HPP
#define SUBJECT_LIST_WIDGET_HPP

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class SubjectListWidget : public QWidget {
    Q_OBJECT
public:
    explicit SubjectListWidget(QWidget *parent = nullptr);

private slots:
    void abrirFormularioNuevo();
    void agregarMateriaATabla(const QString& nombre, const QString& tipoAula);

private:
    QTableWidget *m_table;
    QPushButton *m_registerButton;
    void setupUi();
};

#endif // SUBJECT_LIST_WIDGET_HPP