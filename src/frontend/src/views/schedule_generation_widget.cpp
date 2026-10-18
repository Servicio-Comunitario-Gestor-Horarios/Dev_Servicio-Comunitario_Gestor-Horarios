#include "views/schedule_generation_widget.hpp"

#include <QLabel>
#include <QVBoxLayout>

#if defined(GESTOR_TIENE_BACKEND)

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <middleware/internalclient.h>

#include "backend/services/CargadorConfiguracionSolver.hpp"
#include "backend/services/NucleoDatos.hpp"
#include "backend/services/ServicioAula.hpp"
#include "backend/services/ServicioCursos.hpp"
#include "backend/services/ServicioGeneracion.hpp"
#include "backend/services/ServicioMaterias.hpp"
#include "backend/services/ServicioPlanesEstudio.hpp"
#include "backend/services/ServicioProfesor.hpp"
#include "backend/services/ServicioTurnosRecesos.hpp"
#include "backend/solver/config/solver_config.hpp"

#include "dialogs/schedule_theme.hpp"
#include "services/entrada_solver_builder.hpp"
#include "services/puerto_solver_ipc.hpp"

namespace {

/// Crea una tarjeta con título y devuelve el layout vertical de su contenido.
QFrame* crearTarjeta(const QString& titulo, QVBoxLayout*& contenido, QWidget* padre)
{
    auto* tarjeta = new QFrame(padre);
    tarjeta->setObjectName(QStringLiteral("genCard"));
    auto* lay = new QVBoxLayout(tarjeta);
    lay->setContentsMargins(20, 18, 20, 20);
    lay->setSpacing(14);

    auto* encabezado = new QLabel(titulo, tarjeta);
    encabezado->setObjectName(QStringLiteral("genCardTitulo"));
    lay->addWidget(encabezado);

    contenido = new QVBoxLayout();
    contenido->setSpacing(12);
    lay->addLayout(contenido);
    return tarjeta;
}

QSpinBox* crearSpin(QWidget* padre, int minimo, int maximo, int valor, const QString& sufijo = {})
{
    auto* spin = new QSpinBox(padre);
    spin->setRange(minimo, maximo);
    spin->setValue(valor);
    if (!sufijo.isEmpty())
        spin->setSuffix(sufijo);
    return spin;
}

QComboBox* crearCombo(QWidget* padre, const QStringList& opciones, const QString& actual = {})
{
    auto* combo = new QComboBox(padre);
    combo->addItem(QString());
    combo->addItems(opciones);
    if (!actual.isEmpty()) {
        const int idx = combo->findText(actual);
        combo->setCurrentIndex(idx < 0 ? 0 : idx);
    }
    return combo;
}

QLabel* crearAyuda(const QString& texto, QWidget* padre)
{
    auto* ayuda = new QLabel(texto, padre);
    ayuda->setObjectName(QStringLiteral("genAyuda"));
    ayuda->setWordWrap(true);
    return ayuda;
}

} // namespace

ScheduleGenerationWidget::ScheduleGenerationWidget(QWidget* parent)
    : QWidget(parent)
{
    construirUi();
    aplicarEstilo();
}

ScheduleGenerationWidget::~ScheduleGenerationWidget() = default;

void ScheduleGenerationWidget::setClient(InternalClient* client)
{
    m_client = client;
    if (!m_servicio) {
        m_puerto = new PuertoSolverIpc(client, this);
        m_servicio = new ServicioGeneracion(*m_puerto, 60000, {}, {}, this);
        connect(m_servicio, &ServicioGeneracion::finalizada,
                this, &ScheduleGenerationWidget::onGeneracionFinalizada);
    }
}

void ScheduleGenerationWidget::setNucleoDatos(NucleoDatos* nucleo)
{
    m_nucleo = nucleo;
    recargarDesdeNucleo();
}

void ScheduleGenerationWidget::setSchedulesDirectory(const QString& directorio)
{
    m_schedulesDir = directorio;
}

void ScheduleGenerationWidget::actualizarTema(bool /*modoOscuro*/)
{
    // El palet lo lee `ScheduleTheme::current()` del estado global de la app,
    // que la ventana ya actualizó antes de llamar aquí.
    aplicarEstilo();
}

void ScheduleGenerationWidget::recargarDesdeNucleo()
{
    poblarPlanes();
    poblarCursos();
    poblarDocentes();

    // En la primera carga, alinear los slots por día y la duración del bloque con
    // los turnos de la base: nº de slots/día = suma de los slots de los turnos, y
    // duración del bloque = (franja del turno − recesos) / nº de slots. Los slots
    // de cada turno ya vienen definidos por la base; no se configuran aparte.
    if (!m_defaultsApplied && m_nucleo) {
        if (const auto turnos = m_nucleo->listarTurnos(); turnos.ok && !turnos.valor.isEmpty()) {
            int total = 0;
            for (const TurnoDTO& turno : turnos.valor)
                total += qMax(0, turno.numSlots);
            if (total > 0 && m_spinSlotsDia)
                m_spinSlotsDia->setValue(qBound(1, total, 24));

            if (m_spinDuracion) {
                for (const TurnoDTO& turno : turnos.valor) {
                    if (turno.numSlots <= 0 || !turno.inicio.isValid() || !turno.fin.isValid())
                        continue;
                    int recesoMin = 0;
                    for (const RecesoDTO& receso : turno.recesos)
                        recesoMin += qMax(0, receso.duracion);
                    const int spanMin = turno.inicio.secsTo(turno.fin) / 60;
                    const int duracion = (spanMin - recesoMin) / turno.numSlots;
                    if (duracion > 0)
                        m_spinDuracion->setValue(qBound(m_spinDuracion->minimum(), duracion,
                                                        m_spinDuracion->maximum()));
                    break;
                }
            }
        }
        m_defaultsApplied = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Construcción de la interfaz
// ─────────────────────────────────────────────────────────────────────────

void ScheduleGenerationWidget::construirUi()
{
    setObjectName(QStringLiteral("genRoot"));
    // Un QWidget simple no pinta su `background` de QSS sin este atributo.
    setAttribute(Qt::WA_StyledBackground, true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(34, 28, 34, 22);
    root->setSpacing(16);

    auto* titulo = new QLabel(QStringLiteral("Generación de Horario"), this);
    titulo->setObjectName(QStringLiteral("genTitulo"));
    root->addWidget(titulo);

    auto* subtitulo = new QLabel(
        QStringLiteral("Configura los parámetros del motor y genera los horarios. "
                       "Los datos de docentes, aulas, materias y cursos se toman "
                       "de la base de datos."),
        this);
    subtitulo->setObjectName(QStringLiteral("genSubtitulo"));
    subtitulo->setWordWrap(true);
    root->addWidget(subtitulo);

    // ── Área desplazable con las tarjetas ─────────────────────────────────
    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("genScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));

    auto* contenedor = new QWidget(scroll);
    contenedor->setObjectName(QStringLiteral("genContenedor"));
    contenedor->setAttribute(Qt::WA_StyledBackground, true);
    auto* columna = new QVBoxLayout(contenedor);
    columna->setContentsMargins(2, 2, 12, 2);
    columna->setSpacing(16);

    // ── Tarjeta: parámetros generales ─────────────────────────────────────
    {
        QVBoxLayout* contenido = nullptr;
        QFrame* tarjeta = crearTarjeta(QStringLiteral("Parámetros generales"), contenido, contenedor);

        auto* form = new QFormLayout();
        form->setHorizontalSpacing(18);
        form->setVerticalSpacing(10);
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

        m_spinDuracion = crearSpin(tarjeta, 5, 180, 45, QStringLiteral(" min"));
        m_spinDias = crearSpin(tarjeta, 1, 7, 5);
        m_spinSlotsDia = crearSpin(tarjeta, 1, 24, 12);
        m_checkAulasFijas = new QCheckBox(
            QStringLiteral("Usar el aula fija asignada a cada curso"), tarjeta);
        m_checkAulasFijas->setChecked(true);

        form->addRow(QStringLiteral("Duración del bloque"), m_spinDuracion);
        form->addRow(QStringLiteral("Días a planificar"), m_spinDias);
        form->addRow(QStringLiteral("Slots por día"), m_spinSlotsDia);
        form->addRow(QString(), m_checkAulasFijas);

        contenido->addLayout(form);
        contenido->addWidget(crearAyuda(
            QStringLiteral("Los slots de cada turno se toman de los turnos definidos en "
                           "la base de datos; «slots por día» es su suma."),
            tarjeta));
        columna->addWidget(tarjeta);
    }

    // ── Tarjeta: cursos ───────────────────────────────────────────────────
    {
        QVBoxLayout* contenido = nullptr;
        QFrame* tarjeta = crearTarjeta(QStringLiteral("Cursos"), contenido, contenedor);

        contenido->addWidget(crearAyuda(
            QStringLiteral("Elige el turno, el plan de estudio y el número de estudiantes de "
                           "cada curso. El solver exige turno y plan; los datos de prueba "
                           "sembrados dejan el turno sin asignar."),
            tarjeta));

        m_tablaCursos = new QTableWidget(tarjeta);
        m_tablaCursos->setColumnCount(4);
        m_tablaCursos->setHorizontalHeaderLabels(
            {QStringLiteral("Curso"), QStringLiteral("Turno"),
             QStringLiteral("Plan de estudio"), QStringLiteral("Nº estudiantes")});
        m_tablaCursos->verticalHeader()->setVisible(false);
        m_tablaCursos->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_tablaCursos->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_tablaCursos->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        m_tablaCursos->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        m_tablaCursos->setSelectionMode(QAbstractItemView::NoSelection);
        m_tablaCursos->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tablaCursos->setMinimumHeight(180);
        contenido->addWidget(m_tablaCursos);

        auto* botones = new QHBoxLayout();
        auto* btnRecargar = new QPushButton(QStringLiteral("Recargar desde la base"), tarjeta);
        btnRecargar->setCursor(Qt::PointingHandCursor);
        connect(btnRecargar, &QPushButton::clicked, this, &ScheduleGenerationWidget::recargarDesdeNucleo);
        botones->addWidget(btnRecargar);
        botones->addStretch(1);
        contenido->addLayout(botones);

        columna->addWidget(tarjeta);
    }

    // ── Tarjeta: docentes ─────────────────────────────────────────────────
    {
        QVBoxLayout* contenido = nullptr;
        QFrame* tarjeta = crearTarjeta(QStringLiteral("Docentes"), contenido, contenedor);

        contenido->addWidget(crearAyuda(
            QStringLiteral("Ajusta las horas semanales y, si quieres, el turno/plan/años "
                           "preferidos y materias suplentes (separadas por «;»)."),
            tarjeta));

        m_tablaDocentes = new QTableWidget(tarjeta);
        m_tablaDocentes->setColumnCount(8);
        m_tablaDocentes->setHorizontalHeaderLabels(
            {QStringLiteral("Docente"), QStringLiteral("Horas requeridas"),
             QStringLiteral("Horas en aula"), QStringLiteral("Turno"),
             QStringLiteral("Plan"), QStringLiteral("Años"),
             QStringLiteral("Materias asignadas"), QStringLiteral("Materias suplentes")});
        m_tablaDocentes->verticalHeader()->setVisible(false);
        m_tablaDocentes->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_tablaDocentes->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
        m_tablaDocentes->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
        m_tablaDocentes->setSelectionMode(QAbstractItemView::NoSelection);
        m_tablaDocentes->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tablaDocentes->setMinimumHeight(200);
        contenido->addWidget(m_tablaDocentes);

        auto* botones = new QHBoxLayout();
        m_btnSugerirHoras = new QPushButton(
            QStringLiteral("Recalcular horas sugeridas por docente"), tarjeta);
        m_btnSugerirHoras->setCursor(Qt::PointingHandCursor);
        connect(m_btnSugerirHoras, &QPushButton::clicked, this, [this]() {
            if (!m_nucleo)
                return;
            const GeneracionSolver::OpcionesUI opciones = leerOpciones();
            const QMap<QString, int> sugeridas =
                GeneracionSolver::horasSugeridasPorDocente(*m_nucleo, opciones.cursos);
            for (int fila = 0; fila < m_docentesNombres.size(); ++fila) {
                const int valor = sugeridas.value(m_docentesNombres[fila], 0);
                if (valor <= 0)
                    continue;
                if (auto* req = qobject_cast<QSpinBox*>(m_tablaDocentes->cellWidget(fila, 1)))
                    req->setValue(valor);
                if (auto* aula = qobject_cast<QSpinBox*>(m_tablaDocentes->cellWidget(fila, 2)))
                    aula->setValue(valor);
            }
            establecerEstado(QStringLiteral("Horas sugeridas recalculadas."));
        });
        botones->addWidget(m_btnSugerirHoras);
        botones->addStretch(1);
        contenido->addLayout(botones);

        columna->addWidget(tarjeta);
    }

    // ── Tarjeta: presets ──────────────────────────────────────────────────
    {
        QVBoxLayout* contenido = nullptr;
        QFrame* tarjeta = crearTarjeta(QStringLiteral("Presets"), contenido, contenedor);

        contenido->addWidget(crearAyuda(
            QStringLiteral("Los presets usan el mismo formato que la configuración del "
                           "motor, de modo que se pueden reutilizar o inspeccionar."),
            tarjeta));

        auto* botones = new QHBoxLayout();
        m_btnCargarPreset = new QPushButton(QStringLiteral("Cargar preset…"), tarjeta);
        m_btnGuardarPreset = new QPushButton(QStringLiteral("Guardar preset…"), tarjeta);
        for (auto* b : {m_btnCargarPreset, m_btnGuardarPreset})
            b->setCursor(Qt::PointingHandCursor);
        connect(m_btnCargarPreset, &QPushButton::clicked, this, &ScheduleGenerationWidget::onCargarPreset);
        connect(m_btnGuardarPreset, &QPushButton::clicked, this, &ScheduleGenerationWidget::onGuardarPreset);
        botones->addWidget(m_btnCargarPreset);
        botones->addWidget(m_btnGuardarPreset);
        botones->addStretch(1);
        contenido->addLayout(botones);

        columna->addWidget(tarjeta);
    }

    columna->addStretch(1);
    scroll->setWidget(contenedor);
    root->addWidget(scroll, 1);

    // ── Barra inferior: estado + acción ───────────────────────────────────
    auto* barra = new QHBoxLayout();
    barra->setSpacing(14);

    m_lblEstado = new QLabel(QStringLiteral("Listo para generar."), this);
    m_lblEstado->setObjectName(QStringLiteral("genEstado"));

    m_progreso = new QProgressBar(this);
    m_progreso->setRange(0, 0); // indeterminado
    m_progreso->setFixedWidth(160);
    m_progreso->setVisible(false);

    m_btnGenerar = new QPushButton(QStringLiteral("Generar horarios"), this);
    m_btnGenerar->setObjectName(QStringLiteral("genBotonGenerar"));
    m_btnGenerar->setCursor(Qt::PointingHandCursor);
    m_btnGenerar->setMinimumHeight(40);
    m_btnGenerar->setMinimumWidth(190);
    connect(m_btnGenerar, &QPushButton::clicked, this, &ScheduleGenerationWidget::onGenerarClicked);

    barra->addWidget(m_lblEstado);
    barra->addStretch(1);
    barra->addWidget(m_progreso);
    barra->addWidget(m_btnGenerar);
    root->addLayout(barra);
}

void ScheduleGenerationWidget::aplicarEstilo()
{
    const ScheduleTheme::Palette p = ScheduleTheme::current();

    QString qss = QStringLiteral(R"QSS(
QWidget#genRoot, QWidget#genContenedor { background: %BG%; }
QLabel#genTitulo { font-size: 26px; font-weight: 800; color: %TEXT%; }
QLabel#genSubtitulo { font-size: 13px; color: %MUTED%; }
QLabel#genAyuda { font-size: 12px; color: %MUTED%; }
QLabel#genCardTitulo { font-size: 15px; font-weight: 700; color: %TEXT%; }
QLabel { color: %TEXT%; }
QFrame#genCard { background: %SURFACE%; border: 1px solid %BORDER%; border-radius: 14px; }
QScrollArea#genScroll { background: transparent; border: none; }
QTableWidget { background: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%;
  border-radius: 10px; gridline-color: %BORDER_STRONG%; }
QTableWidget::item { padding: 4px 6px; }
QHeaderView::section { background: %SURFACE2%; color: %MUTED%; border: none;
  border-bottom: 1px solid %BORDER_STRONG%; padding: 6px 8px; font-weight: 600; }
QComboBox, QSpinBox, QLineEdit { background: %SURFACE2%; color: %TEXT%;
  border: 1px solid %BORDER%; border-radius: 8px; padding: 3px 8px; min-height: 24px; }
QComboBox QAbstractItemView { background: %SURFACE%; color: %TEXT%;
  selection-background-color: %ACCENT%; }
QCheckBox { color: %TEXT%; }
QPushButton { background: %SURFACE2%; color: %TEXT%; border: 1px solid %BORDER%;
  border-radius: 8px; padding: 6px 14px; }
QPushButton:hover { border-color: %ACCENT%; }
QPushButton#genBotonGenerar { background: %ACCENT%; color: #FFFFFF; border: none;
  border-radius: 10px; font-size: 14px; font-weight: 700; padding: 8px 22px; }
QPushButton#genBotonGenerar:hover { background: %ACCENT_STRONG%; }
QPushButton#genBotonGenerar:disabled { background: %SURFACE3%; color: %SUBTLE%; }
QProgressBar { background: %SURFACE2%; border: 1px solid %BORDER%; border-radius: 6px;
  text-align: center; color: %TEXT%; }
QProgressBar::chunk { background: %ACCENT%; border-radius: 5px; }
QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar::handle:vertical { background: %BORDER_STRONG%; border-radius: 5px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)QSS");

    qss.replace(QStringLiteral("%BG%"), p.bg);
    qss.replace(QStringLiteral("%SURFACE3%"), p.surface3);
    qss.replace(QStringLiteral("%SURFACE2%"), p.surface2);
    qss.replace(QStringLiteral("%SURFACE%"), p.surface);
    qss.replace(QStringLiteral("%BORDER_STRONG%"), p.borderStrong);
    qss.replace(QStringLiteral("%BORDER%"), p.border);
    qss.replace(QStringLiteral("%ACCENT_STRONG%"), p.accentStrong);
    qss.replace(QStringLiteral("%ACCENT%"), p.accent);
    qss.replace(QStringLiteral("%TEXT%"), p.text);
    qss.replace(QStringLiteral("%MUTED%"), p.textMuted);
    qss.replace(QStringLiteral("%SUBTLE%"), p.textSubtle);

    // El estado se colorea aparte para poder marcar errores.
    if (m_estadoError)
        qss += QStringLiteral("QLabel#genEstado { color: %1; font-size: 12px; }").arg(p.danger);
    else
        qss += QStringLiteral("QLabel#genEstado { color: %1; font-size: 12px; }").arg(p.textMuted);

    setStyleSheet(qss);
}

// ─────────────────────────────────────────────────────────────────────────
// Poblado de datos
// ─────────────────────────────────────────────────────────────────────────

void ScheduleGenerationWidget::poblarPlanes()
{
    m_planesCodigos.clear();
    if (!m_nucleo)
        return;
    if (const auto r = m_nucleo->listarPlanes(); r.ok) {
        for (const PlanDTO& plan : r.valor)
            m_planesCodigos << plan.codigo;
    }
}

void ScheduleGenerationWidget::poblarCursos()
{
    m_cursosNombres.clear();
    m_turnosNombres.clear();
    if (!m_tablaCursos)
        return;
    m_tablaCursos->setRowCount(0);
    if (!m_nucleo)
        return;

    if (const auto t = m_nucleo->listarTurnos(); t.ok) {
        for (const TurnoDTO& turno : t.valor)
            m_turnosNombres << turno.nombre;
    }

    const auto r = m_nucleo->listarCursos();
    if (!r.ok)
        return;

    const QVector<CursoDTO>& cursos = r.valor;
    m_tablaCursos->setRowCount(cursos.size());
    for (int fila = 0; fila < cursos.size(); ++fila) {
        const CursoDTO& curso = cursos[fila];
        const QString nombre = nombreCurso(curso.anio, curso.seccion);
        m_cursosNombres << nombre;

        auto* itemNombre = new QTableWidgetItem(nombre);
        itemNombre->setFlags(itemNombre->flags() & ~Qt::ItemIsEditable);
        m_tablaCursos->setItem(fila, 0, itemNombre);

        // Turno: los cursos del centro se siembran sin turno; el solver exige que
        // cada curso declare uno (V7), así que se elige aquí (lo aporta el preset).
        auto* comboTurno = new QComboBox(m_tablaCursos);
        comboTurno->addItem(QStringLiteral("— Sin turno —"));
        comboTurno->addItems(m_turnosNombres);
        if (!curso.turno.isEmpty()) {
            const int idx = comboTurno->findText(curso.turno);
            if (idx >= 0)
                comboTurno->setCurrentIndex(idx);
        }
        m_tablaCursos->setCellWidget(fila, 1, comboTurno);

        m_tablaCursos->setCellWidget(fila, 2, crearCombo(m_tablaCursos, m_planesCodigos));

        auto* spin = crearSpin(m_tablaCursos, 1, 500, 30);
        m_tablaCursos->setCellWidget(fila, 3, spin);
    }
}

void ScheduleGenerationWidget::poblarDocentes()
{
    m_docentesNombres.clear();
    if (!m_tablaDocentes)
        return;
    m_tablaDocentes->setRowCount(0);
    if (!m_nucleo)
        return;

    const auto r = m_nucleo->listarDocentes();
    if (!r.ok)
        return;

    QStringList turnos;
    if (const auto t = m_nucleo->listarTurnos(); t.ok) {
        for (const TurnoDTO& turno : t.valor)
            turnos << turno.nombre;
    }

    const QVector<ProfesorDTO>& docentes = r.valor;
    m_tablaDocentes->setRowCount(docentes.size());
    for (int fila = 0; fila < docentes.size(); ++fila) {
        const ProfesorDTO& docente = docentes[fila];
        m_docentesNombres << docente.nombre;

        auto* itemNombre = new QTableWidgetItem(docente.nombre);
        itemNombre->setFlags(itemNombre->flags() & ~Qt::ItemIsEditable);
        m_tablaDocentes->setItem(fila, 0, itemNombre);

        const int porDefecto = qMax(1, docente.materias.size() * 2);
        m_tablaDocentes->setCellWidget(fila, 1, crearSpin(m_tablaDocentes, 1, 80, porDefecto));
        m_tablaDocentes->setCellWidget(fila, 2, crearSpin(m_tablaDocentes, 1, 80, porDefecto));

        m_tablaDocentes->setCellWidget(fila, 3, crearCombo(m_tablaDocentes, turnos));
        m_tablaDocentes->setCellWidget(fila, 4, crearCombo(m_tablaDocentes, m_planesCodigos));

        auto* editAnios = new QLineEdit(m_tablaDocentes);
        editAnios->setPlaceholderText(QStringLiteral("Todos (p. ej. 1; 2)"));
        m_tablaDocentes->setCellWidget(fila, 5, editAnios);

        QString materias;
        for (const MateriaAsignadaDTO& materia : docente.materias) {
            if (!materias.isEmpty())
                materias += QStringLiteral(", ");
            materias += materia.nombre;
        }
        auto* itemMaterias = new QTableWidgetItem(materias);
        itemMaterias->setFlags(itemMaterias->flags() & ~Qt::ItemIsEditable);
        m_tablaDocentes->setItem(fila, 6, itemMaterias);

        auto* editSuplentes = new QLineEdit(m_tablaDocentes);
        editSuplentes->setPlaceholderText(QStringLiteral("Opcional, separadas por «;»"));
        m_tablaDocentes->setCellWidget(fila, 7, editSuplentes);
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Lectura / aplicación de opciones
// ─────────────────────────────────────────────────────────────────────────

GeneracionSolver::OpcionesUI ScheduleGenerationWidget::leerOpciones() const
{
    GeneracionSolver::OpcionesUI opciones;
    opciones.duracionMinutos = m_spinDuracion->value();
    opciones.numDias = m_spinDias->value();
    opciones.numSlotsDia = m_spinSlotsDia->value();
    opciones.usarAulasFijas = m_checkAulasFijas->isChecked();

    for (int fila = 0; fila < m_cursosNombres.size(); ++fila) {
        GeneracionSolver::ParametrosCurso pc;
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaCursos->cellWidget(fila, 1)))
            if (combo->currentIndex() > 0)
                pc.turno = combo->currentText().trimmed();
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaCursos->cellWidget(fila, 2)))
            pc.plan = combo->currentText().trimmed();
        if (auto* spin = qobject_cast<QSpinBox*>(m_tablaCursos->cellWidget(fila, 3)))
            pc.numEstudiantes = spin->value();
        opciones.cursos.insert(m_cursosNombres[fila], pc);
    }

    for (int fila = 0; fila < m_docentesNombres.size(); ++fila) {
        GeneracionSolver::ParametrosDocente pd;
        if (auto* spin = qobject_cast<QSpinBox*>(m_tablaDocentes->cellWidget(fila, 1)))
            pd.horasRequeridas = spin->value();
        if (auto* spin = qobject_cast<QSpinBox*>(m_tablaDocentes->cellWidget(fila, 2)))
            pd.horasAula = spin->value();
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaDocentes->cellWidget(fila, 3)))
            pd.turno = combo->currentText().trimmed();
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaDocentes->cellWidget(fila, 4)))
            pd.plan = combo->currentText().trimmed();
        if (auto* edit = qobject_cast<QLineEdit*>(m_tablaDocentes->cellWidget(fila, 5))) {
            const QStringList partes = edit->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
            for (const QString& parte : partes) {
                bool ok = false;
                const int anio = parte.trimmed().toInt(&ok);
                if (ok && !pd.anios.contains(anio))
                    pd.anios.append(anio);
            }
        }
        if (auto* edit = qobject_cast<QLineEdit*>(m_tablaDocentes->cellWidget(fila, 7))) {
            const QStringList partes = edit->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
            for (const QString& parte : partes) {
                const QString limpio = parte.trimmed();
                if (!limpio.isEmpty())
                    pd.materiasSuplentes << limpio;
            }
        }
        opciones.docentes.insert(m_docentesNombres[fila], pd);
    }

    return opciones;
}

void ScheduleGenerationWidget::aplicarOpciones(const GeneracionSolver::OpcionesUI& opciones)
{
    m_spinDuracion->setValue(opciones.duracionMinutos);
    m_spinDias->setValue(opciones.numDias);
    if (opciones.numSlotsDia > 0)
        m_spinSlotsDia->setValue(opciones.numSlotsDia);
    m_checkAulasFijas->setChecked(opciones.usarAulasFijas);

    for (int fila = 0; fila < m_cursosNombres.size(); ++fila) {
        const GeneracionSolver::ParametrosCurso pc =
            opciones.cursos.value(m_cursosNombres[fila]);
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaCursos->cellWidget(fila, 1))) {
            const int idx = combo->findText(pc.turno);
            combo->setCurrentIndex(idx < 0 ? 0 : idx);
        }
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaCursos->cellWidget(fila, 2))) {
            const int idx = combo->findText(pc.plan);
            combo->setCurrentIndex(idx < 0 ? 0 : idx);
        }
        if (auto* spin = qobject_cast<QSpinBox*>(m_tablaCursos->cellWidget(fila, 3)))
            spin->setValue(pc.numEstudiantes > 0 ? pc.numEstudiantes : 30);
    }

    for (int fila = 0; fila < m_docentesNombres.size(); ++fila) {
        const GeneracionSolver::ParametrosDocente pd =
            opciones.docentes.value(m_docentesNombres[fila]);
        if (auto* spin = qobject_cast<QSpinBox*>(m_tablaDocentes->cellWidget(fila, 1)))
            if (pd.horasRequeridas > 0) spin->setValue(pd.horasRequeridas);
        if (auto* spin = qobject_cast<QSpinBox*>(m_tablaDocentes->cellWidget(fila, 2)))
            if (pd.horasAula > 0) spin->setValue(pd.horasAula);
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaDocentes->cellWidget(fila, 3))) {
            const int idx = combo->findText(pd.turno);
            if (idx >= 0) combo->setCurrentIndex(idx);
        }
        if (auto* combo = qobject_cast<QComboBox*>(m_tablaDocentes->cellWidget(fila, 4))) {
            const int idx = combo->findText(pd.plan);
            if (idx >= 0) combo->setCurrentIndex(idx);
        }
        if (auto* edit = qobject_cast<QLineEdit*>(m_tablaDocentes->cellWidget(fila, 5))) {
            QStringList anios;
            for (int anio : pd.anios)
                anios << QString::number(anio);
            edit->setText(anios.join(QStringLiteral("; ")));
        }
        if (auto* edit = qobject_cast<QLineEdit*>(m_tablaDocentes->cellWidget(fila, 7)))
            edit->setText(pd.materiasSuplentes.join(QStringLiteral("; ")));
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Generación
// ─────────────────────────────────────────────────────────────────────────

void ScheduleGenerationWidget::establecerEstado(const QString& texto, bool error)
{
    if (!m_lblEstado)
        return;
    m_estadoError = error;
    m_lblEstado->setText(texto);
    aplicarEstilo();
}

bool ScheduleGenerationWidget::validarConfiguracion(const GeneracionSolver::OpcionesUI& opciones,
                                                    QString& error) const
{
    if (!m_nucleo) {
        error = QStringLiteral("No hay conexión con la base de datos.");
        return false;
    }
    if (opciones.cursos.isEmpty()) {
        error = QStringLiteral("No hay cursos para planificar.");
        return false;
    }

    for (auto it = opciones.cursos.constBegin(); it != opciones.cursos.constEnd(); ++it) {
        if (it.value().turno.isEmpty()) {
            error = QStringLiteral("El curso «%1» no tiene un turno asignado.").arg(it.key());
            return false;
        }
        if (it.value().plan.isEmpty()) {
            error = QStringLiteral("El curso «%1» no tiene un plan de estudio asignado.")
                        .arg(it.key());
            return false;
        }
        if (it.value().numEstudiantes <= 0) {
            error = QStringLiteral("El curso «%1» debe tener más de 0 estudiantes.")
                        .arg(it.key());
            return false;
        }
    }
    return true;
}

void ScheduleGenerationWidget::onGenerarClicked()
{
    if (!m_servicio) {
        establecerEstado(QStringLiteral("El generador no está disponible."), true);
        return;
    }
    if (m_servicio->enCurso())
        return;

    const GeneracionSolver::OpcionesUI opciones = leerOpciones();

    QString error;
    if (!validarConfiguracion(opciones, error)) {
        establecerEstado(error, true);
        ScheduleTheme::showStyledMessage(this, QStringLiteral("Configuración incompleta"), error, true);
        return;
    }

    const GeneracionSolver::Composicion composicion =
        GeneracionSolver::construirEntrada(*m_nucleo, opciones);

    if (!composicion.avisos.isEmpty()) {
        establecerEstado(QStringLiteral("Revisa la configuración antes de generar."), true);
        ScheduleTheme::showStyledMessage(
            this, QStringLiteral("Revisa la configuración"),
            composicion.avisos.join(QStringLiteral("\n")), true);
        return;
    }

    m_contexto = composicion.contexto;
    m_entradaActual = composicion.entrada;

    m_btnGenerar->setEnabled(false);
    m_progreso->setVisible(true);
    establecerEstado(QStringLiteral("Generando horarios… puede tardar unos segundos."));
    m_servicio->generar(m_entradaActual, QString());
}

void ScheduleGenerationWidget::onGeneracionFinalizada()
{
    if (m_progreso)
        m_progreso->setVisible(false);
    if (m_btnGenerar)
        m_btnGenerar->setEnabled(true);

    if (!m_servicio)
        return;

    const ResultadoGeneracion& resultado = m_servicio->ultimoResultado();
    if (!resultado.presentable) {
        QString mensaje = resultado.mensaje;
        if (mensaje.isEmpty())
            mensaje = QStringLiteral("No se pudo generar el horario.");
        establecerEstado(mensaje, true);
        ScheduleTheme::showStyledMessage(this, QStringLiteral("No se pudo generar el horario"),
                                         mensaje, true);
        return;
    }

    const QJsonObject datos =
        GeneracionSolver::aFormatoVisualizacion(resultado.horario, m_contexto);
    const QString ruta = guardarHorario(datos);

    if (ruta.isEmpty()) {
        establecerEstado(QStringLiteral("Horario generado, pero no se pudo guardar el archivo."),
                         true);
    } else {
        establecerEstado(QStringLiteral("Horario generado y guardado."));
    }

    QString mensaje = QStringLiteral(
        "El horario se generó correctamente y ya está disponible en la sección "
        "«Visualización de Horarios», donde puedes revisarlo y editarlo.");
    if (!ruta.isEmpty())
        mensaje += QStringLiteral("\n\nArchivo: %1").arg(QDir::toNativeSeparators(ruta));

    ScheduleTheme::showStyledMessage(this, QStringLiteral("Horario generado"), mensaje, false);
    emit horarioGenerado(datos, ruta);
}

QString ScheduleGenerationWidget::guardarHorario(const QJsonObject& datos) const
{
    if (m_schedulesDir.isEmpty())
        return QString();

    QDir dir(m_schedulesDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return QString();

    const QString nombre = QStringLiteral("horario_%1.json")
                               .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));
    const QString ruta = dir.filePath(nombre);

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QString();
    archivo.write(QJsonDocument(datos).toJson(QJsonDocument::Indented));
    archivo.close();
    return ruta;
}

void ScheduleGenerationWidget::onCargarPreset()
{
    const QString ruta = QFileDialog::getOpenFileName(
        this, QStringLiteral("Cargar preset"), QString(), QStringLiteral("Preset (*.json)"));
    if (ruta.isEmpty())
        return;

    const auto resultado = CargadorConfiguracionSolver::cargar(ruta);
    if (!resultado.ok) {
        ScheduleTheme::showStyledMessage(this, QStringLiteral("No se pudo cargar el preset"),
                                         resultado.mensajeError, true);
        return;
    }

    const SolverConfig& config = resultado.valor;
    GeneracionSolver::OpcionesUI opciones;
    opciones.version = config.version;
    opciones.duracionMinutos = config.franja_horaria.duracion_minutos;
    opciones.numDias = config.dimensiones.num_dias;
    opciones.numSlotsDia = config.dimensiones.num_slots_dia;
    opciones.usarAulasFijas = config.aula_fija;

    for (const CursoSolverConfig& curso : config.cursos) {
        GeneracionSolver::ParametrosCurso pc;
        pc.plan = curso.plan;
        pc.turno = curso.turno;
        pc.numEstudiantes = curso.num_estudiantes;
        opciones.cursos.insert(nombreCurso(curso.anio, curso.seccion), pc);
    }

    // El preset referencia las materias suplentes por índice al pool global; se
    // traducen a nombre con el mismo orden que usa la entrada (materias del núcleo).
    QStringList nombresMaterias;
    if (m_nucleo) {
        if (const auto materias = m_nucleo->listarMaterias(); materias.ok)
            for (const MateriaDTO& m : materias.valor)
                nombresMaterias << m.nombre;
    }

    for (const ProfesorSolverConfig& profesor : config.profesores) {
        GeneracionSolver::ParametrosDocente pd;
        pd.horasRequeridas = profesor.horas_requeridas;
        pd.horasAula = profesor.horas_aula;
        pd.turno = profesor.turno;
        pd.plan = profesor.plan;
        pd.anios = profesor.anios;
        for (const MateriaSuplente& suplente : profesor.materias_suplente) {
            const QString nombre = nombresMaterias.value(suplente.materiaIDx);
            if (!nombre.isEmpty())
                pd.materiasSuplentes << nombre;
        }
        opciones.docentes.insert(profesor.nombre, pd);
    }

    aplicarOpciones(opciones);
    establecerEstado(QStringLiteral("Preset cargado."));
}

void ScheduleGenerationWidget::onGuardarPreset()
{
    if (!m_nucleo)
        return;

    const GeneracionSolver::OpcionesUI opciones = leerOpciones();
    const GeneracionSolver::Composicion composicion =
        GeneracionSolver::construirEntrada(*m_nucleo, opciones);

    const auto config = SolverConfig::fromJson(composicion.entrada);
    if (!config.ok) {
        ScheduleTheme::showStyledMessage(
            this, QStringLiteral("No se pudo preparar el preset"),
            QStringLiteral("La configuración actual no es válida:\n\n%1").arg(config.mensajeError),
            true);
        return;
    }

    const QString ruta = QFileDialog::getSaveFileName(
        this, QStringLiteral("Guardar preset"), QStringLiteral("preset.json"),
        QStringLiteral("Preset (*.json)"));
    if (ruta.isEmpty())
        return;

    const auto guardado = CargadorConfiguracionSolver::guardar(ruta, config.valor);
    if (!guardado.ok) {
        ScheduleTheme::showStyledMessage(this, QStringLiteral("No se pudo guardar el preset"),
                                         guardado.mensajeError, true);
        return;
    }
    establecerEstado(QStringLiteral("Preset guardado."));
}

#else // !GESTOR_TIENE_BACKEND

ScheduleGenerationWidget::ScheduleGenerationWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    auto* etiqueta = new QLabel(
        QStringLiteral("La generación de horarios no está disponible en esta "
                       "configuración (backend desactivado)."),
        this);
    etiqueta->setAlignment(Qt::AlignCenter);
    etiqueta->setWordWrap(true);
    layout->addWidget(etiqueta);
}

ScheduleGenerationWidget::~ScheduleGenerationWidget() = default;
void ScheduleGenerationWidget::setNucleoDatos(NucleoDatos*) {}
void ScheduleGenerationWidget::setClient(InternalClient*) {}
void ScheduleGenerationWidget::setSchedulesDirectory(const QString&) {}
void ScheduleGenerationWidget::actualizarTema(bool) {}
void ScheduleGenerationWidget::recargarDesdeNucleo() {}
void ScheduleGenerationWidget::onGenerarClicked() {}
void ScheduleGenerationWidget::onGeneracionFinalizada() {}
void ScheduleGenerationWidget::onCargarPreset() {}
void ScheduleGenerationWidget::onGuardarPreset() {}

#endif // GESTOR_TIENE_BACKEND
