#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QJsonObject>

class InternalClient;

class SubjectListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SubjectListWidget(QWidget *parent = nullptr);
    void setClient(InternalClient* client);

private slots:
    void onRespuestaRecibida(const QJsonObject& respuesta);
    void abrirFormularioNuevo();

private:
    void setupUi();
    void insertarMateriaEnTabla(const QString& nombre, const QString& tipoAula);

    void enviarCrearMateria(const QString& nombre, const QString& tipoAula);
    void enviarActualizarMateria(int fila, const QString& nombreOriginal,
                                 const QString& nombre, const QString& tipoAula);
    void enviarEliminarMateria(int fila, const QString& nombre);

    struct SolicitudPendiente {
        QString op;
        QJsonObject data;
        int fila = -1;
    } m_pendiente;

    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;
    InternalClient* m_client = nullptr;
};