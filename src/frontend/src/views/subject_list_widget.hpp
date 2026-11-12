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
class QTabWidget;
struct PlanDTO;

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
    /// Refresca asignaturas y planes (la llama MainWindow al abrir la base).
    void cargarMaterias();
    /// Refresca solo la tabla de planes de estudio.
    void cargarPlanes();

private slots:
    void onRespuestaRecibida(const QJsonObject& respuesta);
    void abrirFormularioNuevo();
    void abrirFormularioPlanNuevo();
    void onTabChanged(int index);

private:
    void setupUi();
    void actualizarPanelLateral();
    void insertarMateriaEnTabla(int id, const QString& nombre, const QString& requisitos,
                                const QString& tipoAula, int idTipoAula);
    void insertarPlanEnTabla(const PlanDTO& plan, int numMaterias, int horasTotales);
    void editarPlan(const PlanDTO& plan);
    void duplicarPlan(const PlanDTO& plan);
    void eliminarPlan(const PlanDTO& plan);
    QString sugerirCodigoPlan() const;
    void refrescarBotonRegistro();

    void enviarCrearMateria(const QString& nombre, const QString& requisitos);
    void enviarActualizarMateria(int id, const QString& nombre, const QString& requisitos);
    void enviarEliminarMateria(int id);

    struct SolicitudPendiente {
        QString op;
        QJsonObject data;
        int fila = -1;
    } m_pendiente;

    QTabWidget* m_tabs = nullptr;
    QTableWidget* m_table = nullptr;
    QTableWidget* m_planTable = nullptr;
    QLabel* m_titulo = nullptr;
    QLabel* m_subtitulo = nullptr;
    QPushButton* m_registerButton = nullptr;
    InternalClient* m_client = nullptr;
    NucleoDatos* m_nucleo = nullptr;

    // Punteros para los elementos dinámicos del panel derecho
    QLabel* m_occValLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QVBoxLayout* m_notifLayout = nullptr;
};
