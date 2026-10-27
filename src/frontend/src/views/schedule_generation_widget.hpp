#pragma once

/**
 * @file schedule_generation_widget.hpp
 * @brief Vista de «Generación de horario»: configuración inicial, disparo de la
 *        generación y persistencia del resultado (RF-1).
 *
 * Presenta todos los parámetros que el motor necesita —franja horaria,
 * dimensiones, plan/turno/nº de estudiantes por curso, y horas/turno/plan/años
 * y materias suplentes por docente— más la opción global de aulas fijas. Las
 * materias y sus horas por curso se derivan del plan de estudio elegido; los
 * slots de cada turno ya vienen definidos por los turnos de la base, así que no
 * se configuran aquí. Al generar, compone la entrada del solver, la envía por
 * IPC y, si hay solución, guarda el horario en la carpeta de horarios y avisa
 * por `horarioGenerado`.
 *
 * Cuerpo condicionado a `GESTOR_TIENE_BACKEND`: sin backend la vista muestra un
 * mensaje informativo.
 */

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "services/generacion_tipos.hpp"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableWidget;

class NucleoDatos;
class InternalClient;

class ServicioGeneracion;
class PuertoSolverIpc;

class ScheduleGenerationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScheduleGenerationWidget(QWidget* parent = nullptr);
    ~ScheduleGenerationWidget() override;

    /// Inyecta el núcleo de datos del cliente (no se posee).
    void setNucleoDatos(NucleoDatos* nucleo);
    /// Inyecta el cliente IPC (no se posee).
    void setClient(InternalClient* client);
    /// Directorio donde se guardan los horarios generados.
    void setSchedulesDirectory(const QString& directorio);

    /// Vuelve a leer cursos/docentes/planes del núcleo y repuebla las tablas.
    void recargarDesdeNucleo();

    /// Reaplica los colores del tema (claro/oscuro) en caliente.
    void actualizarTema(bool modoOscuro);

signals:
    /// Se generó y guardó un horario. `datosVisualizacion` es el JSON en el
    /// formato de la sección de visualización; `rutaArchivo` la ubicación.
    void horarioGenerado(const QJsonObject& datosVisualizacion, const QString& rutaArchivo);

private slots:
    void onGenerarClicked();
    void onGeneracionFinalizada();
    void onCargarPreset();
    void onGuardarPreset();

private:
    void construirUi();
    void aplicarEstilo();
    void poblarPlanes();
    void poblarCursos();
    void poblarDocentes();
    /// Reúne los años de los cursos y las materias existentes (para los selectores).
    void poblarCatalogos();

    GeneracionSolver::OpcionesUI leerOpciones() const;
    void aplicarOpciones(const GeneracionSolver::OpcionesUI& opciones);

    bool validarConfiguracion(const GeneracionSolver::OpcionesUI& opciones,
                              QString& error) const;

    QString guardarHorario(const QJsonObject& datos) const;
    void establecerEstado(const QString& texto, bool error = false);

    NucleoDatos* m_nucleo = nullptr;
    InternalClient* m_client = nullptr;
    QString m_schedulesDir;

    // ── Controles ─────────────────────────────────────────────────────────
    QSpinBox* m_spinDuracion = nullptr;
    QSpinBox* m_spinDias = nullptr;
    QSpinBox* m_spinSlotsDia = nullptr;
    QCheckBox* m_checkAulasFijas = nullptr;
    QTableWidget* m_tablaCursos = nullptr;
    QTableWidget* m_tablaDocentes = nullptr;
    QPushButton* m_btnGenerar = nullptr;
    QPushButton* m_btnCargarPreset = nullptr;
    QPushButton* m_btnGuardarPreset = nullptr;
    QPushButton* m_btnSugerirHoras = nullptr;
    QLabel* m_lblEstado = nullptr;
    QProgressBar* m_progreso = nullptr;

    // ── Estado de generación ──────────────────────────────────────────────
    ServicioGeneracion* m_servicio = nullptr;
    PuertoSolverIpc* m_puerto = nullptr;
    GeneracionSolver::ContextoHorario m_contexto;
    QJsonObject m_entradaActual;

    // ── Datos auxiliares (por fila) ───────────────────────────────────────
    QStringList m_planesCodigos;
    QStringList m_turnosNombres;
    QStringList m_cursosNombres;
    QStringList m_docentesNombres;
    QVector<int> m_aniosDisponibles;   ///< Años de los cursos de la base.
    QStringList m_materiasDisponibles; ///< Materias existentes (para suplencias).
    bool m_defaultsApplied = false; ///< Los valores por defecto se derivan una vez.
    bool m_estadoError = false;     ///< El último estado mostrado es un error.
};
