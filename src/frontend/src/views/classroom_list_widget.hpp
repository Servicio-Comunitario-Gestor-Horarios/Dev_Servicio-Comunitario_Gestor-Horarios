#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class NucleoDatos;

class ClassroomListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClassroomListWidget(QWidget *parent = nullptr);
    ~ClassroomListWidget() = default;

    void setNucleoDatos(NucleoDatos* nucleo);
    void actualizarTema(bool modoOscuro);

public slots:
    void cargarAulas();

private slots:
    void abrirFormularioNuevo();

private:
    void setupUi();

    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;

    NucleoDatos* m_nucleo = nullptr;

    // NUEVO: Punteros para actualizar los contadores del panel derecho dinámicamente
    QLabel* m_badgeAulasTeoricas = nullptr;
    QLabel* m_badgeLaboratorios = nullptr;
    QLabel* m_badgeCanchas = nullptr;
};