#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QJsonObject>

class InternalClient;

class ClassroomListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClassroomListWidget(QWidget *parent = nullptr);
    void setClient(InternalClient* client);

private slots:
    void onRespuestaRecibida(const QJsonObject& respuesta);
    void abrirFormularioNuevo();

private:
    void setupUi();
    void insertarAulaEnTabla(const QString& nombre, int capacidad,
                             const QString& edificio, const QString& piso);

    void enviarCrearAula(const QString& nombre, int capacidad,
                         const QString& edificio, const QString& piso);
    void enviarActualizarAula(int fila, const QString& nombreOriginal,
                              const QString& nombre, int capacidad,
                              const QString& edificio, const QString& piso);
    void enviarEliminarAula(int fila, const QString& nombre);

    struct SolicitudPendiente {
        QString op;
        QJsonObject data;
        int fila = -1;
    } m_pendiente;

    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;
    InternalClient* m_client = nullptr;
};