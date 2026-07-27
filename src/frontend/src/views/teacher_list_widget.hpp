#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QJsonObject>

class InternalClient;

class TeacherListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TeacherListWidget(QWidget *parent = nullptr);
    void setClient(InternalClient* client);

private slots:
    void onRespuestaRecibida(const QJsonObject& respuesta);
    void abrirFormularioNuevo();

private:
    void setupUi();
    void insertarProfesorEnTabla(const QString& cedula, const QString& id,
                                 const QString& nombre, const QString& email,
                                 const QString& telefono);

    void enviarCrearProfesor(const QString& cedula, const QString& id,
                             const QString& nombre, const QString& email,
                             const QString& telefono);
    void enviarActualizarProfesor(int fila, const QString& cedula,
                                  const QString& id, const QString& nombre,
                                  const QString& email, const QString& telefono);
    void enviarEliminarProfesor(int fila, const QString& cedula);

    struct SolicitudPendiente {
        QString op;
        QJsonObject data;
        int fila = -1;
    } m_pendiente;

    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;
    InternalClient* m_client = nullptr;
};
