#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QJsonObject>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

class InternalClient;
class NucleoDatos;

class SubjectListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SubjectListWidget(QWidget *parent = nullptr);
    ~SubjectListWidget() = default;

    void setClient(InternalClient* client);
    void setNucleoDatos(NucleoDatos* nucleo);

    void actualizarTema(bool modoOscuro);

public slots:
    void cargarMaterias();

private slots:
    void onRespuestaRecibida(const QJsonObject& respuesta);
    void abrirFormularioNuevo();

private:
    void setupUi();
    void actualizarPanelLateral(); // <-- NUEVO: Para refrescar ocupación y notificaciones dinámicamente
    void insertarMateriaEnTabla(int id, const QString& nombre, const QString& requisitos);

    void enviarCrearMateria(const QString& nombre, const QString& requisitos);
    void enviarActualizarMateria(int id, const QString& nombre, const QString& requisitos);
    void enviarEliminarMateria(int id);

    struct SolicitudPendiente {
        QString op;
        QJsonObject data;
        int fila = -1;
    } m_pendiente;

    QTableWidget* m_table = nullptr;
    QPushButton* m_registerButton = nullptr;
    InternalClient* m_client = nullptr;
    NucleoDatos* m_nucleo = nullptr;

    // Punteros para los elementos dinámicos del panel derecho
    QLabel* m_occValLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QVBoxLayout* m_notifLayout = nullptr;
};