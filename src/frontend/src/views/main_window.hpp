#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QFrame>

class TeacherListWidget;
class ClassroomListWidget;
class SubjectListWidget;
class ScheduleVisualizationWidget;
class ScheduleGenerationWidget;
class InternalClient;
class NucleoDatos;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    /// Inyecta el núcleo de datos (dominios) del lado cliente. El arranque lo
    /// construye y lo entrega aquí; la ventana no lo posee. Puede ser `nullptr`
    /// si el backend de persistencia no está disponible en esta configuración.
    void setNucleoDatos(NucleoDatos* nucleo);

private slots:
    void mostrarInicio();
    void mostrarDocentes();
    void mostrarAulas();
    void mostrarAsignaturas();
    void mostrarGeneracion();
    void mostrarVisualizacion();
    void abrirConfiguracion();
    void aplicarTemaGlobal(bool modoOscuro); // <--- Declaración añadida

protected:
    // Reajusta la disposicion cuando cambia el estado de la ventana
    // (maximizar/restaurar). El area cliente definitiva no siempre esta
    // aplicada al recibir el evento, asi que el trabajo se difiere.
    void changeEvent(QEvent* event) override;

private:
    void setupSidebar();
    void setupCentralArea();
    void configurarEstilosMenu();
    void activarBoton(QPushButton* btn); // <--- Declaración añadida que soluciona tu error

    void reajustarDisposicion();
    void programarReajuste();

    QFrame *m_sidebar = nullptr;
    QStackedWidget *m_contenedorVistas = nullptr;
    QWidget *m_rightContainer = nullptr;

    QPushButton *m_btnInicio = nullptr;
    QPushButton *m_btnDocentes = nullptr;
    QPushButton *m_btnAulas = nullptr;
    QPushButton *m_btnAsignaturas = nullptr;
    QPushButton *m_btnGeneracion = nullptr;
    QPushButton *m_btnVisualizacion = nullptr;

    TeacherListWidget* m_teacherListWidget = nullptr;
    ClassroomListWidget* m_classroomListWidget = nullptr;
    SubjectListWidget* m_subjectListWidget = nullptr;
    ScheduleVisualizationWidget* m_scheduleVisualizationWidget = nullptr;
    ScheduleGenerationWidget* m_generationWidget = nullptr;

    /// Cliente IPC compartido por las vistas (no se posee; padre Qt).
    InternalClient* m_client = nullptr;

    /// Núcleo de datos del cliente, inyectado por el arranque (no lo posee).
    NucleoDatos* m_nucleo = nullptr;

    // Evita acumular varios reajustes diferidos del mismo cambio de estado.
    bool m_reajustePendiente = false;
};