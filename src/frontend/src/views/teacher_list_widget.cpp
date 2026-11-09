/**
 * @file teacher_list_widget.cpp
 * @brief Docentes: CRUD, disponibilidad, preferencias y materias titulares/suplentes.
 *
 * Implementa las issues #94 y #95 con persistencia real:
 *  - Rejilla de disponibilidad/preferencias construida con los turnos, los
 *    eventos (recesos) y la duración de la hora académica de la base.
 *  - Guardado atómico de disponibilidad, preferencias y materias.
 */

#include "teacher_list_widget.hpp"

#include "../forms/teacher_form_dialog.hpp"
#include "backend/services/NucleoDatos.hpp"
#include "services/preferencias_docente_ui.hpp"
#include "widgets/selector_materias.hpp"

#include <QApplication>
#include <QFrame>
#include <QHash>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kNumDias = 5;

/// Clave estable de una celda dentro de los conjuntos de estado.
int claveCelda(int fila, int columna)
{
    return fila * 100 + columna;
}

QStringList diasLaborables()
{
    return {QStringLiteral("Lunes"), QStringLiteral("Martes"), QStringLiteral("Miércoles"),
            QStringLiteral("Jueves"), QStringLiteral("Viernes")};
}

QStringList nombresDe(const QVector<MateriaAsignadaDTO>& materias)
{
    QStringList nombres;
    for (const MateriaAsignadaDTO& materia : materias)
        nombres << materia.nombre;
    nombres.sort(Qt::CaseInsensitive);
    return nombres;
}

QStringList turnosDelNucleo(NucleoDatos* nucleo)
{
    QStringList nombres;
    if (const auto turnos = nucleo->listarTurnos(); turnos.ok) {
        for (const TurnoDTO& turno : turnos.valor)
            nombres << turno.nombre;
    }
    return nombres;
}

QStringList planesDelNucleo(NucleoDatos* nucleo)
{
    QStringList codigos;
    if (const auto planes = nucleo->listarPlanes(); planes.ok) {
        for (const PlanDTO& plan : planes.valor)
            codigos << plan.codigo;
    }
    return codigos;
}

QVector<int> aniosDelNucleo(NucleoDatos* nucleo)
{
    QVector<int> anios;
    if (const auto cursos = nucleo->listarCursos(); cursos.ok) {
        for (const CursoDTO& curso : cursos.valor) {
            if (curso.anio > 0 && !anios.contains(curso.anio))
                anios.append(curso.anio);
        }
    }
    std::sort(anios.begin(), anios.end());
    return anios;
}

QMap<QString, QString> docentesExistentes(NucleoDatos* nucleo)
{
    QMap<QString, QString> mapa;
    if (const auto docentes = nucleo->listarDocentes(); docentes.ok) {
        for (const ProfesorDTO& docente : docentes.valor)
            mapa.insert(docente.id, docente.nombre);
    }
    return mapa;
}

QColor colorReceso(bool oscuro)
{
    return oscuro ? QColor("#374151") : QColor("#E5E7EB");
}

QColor colorDisponible()
{
    return QColor("#A7F3D0");
}

QColor colorNoDisponible()
{
    return QColor("#FECACA");
}

QColor colorPreferida()
{
    return QColor("#FDE68A");
}

QColor colorEvitada()
{
    return QColor("#FED7AA");
}

QColor colorVacia(bool oscuro)
{
    return oscuro ? QColor("#1e1e1e") : QColor("#FFFFFF");
}

/// ¿La franja persistida cubre por completo el bloque indicado?
bool franjaCubre(const FranjaHoraria& franja, int dia, const QTime& inicio, const QTime& fin)
{
    return franja.dia == dia && franja.inicio <= inicio && franja.fin >= fin;
}

} // namespace

TeacherListWidget::TeacherListWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void TeacherListWidget::setClient(InternalClient* client)
{
    m_client = client;
}

void TeacherListWidget::setNucleoDatos(NucleoDatos* nucleo)
{
    m_nucleo = nucleo;
    m_cedulaActual.clear();
    m_filaActual = -1;
    construirRejillas();
    limpiarPanelDocente();
    cargarDocentes();
}

void TeacherListWidget::setupUi()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // ── Columna izquierda: listado y CRUD ─────────────────────────────────
    auto* leftWidget = new QWidget(this);
    leftWidget->setObjectName(QStringLiteral("TeacherLeftWidget"));
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* topSectionLayout = new QHBoxLayout();
    auto* textLayout = new QVBoxLayout();

    auto* titulo = new QLabel(QStringLiteral("Gestión de Docentes"), leftWidget);
    titulo->setObjectName(QStringLiteral("TeacherTitle"));

    auto* subtitulo = new QLabel(
        QStringLiteral("Administra la información, disponibilidad y materias del personal académico"),
        leftWidget);
    subtitulo->setObjectName(QStringLiteral("TeacherSub"));

    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton(QStringLiteral("+ Registrar Docente"), leftWidget);
    m_registerButton->setObjectName(QStringLiteral("TeacherRegBtn"));
    m_registerButton->setCursor(Qt::PointingHandCursor);

    topSectionLayout->addLayout(textLayout);
    topSectionLayout->addStretch();
    topSectionLayout->addWidget(m_registerButton, 0, Qt::AlignVCenter);

    m_table = new QTableWidget(0, 6, leftWidget);
    m_table->setObjectName(QStringLiteral("TeacherTable"));
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Cédula"), QStringLiteral("Nombre"), QStringLiteral("Email"),
         QStringLiteral("Teléfono"), QStringLiteral("Materias"), QStringLiteral("Acciones")});

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setDefaultSectionSize(45);

    leftLayout->addLayout(topSectionLayout);
    leftLayout->addSpacing(20);
    leftLayout->addWidget(m_table);

    // ── Columna derecha: pestañas de la ficha ─────────────────────────────
    auto* rightPanel = new QFrame(this);
    rightPanel->setObjectName(QStringLiteral("TeacherRightPanel"));
    rightPanel->setMinimumWidth(520);
    rightPanel->setMaximumWidth(640);

    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(14);

    m_detalleNombre = new QLabel(QStringLiteral("Detalles del Docente"), rightPanel);
    m_detalleNombre->setObjectName(QStringLiteral("DetailTitle"));
    m_detalleNombre->setAlignment(Qt::AlignHCenter);
    rightLayout->addWidget(m_detalleNombre);

    m_tabs = new QTabWidget(rightPanel);
    m_tabs->setObjectName(QStringLiteral("TeacherTabs"));

    // Pestaña: disponibilidad
    auto* pageDisp = new QWidget(m_tabs);
    auto* layDisp = new QVBoxLayout(pageDisp);
    layDisp->setContentsMargins(6, 10, 6, 4);
    layDisp->setSpacing(8);

    auto* ayudaDisp = new QLabel(
        QStringLiteral("Haz clic en las horas en las que el docente no puede dar clase. "
                       "Verde = disponible · Rojo = no disponible."),
        pageDisp);
    ayudaDisp->setObjectName(QStringLiteral("TeacherHelp"));
    ayudaDisp->setWordWrap(true);
    layDisp->addWidget(ayudaDisp);

    auto* accionesDisp = new QHBoxLayout();
    auto* btnTodoDisponible = new QPushButton(QStringLiteral("Marcar todo disponible"), pageDisp);
    btnTodoDisponible->setObjectName(QStringLiteral("TeacherManageButton"));
    btnTodoDisponible->setCursor(Qt::PointingHandCursor);
    accionesDisp->addWidget(btnTodoDisponible);
    accionesDisp->addStretch();
    layDisp->addLayout(accionesDisp);

    m_gridDisponibilidad = new QTableWidget(0, kNumDias, pageDisp);
    m_gridDisponibilidad->setObjectName(QStringLiteral("AvailabilityGrid"));
    m_gridDisponibilidad->setHorizontalHeaderLabels(diasLaborables());
    m_gridDisponibilidad->verticalHeader()->setMinimumWidth(110);
    m_gridDisponibilidad->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_gridDisponibilidad->verticalHeader()->setDefaultSectionSize(28);
    m_gridDisponibilidad->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_gridDisponibilidad->horizontalHeader()->setMinimumHeight(30);
    // Retícula visible: sin ella las celdas parecen un panel único hasta que se
    // pinta una. `gridline-color` se ajusta por tema en `actualizarTema`.
    m_gridDisponibilidad->setShowGrid(true);
    m_gridDisponibilidad->setGridStyle(Qt::SolidLine);
    m_gridDisponibilidad->setSelectionMode(QAbstractItemView::NoSelection);
    m_gridDisponibilidad->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layDisp->addWidget(m_gridDisponibilidad, 1);

    auto* leyendaDisp = new QHBoxLayout();
    auto anadirLeyenda = [](QHBoxLayout* fila, const QString& color, const QString& texto,
                            QWidget* padre) {
        auto* swatch = new QLabel(padre);
        swatch->setFixedSize(16, 10);
        swatch->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 2px;")
                                  .arg(color));
        auto* label = new QLabel(texto, padre);
        label->setObjectName(QStringLiteral("TeacherLegendText"));
        fila->addWidget(swatch);
        fila->addWidget(label);
    };
    leyendaDisp->addStretch();
    anadirLeyenda(leyendaDisp, colorDisponible().name(), QStringLiteral("Disponible"), pageDisp);
    leyendaDisp->addSpacing(14);
    anadirLeyenda(leyendaDisp, colorNoDisponible().name(), QStringLiteral("No disponible"), pageDisp);
    leyendaDisp->addStretch();
    layDisp->addLayout(leyendaDisp);

    // Pestaña: horas preferidas
    auto* pagePref = new QWidget(m_tabs);
    auto* layPref = new QVBoxLayout(pagePref);
    layPref->setContentsMargins(6, 10, 6, 4);
    layPref->setSpacing(8);

    auto* ayudaPref = new QLabel(
        QStringLiteral("Marca las horas que el docente prefiere. Las horas no disponibles "
                       "aparecen bloqueadas y no se pueden marcar."),
        pagePref);
    ayudaPref->setObjectName(QStringLiteral("TeacherHelp"));
    ayudaPref->setWordWrap(true);
    layPref->addWidget(ayudaPref);

    m_gridPreferencias = new QTableWidget(0, kNumDias, pagePref);
    m_gridPreferencias->setObjectName(QStringLiteral("PreferencesGrid"));
    m_gridPreferencias->setHorizontalHeaderLabels(diasLaborables());
    m_gridPreferencias->verticalHeader()->setMinimumWidth(110);
    m_gridPreferencias->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_gridPreferencias->verticalHeader()->setDefaultSectionSize(28);
    m_gridPreferencias->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_gridPreferencias->horizontalHeader()->setMinimumHeight(30);
    m_gridPreferencias->setShowGrid(true);
    m_gridPreferencias->setGridStyle(Qt::SolidLine);
    m_gridPreferencias->setSelectionMode(QAbstractItemView::NoSelection);
    m_gridPreferencias->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layPref->addWidget(m_gridPreferencias, 1);

    auto* leyendaPref = new QHBoxLayout();
    leyendaPref->addStretch();
    anadirLeyenda(leyendaPref, colorPreferida().name(), QStringLiteral("Preferida"), pagePref);
    leyendaPref->addSpacing(14);
    anadirLeyenda(leyendaPref, colorEvitada().name(), QStringLiteral("Evitada"), pagePref);
    leyendaPref->addSpacing(14);
    anadirLeyenda(leyendaPref, colorReceso(false).name(), QStringLiteral("No disponible"), pagePref);
    leyendaPref->addStretch();
    layPref->addLayout(leyendaPref);

    // Pestaña: materias titulares/suplentes
    auto* pageAsig = new QWidget(m_tabs);
    auto* layAsig = new QVBoxLayout(pageAsig);
    layAsig->setContentsMargins(6, 10, 6, 4);
    layAsig->setSpacing(8);

    auto* ayudaAsig = new QLabel(
        QStringLiteral("Gestiona las materias que el docente dicta y aquellas en las que "
                       "puede actuar como suplente. Una materia no puede estar en las dos listas."),
        pageAsig);
    ayudaAsig->setObjectName(QStringLiteral("TeacherHelp"));
    ayudaAsig->setWordWrap(true);
    layAsig->addWidget(ayudaAsig);

    auto* lblTit = new QLabel(QStringLiteral("Materias que dicta (titular):"), pageAsig);
    lblTit->setObjectName(QStringLiteral("TeacherSectionTitle"));
    layAsig->addWidget(lblTit);

    m_listTitulares = new QListWidget(pageAsig);
    m_listTitulares->setObjectName(QStringLiteral("TeacherTitulares"));
    m_listTitulares->setSelectionMode(QAbstractItemView::NoSelection);
    layAsig->addWidget(m_listTitulares, 1);

    m_btnGestionarTitulares = new QPushButton(QStringLiteral("Gestionar titulares"), pageAsig);
    m_btnGestionarTitulares->setObjectName(QStringLiteral("TeacherManageButton"));
    m_btnGestionarTitulares->setCursor(Qt::PointingHandCursor);
    layAsig->addWidget(m_btnGestionarTitulares);

    m_avisoMaterias = new QLabel(
        QStringLiteral("Sin materias asignadas: este docente no podrá usarse en la generación."),
        pageAsig);
    m_avisoMaterias->setObjectName(QStringLiteral("TeacherWarning"));
    m_avisoMaterias->setWordWrap(true);
    m_avisoMaterias->setVisible(false);
    layAsig->addWidget(m_avisoMaterias);

    auto* lblSup = new QLabel(QStringLiteral("Materias en las que puede ser suplente:"), pageAsig);
    lblSup->setObjectName(QStringLiteral("TeacherSectionTitle"));
    layAsig->addWidget(lblSup);

    m_listSuplentes = new QListWidget(pageAsig);
    m_listSuplentes->setObjectName(QStringLiteral("TeacherSuplentes"));
    m_listSuplentes->setSelectionMode(QAbstractItemView::NoSelection);
    layAsig->addWidget(m_listSuplentes, 1);

    m_btnGestionarSuplentes = new QPushButton(QStringLiteral("Gestionar suplentes"), pageAsig);
    m_btnGestionarSuplentes->setObjectName(QStringLiteral("TeacherManageButton"));
    m_btnGestionarSuplentes->setCursor(Qt::PointingHandCursor);
    layAsig->addWidget(m_btnGestionarSuplentes);

    m_tabs->addTab(pageDisp, QStringLiteral("Disponibilidad"));
    m_tabs->addTab(pagePref, QStringLiteral("Horas preferidas"));
    m_tabs->addTab(pageAsig, QStringLiteral("Asignaturas"));
    rightLayout->addWidget(m_tabs, 1);

    // Barra de guardado
    m_panelGuardado = new QWidget(rightPanel);
    auto* layGuardado = new QHBoxLayout(m_panelGuardado);
    layGuardado->setContentsMargins(0, 0, 0, 0);
    layGuardado->setSpacing(10);

    m_estadoGuardado = new QLabel(QStringLiteral("Selecciona un docente"), m_panelGuardado);
    m_estadoGuardado->setObjectName(QStringLiteral("TeacherStatus"));

    m_btnDescartar = new QPushButton(QStringLiteral("Descartar cambios"), m_panelGuardado);
    m_btnDescartar->setObjectName(QStringLiteral("TeacherDiscardButton"));
    m_btnDescartar->setCursor(Qt::PointingHandCursor);

    m_btnGuardar = new QPushButton(QStringLiteral("Guardar"), m_panelGuardado);
    m_btnGuardar->setObjectName(QStringLiteral("TeacherSaveButton"));
    m_btnGuardar->setCursor(Qt::PointingHandCursor);
    m_btnGuardar->setDefault(true);

    layGuardado->addWidget(m_estadoGuardado, 1);
    layGuardado->addWidget(m_btnDescartar);
    layGuardado->addWidget(m_btnGuardar);
    rightLayout->addWidget(m_panelGuardado);

    mainLayout->addWidget(leftWidget, 7);
    mainLayout->addWidget(rightPanel, 5);

    // ── Conexiones ────────────────────────────────────────────────────────
    connect(m_registerButton, &QPushButton::clicked,
            this, &TeacherListWidget::abrirFormularioNuevo);
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int) {
        m_table->selectRow(row);
        onDocenteSeleccionado();
    });
    connect(m_gridDisponibilidad, &QTableWidget::cellClicked,
            this, &TeacherListWidget::onDisponibilidadCellClicked);
    connect(m_gridPreferencias, &QTableWidget::cellClicked,
            this, &TeacherListWidget::onPreferenciasCellClicked);
    connect(btnTodoDisponible, &QPushButton::clicked, this, [this]() {
        if (m_cedulaActual.isEmpty() || m_filas.isEmpty())
            return;
        m_noDisponible.clear();
        repintarRejillas();
        actualizarEstadoCambios();
    });
    connect(m_btnGestionarTitulares, &QPushButton::clicked,
            this, &TeacherListWidget::gestionarTitulares);
    connect(m_btnGestionarSuplentes, &QPushButton::clicked,
            this, &TeacherListWidget::gestionarSuplentes);
    connect(m_btnGuardar, &QPushButton::clicked, this, &TeacherListWidget::guardarCambios);
    connect(m_btnDescartar, &QPushButton::clicked, this, &TeacherListWidget::descartarCambios);

    actualizarEstadoCambios();
}

void TeacherListWidget::construirRejillas()
{
    m_filas.clear();
    m_gridConstruida = false;

    if (!m_gridDisponibilidad || !m_gridPreferencias)
        return;

    m_gridDisponibilidad->setRowCount(0);
    m_gridPreferencias->setRowCount(0);

    if (!m_nucleo)
        return;

    int duracion = 35;
    if (const auto config = m_nucleo->configuracionHoraria(); config.ok)
        duracion = config.valor.duracionMinutos;
    if (duracion <= 0)
        duracion = 35;

    QVector<TurnoDTO> turnos;
    if (const auto resultado = m_nucleo->listarTurnos(); resultado.ok)
        turnos = resultado.valor;

    std::sort(turnos.begin(), turnos.end(), [](const TurnoDTO& a, const TurnoDTO& b) {
        if (a.inicio == b.inicio)
            return a.nombre < b.nombre;
        return a.inicio < b.inicio;
    });

    QSet<QString> bloquesVistos;
    for (const TurnoDTO& turno : turnos) {
        if (!turno.inicio.isValid() || turno.numSlots <= 0)
            continue;

        QTime horaReal = turno.inicio;

        for (int slot = 0; slot < turno.numSlots; ++slot) {
            FilaRejilla fila;
            fila.receso = false;
            fila.turno = turno.nombre;
            fila.slot = slot;
            fila.inicioSlot = turno.inicio.addSecs(slot * duracion * 60);
            fila.finSlot = fila.inicioSlot.addSecs(duracion * 60);

            const QTime finReal = horaReal.addSecs(duracion * 60);
            fila.etiqueta = QStringLiteral("%1 - %2")
                                .arg(horaReal.toString(QStringLiteral("HH:mm")),
                                     finReal.toString(QStringLiteral("HH:mm")));

            const QString claveBloque =
                fila.inicioSlot.toString(QStringLiteral("HH:mm")) + QLatin1Char('|')
                + fila.finSlot.toString(QStringLiteral("HH:mm"));
            if (!bloquesVistos.contains(claveBloque)) {
                bloquesVistos.insert(claveBloque);
                m_filas.append(fila);
            }

            horaReal = finReal;

            for (const EventoDTO& evento : turno.eventos) {
                if (evento.despuesDeSlot != slot + 1 || evento.duracion <= 0)
                    continue;

                FilaRejilla receso;
                receso.receso = true;
                receso.turno = turno.nombre;
                receso.inicioSlot = QTime();
                receso.finSlot = QTime();
                const QTime finEvento = horaReal.addSecs(evento.duracion * 60);
                const QString nombreEvento = evento.nombre.isEmpty()
                                                 ? QStringLiteral("Evento")
                                                 : evento.nombre;
                receso.etiqueta = QStringLiteral("%1  %2 - %3")
                                      .arg(nombreEvento,
                                           horaReal.toString(QStringLiteral("HH:mm")),
                                           finEvento.toString(QStringLiteral("HH:mm")));
                m_filas.append(receso);
                horaReal = finEvento;
            }
        }
    }

    QStringList etiquetas;
    etiquetas.reserve(m_filas.size());
    for (const FilaRejilla& fila : m_filas)
        etiquetas << fila.etiqueta;

    for (QTableWidget* grid : {m_gridDisponibilidad, m_gridPreferencias}) {
        grid->setRowCount(m_filas.size());
        grid->setVerticalHeaderLabels(etiquetas);
        for (int r = 0; r < m_filas.size(); ++r) {
            for (int c = 0; c < kNumDias; ++c) {
                if (!grid->item(r, c))
                    grid->setItem(r, c, new QTableWidgetItem);
            }
        }
    }

    m_gridConstruida = true;
}

void TeacherListWidget::cargarDocentes()
{
    if (!m_nucleo || !m_table)
        return;

    const QString seleccion = m_cedulaActual;
    m_table->setRowCount(0);

    const auto res = m_nucleo->listarDocentes();
    if (!res.ok) {
        QMessageBox::warning(this, QStringLiteral("Error BD"),
                             QStringLiteral("No se pudo cargar la lista de docentes."));
        return;
    }

    for (const ProfesorDTO& profe : res.valor) {
        const int fila = m_table->rowCount();
        m_table->insertRow(fila);

        auto crearItemCentrado = [](const QString& texto) {
            auto* item = new QTableWidgetItem(texto);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        QStringList listaMaterias;
        for (const MateriaAsignadaDTO& mat : profe.materias)
            listaMaterias << mat.nombre;
        const QString txtMaterias = listaMaterias.isEmpty()
                                        ? QStringLiteral("-")
                                        : listaMaterias.join(QStringLiteral(", "));

        m_table->setItem(fila, 0, crearItemCentrado(profe.id));
        m_table->setItem(fila, 1, crearItemCentrado(profe.nombre));
        m_table->setItem(fila, 2, crearItemCentrado(profe.email));
        m_table->setItem(fila, 3, crearItemCentrado(profe.telefono.isEmpty()
                                                        ? QStringLiteral("N/A")
                                                        : profe.telefono));
        m_table->setItem(fila, 4, crearItemCentrado(txtMaterias));

        if (!profe.materiasSuplentes.isEmpty()) {
            QStringList suplentes;
            for (const MateriaAsignadaDTO& mat : profe.materiasSuplentes)
                suplentes << mat.nombre;
            m_table->item(fila, 4)->setToolTip(
                QStringLiteral("Suplente en: %1").arg(suplentes.join(QStringLiteral(", "))));
        }

        auto* panelAcciones = new QWidget();
        auto* layoutAcciones = new QHBoxLayout(panelAcciones);
        layoutAcciones->setContentsMargins(0, 0, 0, 0);
        layoutAcciones->setSpacing(10);

        auto* btnEditar = new QPushButton(QStringLiteral("✏️"));
        btnEditar->setObjectName(QStringLiteral("TeacherEditButton"));
        btnEditar->setCursor(Qt::PointingHandCursor);

        auto* btnEliminar = new QPushButton(QStringLiteral("🗑️"));
        btnEliminar->setObjectName(QStringLiteral("TeacherDeleteButton"));
        btnEliminar->setCursor(Qt::PointingHandCursor);

        layoutAcciones->addStretch();
        layoutAcciones->addWidget(btnEditar);
        layoutAcciones->addWidget(btnEliminar);
        layoutAcciones->addStretch();

        connect(btnEliminar, &QPushButton::clicked, this, [this, panelAcciones]() {
            for (int i = 0; i < m_table->rowCount(); ++i) {
                if (m_table->cellWidget(i, 5) == panelAcciones) {
                    eliminarDocente(i);
                    break;
                }
            }
        });
        connect(btnEditar, &QPushButton::clicked, this, [this, panelAcciones]() {
            for (int i = 0; i < m_table->rowCount(); ++i) {
                if (m_table->cellWidget(i, 5) == panelAcciones) {
                    abrirFormularioEditar(i);
                    break;
                }
            }
        });

        m_table->setCellWidget(fila, 5, panelAcciones);
    }

    if (!seleccion.isEmpty()) {
        seleccionarFilaPorCedula(seleccion);
    } else {
        limpiarPanelDocente();
    }

    // Los botones de cada fila se crean al poblar la tabla: aplicar el tema
    // vigente después evita que queden con el estilo por defecto.
    actualizarTema(m_isDark);
}

void TeacherListWidget::seleccionarFilaPorCedula(const QString& cedula)
{
    for (int fila = 0; fila < m_table->rowCount(); ++fila) {
        if (m_table->item(fila, 0) && m_table->item(fila, 0)->text() == cedula) {
            m_filaActual = fila;
            m_cedulaActual = cedula;
            m_table->selectRow(fila);
            cargarDocenteActual();
            return;
        }
    }
    limpiarPanelDocente();
}

void TeacherListWidget::limpiarPanelDocente()
{
    m_cedulaActual.clear();
    m_filaActual = -1;
    m_noDisponible.clear();
    m_noDisponibleOriginal.clear();
    m_preferidas.clear();
    m_preferidasOriginal.clear();
    m_evitadas.clear();
    m_evitadasOriginal.clear();
    m_titulares.clear();
    m_titularesOriginal.clear();
    m_suplentes.clear();
    m_suplentesOriginal.clear();

    if (m_detalleNombre)
        m_detalleNombre->setText(QStringLiteral("Detalles del Docente"));

    repintarRejillas();
    repintarAsignaturas();
    actualizarEstadoCambios();
}

void TeacherListWidget::cargarDocenteActual()
{
    if (!m_nucleo || m_cedulaActual.isEmpty()) {
        limpiarPanelDocente();
        return;
    }

    const auto resultado = m_nucleo->obtenerDocente(m_cedulaActual);
    if (!resultado.ok) {
        QMessageBox::warning(this, QStringLiteral("No se pudo abrir el docente"),
                             resultado.mensajeError);
        limpiarPanelDocente();
        return;
    }

    const ProfesorDTO& profe = resultado.valor;
    m_detalleNombre->setText(profe.nombre);

    m_noDisponible.clear();
    m_preferidas.clear();
    m_evitadas.clear();

    for (int r = 0; r < m_filas.size(); ++r) {
        if (m_filas[r].receso)
            continue;

        for (int c = 0; c < kNumDias; ++c) {
            const int dia = c + 1;
            bool disponible = false;
            for (const DisponibilidadDTO& disp : profe.disponibilidad) {
                if (franjaCubre(disp.franja, dia, m_filas[r].inicioSlot,
                                m_filas[r].finSlot)) {
                    disponible = true;
                    break;
                }
            }
            if (!disponible)
                m_noDisponible.insert(claveCelda(r, c));

            for (const PreferenciaDTO& pref : profe.preferencias) {
                if (!franjaCubre(pref.franja, dia, m_filas[r].inicioSlot,
                                 m_filas[r].finSlot))
                    continue;
                if (pref.sentido == SentidoPreferencia::Preferida)
                    m_preferidas.insert(claveCelda(r, c));
                else
                    m_evitadas.insert(claveCelda(r, c));
            }
        }
    }

    // Una hora no disponible no puede conservar una preferencia.
    for (auto it = m_preferidas.begin(); it != m_preferidas.end();) {
        if (m_noDisponible.contains(*it))
            it = m_preferidas.erase(it);
        else
            ++it;
    }
    for (auto it = m_evitadas.begin(); it != m_evitadas.end();) {
        if (m_noDisponible.contains(*it))
            it = m_evitadas.erase(it);
        else
            ++it;
    }

    m_titulares = nombresDe(profe.materias);
    m_suplentes = nombresDe(profe.materiasSuplentes);

    m_noDisponibleOriginal = m_noDisponible;
    m_preferidasOriginal = m_preferidas;
    m_evitadasOriginal = m_evitadas;
    m_titularesOriginal = m_titulares;
    m_suplentesOriginal = m_suplentes;

    repintarRejillas();
    repintarAsignaturas();
    actualizarEstadoCambios();
}

void TeacherListWidget::repintarRejillas()
{
    if (!m_gridDisponibilidad || !m_gridPreferencias)
        return;

    const QColor vacia = colorVacia(m_isDark);
    const QColor receso = colorReceso(m_isDark);

    for (int r = 0; r < m_filas.size(); ++r) {
        for (int c = 0; c < kNumDias; ++c) {
            auto* itemDisp = m_gridDisponibilidad->item(r, c);
            auto* itemPref = m_gridPreferencias->item(r, c);
            if (!itemDisp || !itemPref)
                continue;

            const bool esReceso = m_filas[r].receso;
            const int clave = claveCelda(r, c);
            const bool noDisponible = m_noDisponible.contains(clave);
            const bool preferida = m_preferidas.contains(clave);
            const bool evitada = m_evitadas.contains(clave);

            itemDisp->setTextAlignment(Qt::AlignCenter);
            itemPref->setTextAlignment(Qt::AlignCenter);

            if (esReceso) {
                itemDisp->setBackground(receso);
                itemDisp->setText(QString());
                itemDisp->setToolTip(QStringLiteral("Receso: no se puede pintar"));
                itemPref->setBackground(receso);
                itemPref->setText(QString());
                itemPref->setToolTip(QStringLiteral("Receso: no se puede pintar"));
                continue;
            }

            if (noDisponible) {
                itemDisp->setBackground(colorNoDisponible());
                itemDisp->setText(QStringLiteral("✖"));
                itemDisp->setForeground(QColor("#991B1B"));
                itemDisp->setToolTip(QStringLiteral("No disponible"));
            } else {
                itemDisp->setBackground(colorDisponible());
                itemDisp->setText(QString());
                itemDisp->setForeground(QColor("#065F46"));
                itemDisp->setToolTip(QStringLiteral("Disponible"));
            }

            if (noDisponible) {
                itemPref->setBackground(receso);
                itemPref->setText(QStringLiteral("Bloq."));
                itemPref->setForeground(QColor("#6B7280"));
                itemPref->setToolTip(QStringLiteral("No disponible: no se puede preferir"));
            } else if (preferida) {
                itemPref->setBackground(colorPreferida());
                itemPref->setText(QStringLiteral("★"));
                itemPref->setForeground(QColor("#92400E"));
                itemPref->setToolTip(QStringLiteral("Hora preferida"));
            } else if (evitada) {
                itemPref->setBackground(colorEvitada());
                itemPref->setText(QStringLiteral("Evitada"));
                itemPref->setForeground(QColor("#9A3412"));
                itemPref->setToolTip(QStringLiteral("Hora evitada (se conserva al guardar)"));
            } else {
                itemPref->setBackground(vacia);
                itemPref->setText(QString());
                itemPref->setForeground(m_isDark ? QColor("#FFFFFF") : QColor("#111827"));
                itemPref->setToolTip(QStringLiteral("Sin preferencia"));
            }
        }
    }
}

void TeacherListWidget::repintarAsignaturas()
{
    if (!m_listTitulares || !m_listSuplentes)
        return;

    m_listTitulares->clear();
    m_listTitulares->addItems(m_titulares);
    m_listSuplentes->clear();
    m_listSuplentes->addItems(m_suplentes);

    if (m_avisoMaterias)
        m_avisoMaterias->setVisible(m_titulares.isEmpty());

    if (m_filaActual >= 0 && m_filaActual < m_table->rowCount()) {
        auto* item = m_table->item(m_filaActual, 4);
        if (item) {
            item->setText(m_titulares.isEmpty() ? QStringLiteral("-")
                                                : m_titulares.join(QStringLiteral(", ")));
            item->setToolTip(m_suplentes.isEmpty()
                                 ? QString()
                                 : QStringLiteral("Suplente en: %1")
                                       .arg(m_suplentes.join(QStringLiteral(", "))));
        }
    }
}

bool TeacherListWidget::tieneCambios() const
{
    if (m_cedulaActual.isEmpty())
        return false;
    return m_noDisponible != m_noDisponibleOriginal
           || m_preferidas != m_preferidasOriginal
           || m_evitadas != m_evitadasOriginal
           || m_titulares != m_titularesOriginal
           || m_suplentes != m_suplentesOriginal;
}

void TeacherListWidget::actualizarTitulosPestanas()
{
    if (!m_tabs)
        return;

    const bool dispCambios = m_noDisponible != m_noDisponibleOriginal;
    const bool prefCambios = m_preferidas != m_preferidasOriginal
                             || m_evitadas != m_evitadasOriginal;
    const bool asigCambios = m_titulares != m_titularesOriginal
                             || m_suplentes != m_suplentesOriginal;

    m_tabs->setTabText(0, QStringLiteral("Disponibilidad") + (dispCambios ? QStringLiteral(" *") : QString()));
    m_tabs->setTabText(1, QStringLiteral("Horas preferidas") + (prefCambios ? QStringLiteral(" *") : QString()));
    m_tabs->setTabText(2, QStringLiteral("Asignaturas") + (asigCambios ? QStringLiteral(" *") : QString()));
}

void TeacherListWidget::actualizarEstadoCambios()
{
    const bool hayDocente = !m_cedulaActual.isEmpty();
    const bool cambios = tieneCambios();

    actualizarTitulosPestanas();

    if (m_btnGuardar)
        m_btnGuardar->setEnabled(hayDocente && cambios);
    if (m_btnDescartar)
        m_btnDescartar->setEnabled(hayDocente && cambios);
    if (m_btnGestionarTitulares)
        m_btnGestionarTitulares->setEnabled(hayDocente);
    if (m_btnGestionarSuplentes)
        m_btnGestionarSuplentes->setEnabled(hayDocente);

    if (m_estadoGuardado) {
        if (!hayDocente)
            m_estadoGuardado->setText(QStringLiteral("Selecciona un docente"));
        else if (cambios)
            m_estadoGuardado->setText(QStringLiteral("Cambios sin guardar"));
        else
            m_estadoGuardado->setText(QStringLiteral("Todo guardado"));
    }
}

bool TeacherListWidget::celdaDisponible(int fila, int columna) const
{
    if (fila < 0 || fila >= m_filas.size() || m_filas[fila].receso)
        return false;
    return !m_noDisponible.contains(claveCelda(fila, columna));
}

void TeacherListWidget::onDocenteSeleccionado()
{
    if (!m_nucleo || !m_table)
        return;

    const int fila = m_table->currentRow();
    if (fila < 0)
        return;

    auto* itemId = m_table->item(fila, 0);
    auto* itemNombre = m_table->item(fila, 1);
    if (!itemId || !itemNombre)
        return;

    const QString cedula = itemId->text();
    if (cedula == m_cedulaActual)
        return;

    if (!pedirConfirmacionCambios()) {
        if (m_filaActual >= 0 && m_filaActual < m_table->rowCount()) {
            m_table->setCurrentCell(m_filaActual, 0);
            m_table->selectRow(m_filaActual);
        }
        return;
    }

    m_filaActual = fila;
    m_cedulaActual = cedula;
    m_detalleNombre->setText(itemNombre->text());
    cargarDocenteActual();
}

bool TeacherListWidget::pedirConfirmacionCambios()
{
    if (!tieneCambios())
        return true;

    const auto respuesta = QMessageBox::question(
        this, QStringLiteral("Cambios sin guardar"),
        QStringLiteral("Tienes cambios sin guardar en este docente. ¿Deseas descartarlos?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (respuesta != QMessageBox::Yes)
        return false;

    descartarCambios();
    return true;
}

void TeacherListWidget::onDisponibilidadCellClicked(int fila, int columna)
{
    if (m_cedulaActual.isEmpty() || fila < 0 || fila >= m_filas.size()
        || m_filas[fila].receso)
        return;

    const int clave = claveCelda(fila, columna);
    if (m_noDisponible.contains(clave)) {
        m_noDisponible.remove(clave);
    } else {
        m_noDisponible.insert(clave);
        const bool eraPreferida = m_preferidas.remove(clave);
        const bool eraEvitada = m_evitadas.remove(clave);
        if (eraPreferida || eraEvitada) {
            QMessageBox::information(
                this, QStringLiteral("Preferencia retirada"),
                QStringLiteral("La hora se marcó como no disponible; su preferencia fue retirada."));
        }
    }

    repintarRejillas();
    actualizarEstadoCambios();
}

void TeacherListWidget::onPreferenciasCellClicked(int fila, int columna)
{
    if (m_cedulaActual.isEmpty() || fila < 0 || fila >= m_filas.size()
        || m_filas[fila].receso)
        return;

    if (!celdaDisponible(fila, columna)) {
        QMessageBox::warning(
            this, QStringLiteral("Hora bloqueada"),
            QStringLiteral("No puedes marcar como preferida una hora en la que el docente "
                           "no está disponible."));
        return;
    }

    const int clave = claveCelda(fila, columna);
    if (m_preferidas.contains(clave))
        m_preferidas.remove(clave);
    else {
        m_preferidas.insert(clave);
        m_evitadas.remove(clave);
    }

    repintarRejillas();
    actualizarEstadoCambios();
}

void TeacherListWidget::gestionarTitulares()
{
    if (!m_nucleo || m_cedulaActual.isEmpty())
        return;

    const auto materias = m_nucleo->listarMaterias();
    if (!materias.ok) {
        QMessageBox::warning(this, QStringLiteral("Error BD"), materias.mensajeError);
        return;
    }

    QStringList disponibles;
    for (const MateriaDTO& materia : materias.valor)
        disponibles << materia.nombre;
    disponibles.sort(Qt::CaseInsensitive);

    SelectorMateriasDialog dialogo(
        disponibles, m_titulares, this, QStringLiteral("Materias que dicta"),
        QStringLiteral("Marca las materias que este docente imparte como titular. "
                       "Al marcarlas aquí se quitan de la lista de suplentes."));
    if (dialogo.exec() != QDialog::Accepted)
        return;

    const QStringList nuevas = dialogo.seleccionadas();
    QStringList retiradas;
    for (const QString& nombre : nuevas) {
        if (m_suplentes.contains(nombre)) {
            m_suplentes.removeAll(nombre);
            retiradas << nombre;
        }
    }
    m_titulares = nuevas;

    if (!retiradas.isEmpty()) {
        QMessageBox::information(
            this, QStringLiteral("Materias movidas"),
            QStringLiteral("Se retiraron de la lista de suplentes: %1")
                .arg(retiradas.join(QStringLiteral(", "))));
    }

    repintarAsignaturas();
    actualizarEstadoCambios();
}

void TeacherListWidget::gestionarSuplentes()
{
    if (!m_nucleo || m_cedulaActual.isEmpty())
        return;

    const auto materias = m_nucleo->listarMaterias();
    if (!materias.ok) {
        QMessageBox::warning(this, QStringLiteral("Error BD"), materias.mensajeError);
        return;
    }

    QStringList disponibles;
    for (const MateriaDTO& materia : materias.valor)
        disponibles << materia.nombre;
    disponibles.sort(Qt::CaseInsensitive);

    SelectorMateriasDialog dialogo(
        disponibles, m_suplentes, this, QStringLiteral("Materias suplentes"),
        QStringLiteral("Marca las materias en las que este docente puede actuar como suplente. "
                       "Al marcarlas aquí se quitan de la lista de titulares."));
    if (dialogo.exec() != QDialog::Accepted)
        return;

    const QStringList nuevas = dialogo.seleccionadas();
    QStringList retiradas;
    for (const QString& nombre : nuevas) {
        if (m_titulares.contains(nombre)) {
            m_titulares.removeAll(nombre);
            retiradas << nombre;
        }
    }
    m_suplentes = nuevas;

    if (!retiradas.isEmpty()) {
        QMessageBox::information(
            this, QStringLiteral("Materias movidas"),
            QStringLiteral("Se retiraron de la lista de titulares: %1")
                .arg(retiradas.join(QStringLiteral(", "))));
    }

    repintarAsignaturas();
    actualizarEstadoCambios();
}

void TeacherListWidget::guardarCambios()
{
    if (!m_nucleo || m_cedulaActual.isEmpty())
        return;

    if (m_filas.isEmpty()) {
        QMessageBox::warning(
            this, QStringLiteral("Sin turnos configurados"),
            QStringLiteral("No hay turnos con franjas configuradas en la base. "
                           "Configura los turnos antes de guardar la disponibilidad."));
        return;
    }

    const auto materias = m_nucleo->listarMaterias();
    if (!materias.ok) {
        QMessageBox::warning(this, QStringLiteral("Error BD"), materias.mensajeError);
        return;
    }

    QHash<QString, int> ids;
    for (const MateriaDTO& materia : materias.valor)
        ids.insert(materia.nombre, materia.id);

    QVector<int> titulares;
    QVector<int> suplentes;
    for (const QString& nombre : m_titulares) {
        if (ids.contains(nombre))
            titulares.append(ids.value(nombre));
    }
    for (const QString& nombre : m_suplentes) {
        if (ids.contains(nombre))
            suplentes.append(ids.value(nombre));
    }

    // Disponibilidad: todos los bloques verdes, fusionando bloques contiguos por día.
    QVector<FranjaHoraria> disponibilidad;
    for (int columna = 0; columna < kNumDias; ++columna) {
        bool abierto = false;
        QTime inicio;
        QTime fin;

        for (int fila = 0; fila < m_filas.size(); ++fila) {
            if (m_filas[fila].receso)
                continue;

            if (m_noDisponible.contains(claveCelda(fila, columna))) {
                if (abierto) {
                    disponibilidad.append(FranjaHoraria{columna + 1, inicio, fin});
                    abierto = false;
                }
                continue;
            }

            if (!abierto) {
                inicio = m_filas[fila].inicioSlot;
                fin = m_filas[fila].finSlot;
                abierto = true;
            } else if (fin == m_filas[fila].inicioSlot) {
                fin = m_filas[fila].finSlot;
            } else {
                disponibilidad.append(FranjaHoraria{columna + 1, inicio, fin});
                inicio = m_filas[fila].inicioSlot;
                fin = m_filas[fila].finSlot;
            }
        }

        if (abierto)
            disponibilidad.append(FranjaHoraria{columna + 1, inicio, fin});
    }

    QVector<PreferenciaDTO> preferencias;
    for (int fila = 0; fila < m_filas.size(); ++fila) {
        if (m_filas[fila].receso)
            continue;

        for (int columna = 0; columna < kNumDias; ++columna) {
            if (m_noDisponible.contains(claveCelda(fila, columna)))
                continue;

            const int clave = claveCelda(fila, columna);
            const bool preferida = m_preferidas.contains(clave);
            const bool evitada = m_evitadas.contains(clave);
            if (!preferida && !evitada)
                continue;

            PreferenciaDTO dto;
            dto.idProfesor = m_cedulaActual;
            dto.franja = FranjaHoraria{columna + 1, m_filas[fila].inicioSlot,
                                       m_filas[fila].finSlot};
            dto.sentido = preferida ? SentidoPreferencia::Preferida
                                    : SentidoPreferencia::Evitada;
            preferencias.append(dto);
        }
    }

    QString error;
    const bool ok = m_nucleo->guardarFichaDocente(
        m_cedulaActual, disponibilidad, preferencias, titulares, suplentes, &error);

    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("No se pudo guardar"),
                             error.isEmpty()
                                 ? QStringLiteral("No se pudo guardar la ficha del docente.")
                                 : error);
        return;
    }

    cargarDocenteActual();

    if (m_estadoGuardado)
        m_estadoGuardado->setText(QStringLiteral("Guardado correctamente"));}

void TeacherListWidget::descartarCambios()
{
    if (m_cedulaActual.isEmpty())
        return;

    m_noDisponible = m_noDisponibleOriginal;
    m_preferidas = m_preferidasOriginal;
    m_evitadas = m_evitadasOriginal;
    m_titulares = m_titularesOriginal;
    m_suplentes = m_suplentesOriginal;

    repintarRejillas();
    repintarAsignaturas();
    actualizarEstadoCambios();
}

void TeacherListWidget::abrirFormularioNuevo()
{
    if (!m_nucleo)
        return;

    if (!pedirConfirmacionCambios())
        return;

    gestor::frontend::forms::TeacherFormDialog dialogo(this);
    dialogo.cargarOpcionesGeneracion(turnosDelNucleo(m_nucleo),
                                     planesDelNucleo(m_nucleo),
                                     aniosDelNucleo(m_nucleo));
    dialogo.setDocentesExistentes(docentesExistentes(m_nucleo));
    connect(&dialogo, &gestor::frontend::forms::TeacherFormDialog::profesorGuardado,
            this, [this](const QString& cedula, const QString& nombre,
                         const QString& email, const QString& telefono) {
                const auto resultado = m_nucleo->crearDocente(cedula, nombre, email, telefono);
                if (!resultado.ok) {
                    QMessageBox::warning(this, QStringLiteral("Error"),
                                         QStringLiteral("No se pudo crear el docente: ")
                                             + resultado.mensajeError);
                } else {
                    cargarDocentes();
                }
            });
    dialogo.exec();

    if (dialogo.result() == QDialog::Accepted) {
        PreferenciasDocenteUI::Preferencia preferencia;
        preferencia.turno = dialogo.turnoSeleccionado();
        preferencia.plan = dialogo.planSeleccionado();
        preferencia.anio = dialogo.anioSeleccionado();
        PreferenciasDocenteUI::guardar(dialogo.cedula(), preferencia);
    }
}

void TeacherListWidget::abrirFormularioEditar(int fila)
{
    if (!m_nucleo || !m_table)
        return;

    if (!pedirConfirmacionCambios())
        return;

    const QString cedula = m_table->item(fila, 0)->text();
    const QString nombre = m_table->item(fila, 1)->text();
    const QString email = m_table->item(fila, 2)->text();
    const QString telefono = m_table->item(fila, 3)->text();

    gestor::frontend::forms::TeacherFormDialog dialogo(this);
    dialogo.setWindowTitle(QStringLiteral("Editar Docente"));
    dialogo.cargarOpcionesGeneracion(turnosDelNucleo(m_nucleo),
                                     planesDelNucleo(m_nucleo),
                                     aniosDelNucleo(m_nucleo));

    const PreferenciasDocenteUI::Preferencia preferencia =
        PreferenciasDocenteUI::leer(cedula);
    dialogo.cargarPreferencias(preferencia.turno, preferencia.plan, preferencia.anio);
    dialogo.cargarDatos(cedula, nombre, email, telefono);
    dialogo.setDocentesExistentes(docentesExistentes(m_nucleo), cedula);

    connect(&dialogo, &gestor::frontend::forms::TeacherFormDialog::profesorGuardado,
            this, [this](const QString& nCedula, const QString& nNombre,
                         const QString& nEmail, const QString& nTelefono) {
                const auto resultado =
                    m_nucleo->actualizarDocente(nCedula, nNombre, nEmail, nTelefono);
                if (!resultado.ok) {
                    QMessageBox::warning(this, QStringLiteral("Error"),
                                         QStringLiteral("No se pudo actualizar el docente: ")
                                             + resultado.mensajeError);
                } else {
                    // Las materias se gestionan en el panel de asignaturas.
                    if (m_cedulaActual == nCedula)
                        cargarDocenteActual();
                    cargarDocentes();
                }
            });
    dialogo.exec();

    if (dialogo.result() == QDialog::Accepted) {
        PreferenciasDocenteUI::Preferencia nueva;
        nueva.turno = dialogo.turnoSeleccionado();
        nueva.plan = dialogo.planSeleccionado();
        nueva.anio = dialogo.anioSeleccionado();
        PreferenciasDocenteUI::guardar(dialogo.cedula(), nueva);
        if (dialogo.cedula() != cedula)
            PreferenciasDocenteUI::eliminar(cedula);
    }
}

void TeacherListWidget::eliminarDocente(int fila)
{
    if (!m_nucleo || !m_table)
        return;

    if (!pedirConfirmacionCambios())
        return;

    const QString cedula = m_table->item(fila, 0)->text();
    if (QMessageBox::question(this, QStringLiteral("Confirmar"),
                              QStringLiteral("¿Está seguro de eliminar a este docente?"))
        != QMessageBox::Yes)
        return;

    if (!m_nucleo->eliminarDocente(cedula)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
                             QStringLiteral("No se pudo eliminar el docente de la base de datos."));
        return;
    }

    PreferenciasDocenteUI::eliminar(cedula);
    if (m_cedulaActual == cedula)
        limpiarPanelDocente();
    cargarDocentes();
}

void TeacherListWidget::actualizarTema(bool modoOscuro)
{
    m_isDark = modoOscuro;

    const QString tituloColor = modoOscuro ? QStringLiteral("#ffffff") : QStringLiteral("#111827");
    const QString subColor = modoOscuro ? QStringLiteral("#aaaaaa") : QStringLiteral("#6b7280");
    const QString panelFondo = modoOscuro ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff");
    const QString panelBorde = modoOscuro ? QStringLiteral("#333333") : QStringLiteral("#e0e0e0");
    const QString tablaFondo = modoOscuro ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff");
    const QString tablaHeader = modoOscuro ? QStringLiteral("#252525") : QStringLiteral("#ffffff");
    const QString tablaTexto = modoOscuro ? QStringLiteral("#ffffff") : QStringLiteral("#334155");
    const QString headerTexto = modoOscuro ? QStringLiteral("#ffffff") : QStringLiteral("#374151");
    const QString bordeFila = modoOscuro ? QStringLiteral("#2a2a2a") : QStringLiteral("#f3f4f6");
    const QString campoFondo = modoOscuro ? QStringLiteral("#2a2a2a") : QStringLiteral("#ffffff");
    const QString campoBorde = modoOscuro ? QStringLiteral("#444444") : QStringLiteral("#d1d5db");
    const QString acento = modoOscuro ? QStringLiteral("#3b82f6") : QStringLiteral("#1e3a8a");
    const QString acentoHover = modoOscuro ? QStringLiteral("#2563eb") : QStringLiteral("#162a69");
    // Color de la retícula de las rejillas de disponibilidad/preferencias. Debe
    // contrastar tanto sobre las celdas pintadas como sobre las vacías.
    const QString lineaRejilla = modoOscuro ? QStringLiteral("#5b6472") : QStringLiteral("#b6c2d1");

    if (auto t = findChild<QLabel*>(QStringLiteral("TeacherTitle")))
        t->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: bold; color: %1;")
                             .arg(tituloColor));
    if (auto s = findChild<QLabel*>(QStringLiteral("TeacherSub")))
        s->setStyleSheet(QStringLiteral("font-size: 14px; color: %1;").arg(subColor));
    if (m_detalleNombre)
        m_detalleNombre->setStyleSheet(
            QStringLiteral("font-size: 17px; font-weight: bold; color: %1;").arg(tituloColor));

    if (auto panel = findChild<QFrame*>(QStringLiteral("TeacherRightPanel"))) {
        panel->setStyleSheet(
            QStringLiteral("QFrame { background-color: %1; border-radius: 12px;"
                           " border: 1px solid %2; }")
                .arg(panelFondo, panelBorde));
    }

    if (m_table) {
        m_table->setStyleSheet(
            QStringLiteral("QTableWidget { background-color: %1; border-radius: 8px;"
                           " border: 1px solid %2; color: %3; }"
                           "QHeaderView::section { background-color: %4; font-weight: bold;"
                           " color: %5; border: none; border-bottom: 1px solid %2;"
                           " padding: 12px; }"
                           "QTableWidget::item { border-bottom: 1px solid %6; padding: 5px;"
                           " color: %3; }")
                .arg(tablaFondo, panelBorde, tablaTexto, tablaHeader, headerTexto, bordeFila));
    }

    if (m_tabs) {
        m_tabs->setStyleSheet(
            modoOscuro
                ? QStringLiteral(
                      "QTabWidget::pane { border: none; background: transparent; }"
                      "QTabBar::tab { background: #252525; color: #aaaaaa; padding: 8px 14px;"
                      " border: 1px solid #333333; border-bottom: none;"
                      " border-top-left-radius: 6px; border-top-right-radius: 6px;"
                      " margin-right: 2px; font-weight: 700; }"
                      "QTabBar::tab:selected { background: #1e1e1e; color: #ffffff;"
                      " border-color: #3b82f6; }"
                      "QTabBar::tab:hover { color: #ffffff; }")
                : QStringLiteral(
                      "QTabWidget::pane { border: none; background: transparent; }"
                      "QTabBar::tab { background: #f1f5f9; color: #475569; padding: 8px 14px;"
                      " border: 1px solid #e2e8f0; border-bottom: none;"
                      " border-top-left-radius: 6px; border-top-right-radius: 6px;"
                      " margin-right: 2px; font-weight: 700; }"
                      "QTabBar::tab:selected { background: #ffffff; color: #1e3a8a;"
                      " border-color: #1e3a8a; }"
                      "QTabBar::tab:hover { color: #0f172a; }"));
    }

    for (QTableWidget* grid : {m_gridDisponibilidad, m_gridPreferencias}) {
        if (!grid)
            continue;
        // La retícula se dibuja con `showGrid(true)`; aquí solo se define su
        // color. Las cabeceras llevan además separadores propios para que las
        // filas/columnas se distingan aunque la celda esté pintada.
        grid->setShowGrid(true);
        grid->setStyleSheet(
            QStringLiteral("QTableWidget { border: 1px solid %1; gridline-color: %2;"
                           " background-color: %3; color: %4; }"
                           "QHeaderView::section { background-color: %5; border: none;"
                           " border-right: 1px solid %2; border-bottom: 1px solid %2;"
                           " font-size: 11px; color: %6; padding: 3px; }")
                .arg(panelBorde, lineaRejilla, tablaFondo, tablaTexto, tablaHeader,
                     headerTexto));
    }

    for (QListWidget* lista : {m_listTitulares, m_listSuplentes}) {
        if (!lista)
            continue;
        lista->setStyleSheet(
            QStringLiteral("QListWidget { background-color: %1; color: %2;"
                           " border: 1px solid %3; border-radius: 6px; padding: 4px; }"
                           "QListWidget::item { padding: 4px; }")
                .arg(campoFondo, tablaTexto, campoBorde));
    }

    for (QLabel* help : findChildren<QLabel*>(QStringLiteral("TeacherHelp")))
        help->setStyleSheet(QStringLiteral("font-size: 11px; color: %1;").arg(subColor));
    for (QLabel* legend : findChildren<QLabel*>(QStringLiteral("TeacherLegendText")))
        legend->setStyleSheet(QStringLiteral("font-size: 11px; color: %1;").arg(subColor));
    for (QLabel* section : findChildren<QLabel*>(QStringLiteral("TeacherSectionTitle")))
        section->setStyleSheet(
            QStringLiteral("font-size: 12px; font-weight: 700; color: %1;").arg(tituloColor));
    if (m_avisoMaterias)
        m_avisoMaterias->setStyleSheet(
            QStringLiteral("font-size: 11px; font-weight: 600; color: %1;")
                .arg(modoOscuro ? QStringLiteral("#fca5a5") : QStringLiteral("#b91c1c")));
    if (m_estadoGuardado)
        m_estadoGuardado->setStyleSheet(
            QStringLiteral("font-size: 11px; color: %1;").arg(subColor));

    for (QPushButton* btn : findChildren<QPushButton*>(QStringLiteral("TeacherManageButton")))
        btn->setStyleSheet(
            QStringLiteral("QPushButton { background-color: %1; color: %2; font-weight: 700;"
                           " padding: 7px 12px; border-radius: 6px; border: 1px solid %3; }")
                .arg(campoFondo, tablaTexto, campoBorde));

    for (QPushButton* btn : findChildren<QPushButton*>(QStringLiteral("TeacherSaveButton")))
        btn->setStyleSheet(
            QStringLiteral("QPushButton { background-color: %1; color: white; font-weight: 700;"
                           " padding: 8px 18px; border-radius: 6px; border: none; }"
                           "QPushButton:hover { background-color: %2; }"
                           "QPushButton:disabled { background-color: %3; color: %4; }")
                .arg(acento, acentoHover,
                     modoOscuro ? QStringLiteral("#333333") : QStringLiteral("#e5e7eb"),
                     subColor));

    for (QPushButton* btn : findChildren<QPushButton*>(QStringLiteral("TeacherDiscardButton")))
        btn->setStyleSheet(
            QStringLiteral("QPushButton { background-color: %1; color: %2; font-weight: 600;"
                           " padding: 8px 14px; border-radius: 6px; border: 1px solid %3; }"
                           "QPushButton:disabled { color: %4; }")
                .arg(campoFondo, tablaTexto, campoBorde, subColor));

    if (m_registerButton) {
        m_registerButton->setStyleSheet(
            QStringLiteral("QPushButton { background-color: %1; color: white; font-weight: bold;"
                           " padding: 10px 20px; border-radius: 6px; font-size: 14px; }"
                           "QPushButton:hover { background-color: %2; }")
                .arg(acento, acentoHover));
    }

    for (QPushButton* btn : findChildren<QPushButton*>(QStringLiteral("TeacherEditButton")))
        btn->setStyleSheet(
            QStringLiteral("color: %1; background: transparent; border: none; font-weight: bold;")
                .arg(modoOscuro ? QStringLiteral("#93c5fd") : QStringLiteral("#2563eb")));
    for (QPushButton* btn : findChildren<QPushButton*>(QStringLiteral("TeacherDeleteButton")))
        btn->setStyleSheet(
            QStringLiteral("color: %1; background: transparent; border: none; font-weight: bold;")
                .arg(modoOscuro ? QStringLiteral("#fca5a5") : QStringLiteral("#dc2626")));

    repintarRejillas();
}
