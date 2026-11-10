/**
 * @file subject_list_widget.cpp
 * @brief Gestión de asignaturas y planes de estudio conectada a NucleoDatos.
 */

#include "subject_list_widget.hpp"
#include "../forms/plan_form_dialog.hpp"
#include "../forms/subject_form_dialog.hpp"
#include "backend/services/NucleoDatos.hpp"
#include "services/tipos_aula_ui.hpp"
#include "../../../middleware/include/middleware/internalclient.h"
#include "../../../middleware/include/middleware/messages.h"

#include <QApplication>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QSet>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

/// Hoja de estilo compartida por las tablas de asignaturas y planes.
QString hojaTabla(bool oscuro) {
    if (oscuro) {
        return QStringLiteral(
            "QTableWidget { background-color: #1e1e1e; border-radius: 12px;"
            " border: 1px solid #333333; gridline-color: transparent; outline: none;"
            " color: #ffffff; }"
            "QHeaderView::section { background-color: #252525; font-weight: 700;"
            " color: #ffffff; border: none; border-bottom: 1px solid #333333;"
            " padding: 14px 16px; font-size: 13px; }"
            "QTableWidget::item { border: none; border-bottom: 1px solid #2a2a2a;"
            " padding: 10px 16px; color: #ffffff; font-size: 13px; }");
    }

    return QStringLiteral(
        "QTableWidget { background-color: white; border-radius: 12px;"
        " border: 1px solid #e2e8f0; gridline-color: transparent; outline: none;"
        " color: #0f172a; }"
        "QHeaderView::section { background-color: #f8fafc; font-weight: 700;"
        " color: #475569; border: none; border-bottom: 1px solid #e2e8f0;"
        " padding: 14px 16px; font-size: 13px; }"
        "QTableWidget::item { border: none; border-bottom: 1px solid #f1f5f9;"
        " padding: 10px 16px; color: #0f172a; font-size: 13px; }");
}

QString hojaBotonEditar(bool oscuro) {
    return oscuro
               ? QStringLiteral("QPushButton { color: #93c5fd; background: #1e3a5f;"
                                " border: none; font-weight: 700; font-size: 12px;"
                                " border-radius: 6px; }"
                                "QPushButton:hover { background-color: #1e40af; }")
               : QStringLiteral("QPushButton { color: #2563eb; background: #eff6ff;"
                                " border: none; font-weight: 700; font-size: 12px;"
                                " border-radius: 6px; }"
                                "QPushButton:hover { background-color: #dbeafe; }");
}

QString hojaBotonDuplicar(bool oscuro) {
    return oscuro
               ? QStringLiteral("QPushButton { color: #c4b5fd; background: #2e1065;"
                                " border: none; font-weight: 700; font-size: 12px;"
                                " border-radius: 6px; }"
                                "QPushButton:hover { background-color: #4c1d95; }")
               : QStringLiteral("QPushButton { color: #7c3aed; background: #f5f3ff;"
                                " border: none; font-weight: 700; font-size: 12px;"
                                " border-radius: 6px; }"
                                "QPushButton:hover { background-color: #ede9fe; }");
}

QString hojaBotonEliminar(bool oscuro) {
    return oscuro
               ? QStringLiteral("QPushButton { color: #fca5a5; background: #450a0a;"
                                " border: none; font-weight: 700; font-size: 12px;"
                                " border-radius: 6px; }"
                                "QPushButton:hover { background-color: #7f1d1d; }")
               : QStringLiteral("QPushButton { color: #dc2626; background: #ffeeec;"
                                " border: none; font-weight: 700; font-size: 12px;"
                                " border-radius: 6px; }"
                                "QPushButton:hover { background-color: #fee2e2; }");
}

} // namespace

SubjectListWidget::SubjectListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    actualizarTema(qApp->property("isDarkMode").toBool());
}

void SubjectListWidget::setClient(InternalClient* client) {
    m_client = client;
    if (m_client) {
        connect(m_client, &InternalClient::respuestaRecibida,
                this, &SubjectListWidget::onRespuestaRecibida);
    }
}

void SubjectListWidget::setNucleoDatos(NucleoDatos* nucleo) {
    m_nucleo = nucleo;
    cargarMaterias();
}

void SubjectListWidget::setupUi() {
    // Layout principal horizontal que divide la pantalla en dos columnas
    QHBoxLayout *masterLayout = new QHBoxLayout(this);
    masterLayout->setContentsMargins(40, 40, 40, 40);
    masterLayout->setSpacing(24);

    // ==========================================
    // COLUMNA IZQUIERDA: CONTENIDO PRINCIPAL
    // ==========================================
    QVBoxLayout *leftContentLayout = new QVBoxLayout();
    leftContentLayout->setSpacing(18);

    // Cabecera
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);

    m_titulo = new QLabel("Gestión de Asignaturas", this);
    m_titulo->setObjectName("SubjectTitle");
    m_titulo->setWordWrap(true);
    m_titulo->setMinimumWidth(0);

    m_subtitulo = new QLabel("Administra las asignaturas y su tipo de aula", this);
    m_subtitulo->setObjectName("SubjectSub");
    m_subtitulo->setWordWrap(true);
    m_subtitulo->setMinimumWidth(0);

    textLayout->addWidget(m_titulo);
    textLayout->addWidget(m_subtitulo);

    m_registerButton = new QPushButton("+ Registrar Asignatura", this);
    m_registerButton->setObjectName("SubjectRegBtn");
    m_registerButton->setCursor(Qt::PointingHandCursor);
    m_registerButton->setStyleSheet(
        "QPushButton { background-color: #1e3a8a; color: white; font-weight: 700; "
        "padding: 10px 20px; border-radius: 6px; font-size: 13px; } "
        "QPushButton:hover { background-color: #162a69; }");

    headerLayout->addLayout(textLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(m_registerButton);
    leftContentLayout->addLayout(headerLayout);

    // Pestañas: asignaturas y planes de estudio
    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName("SubjectTabs");

    // ── Pestaña de asignaturas ────────────────────────────────────────────
    QWidget *subjectPage = new QWidget(m_tabs);
    QVBoxLayout *subjectLayout = new QVBoxLayout(subjectPage);
    subjectLayout->setContentsMargins(0, 12, 0, 0);

    m_table = new QTableWidget(0, 3, subjectPage);
    m_table->setObjectName("SubjectTable");
    m_table->setHorizontalHeaderLabels({"Nombre", "Tipo de aula", "Acciones"});

    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 260);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    if (auto itemAcciones = m_table->horizontalHeaderItem(2)) {
        itemAcciones->setTextAlignment(Qt::AlignCenter);
    }

    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setDefaultSectionSize(60);

    subjectLayout->addWidget(m_table);
    m_tabs->addTab(subjectPage, "Asignaturas");

    // ── Pestaña de planes de estudio ──────────────────────────────────────
    QWidget *planPage = new QWidget(m_tabs);
    QVBoxLayout *planLayout = new QVBoxLayout(planPage);
    planLayout->setContentsMargins(0, 12, 0, 0);

    QLabel *planHint = new QLabel(
        "Los planes nuevos usan una lista global de materias y se aplican a los "
        "años 1.º a 5.º. Los planes existentes conservan su distribución por año.",
        planPage);
    planHint->setObjectName("PlanHint");
    planHint->setWordWrap(true);
    planLayout->addWidget(planHint);

    m_planTable = new QTableWidget(0, 5, planPage);
    m_planTable->setObjectName("PlanTable");
    m_planTable->setHorizontalHeaderLabels(
        {"Código", "Nombre", "Materias", "Horas totales", "Acciones"});

    QHeaderView *planHeader = m_planTable->horizontalHeader();
    planHeader->setSectionResizeMode(0, QHeaderView::Fixed);
    planHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    planHeader->setSectionResizeMode(2, QHeaderView::Fixed);
    planHeader->setSectionResizeMode(3, QHeaderView::Fixed);
    planHeader->setSectionResizeMode(4, QHeaderView::Fixed);
    m_planTable->setColumnWidth(0, 110);
    m_planTable->setColumnWidth(2, 100);
    m_planTable->setColumnWidth(3, 120);
    m_planTable->setColumnWidth(4, 340);
    planHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    if (auto itemAcciones = m_planTable->horizontalHeaderItem(4)) {
        itemAcciones->setTextAlignment(Qt::AlignCenter);
    }

    m_planTable->verticalHeader()->setVisible(false);
    m_planTable->setShowGrid(false);
    m_planTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_planTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_planTable->verticalHeader()->setDefaultSectionSize(60);

    planLayout->addWidget(m_planTable);
    m_tabs->addTab(planPage, "Planes de estudio");

    leftContentLayout->addWidget(m_tabs, 1);

    // ==========================================
    // COLUMNA DERECHA: PANEL LATERAL DINÁMICO
    // ==========================================
    QFrame *sidePanel = new QFrame(this);
    sidePanel->setObjectName("SubjectSidePanel");
    sidePanel->setMinimumWidth(220);
    sidePanel->setMaximumWidth(310);

    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(24, 28, 24, 28);
    sideLayout->setSpacing(22);

    QLabel *summaryTitle = new QLabel("Controles de Resumen", sidePanel);
    summaryTitle->setObjectName("SummaryTitle");
    summaryTitle->setWordWrap(true);
    sideLayout->addWidget(summaryTitle);

    QHBoxLayout *occupancyLayout = new QHBoxLayout();
    occupancyLayout->setAlignment(Qt::AlignVCenter);

    QLabel *occLabel = new QLabel("Ocupación de\nAulas:", sidePanel);
    occLabel->setObjectName("OccLabel");

    m_occValLabel = new QLabel("0%", sidePanel);
    m_occValLabel->setObjectName("OccVal");
    m_occValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    occupancyLayout->addWidget(occLabel);
    occupancyLayout->addStretch();
    occupancyLayout->addWidget(m_occValLabel);
    sideLayout->addLayout(occupancyLayout);

    m_progressBar = new QProgressBar(sidePanel);
    m_progressBar->setObjectName("SubjectProgressBar");
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    sideLayout->addWidget(m_progressBar);

    QFrame *lineDiv = new QFrame(sidePanel);
    lineDiv->setObjectName("SubjectLineDiv");
    lineDiv->setFrameShape(QFrame::HLine);
    sideLayout->addWidget(lineDiv);

    QLabel *notifTitle = new QLabel("Notificaciones Recientes", sidePanel);
    notifTitle->setObjectName("NotifTitle");
    notifTitle->setWordWrap(true);
    sideLayout->addWidget(notifTitle);

    // Contenedor dinámico de notificaciones
    m_notifLayout = new QVBoxLayout();
    m_notifLayout->setSpacing(8);
    sideLayout->addLayout(m_notifLayout);
    sideLayout->addStretch();

    masterLayout->addLayout(leftContentLayout, 3);
    masterLayout->addWidget(sidePanel, 1);

    connect(m_registerButton, &QPushButton::clicked, this, &SubjectListWidget::abrirFormularioNuevo);
    connect(m_tabs, &QTabWidget::currentChanged, this, &SubjectListWidget::onTabChanged);

    onTabChanged(0);
}

// ==========================================
// PESTAÑAS
// ==========================================
void SubjectListWidget::onTabChanged(int index) {
    Q_UNUSED(index);
    refrescarBotonRegistro();
}

void SubjectListWidget::refrescarBotonRegistro() {
    if (!m_tabs || !m_registerButton) {
        return;
    }

    if (m_tabs->currentIndex() == 1) {
        m_registerButton->setText("+ Registrar Plan de Estudio");
        if (m_titulo) m_titulo->setText("Gestión de Planes de Estudio");
        if (m_subtitulo) {
            m_subtitulo->setText(
                "Administra los planes, sus materias y horas semanales");
        }
    } else {
        m_registerButton->setText("+ Registrar Asignatura");
        if (m_titulo) m_titulo->setText("Gestión de Asignaturas");
        if (m_subtitulo) {
            m_subtitulo->setText("Administra las asignaturas y su tipo de aula");
        }
    }
}

// ==========================================
// CARGAR DESDE NUCLEO DATOS Y ACTUALIZAR PANEL
// ==========================================
void SubjectListWidget::cargarMaterias() {
    if (!m_nucleo) return;

    m_table->setRowCount(0);
    auto res = m_nucleo->listarMaterias();

    if (!res.ok) {
        QMessageBox::warning(this, "Error BD", "No se pudo cargar la lista de asignaturas.");
    } else {
        for (const auto& materia : res.valor) {
            insertarMateriaEnTabla(materia.id, materia.nombre, materia.requisitos,
                                   materia.tipoAula, materia.idTipoAula);
        }
    }

    cargarPlanes();

    // Actualizar dinámicamente el panel lateral con datos reales de aulas y materias
    actualizarPanelLateral();

    // Los botones de cada fila se crean al poblar la tabla; aplicar el tema
    // vigente después evita que queden con el estilo por defecto.
    actualizarTema(qApp->property("isDarkMode").toBool());
}

void SubjectListWidget::cargarPlanes() {
    if (!m_nucleo || !m_planTable) return;

    m_planTable->setRowCount(0);

    const auto res = m_nucleo->listarPlanes();
    if (!res.ok) {
        QMessageBox::warning(this, "Error BD",
                             "No se pudo cargar la lista de planes de estudio.");
        return;
    }

    for (const PlanDTO& plan : res.valor) {
        int numMaterias = 0;
        int horasTotales = 0;

        const auto carga = m_nucleo->listarCargaDePlan(plan.codigo);
        if (carga.ok) {
            QSet<int> ids;
            for (const CargaHorariaDTO& item : carga.valor) {
                ids.insert(item.idMateria);
                horasTotales += item.horas;
            }
            numMaterias = ids.size();
        }

        insertarPlanEnTabla(plan, numMaterias, horasTotales);
    }

    actualizarTema(qApp->property("isDarkMode").toBool());
}

void SubjectListWidget::actualizarPanelLateral() {
    if (!m_nucleo) return;

    auto resAulas = m_nucleo->listarAulas();
    auto resMaterias = m_nucleo->listarMaterias();
    auto resPlanes = m_nucleo->listarPlanes();

    int totalAulas = resAulas.ok ? resAulas.valor.size() : 0;
    int totalMaterias = resMaterias.ok ? resMaterias.valor.size() : 0;
    int totalPlanes = resPlanes.ok ? resPlanes.valor.size() : 0;

    int aulasOcupadasEstimadas = 0;

    // Lógica de emparejamiento inteligente (Estimación de compatibilidad)
    if (totalAulas > 0 && totalMaterias > 0) {
        // Hacemos una copia temporal de las aulas para no contar la misma dos veces
        auto aulasDisponibles = resAulas.valor;

        for (const auto& materia : resMaterias.valor) {
            // El emparejamiento usa el tipo de aula del catálogo (spec 009):
            // una materia solo puede ocupar aulas de su mismo tipo.
            const QString tipoMateria = materia.tipoAula.toLower().trimmed();

            for (int i = 0; i < aulasDisponibles.size(); ++i) {
                const QString tipoAula = aulasDisponibles[i].tipoAula.toLower().trimmed();

                bool coincidenciaDirecta = !tipoMateria.isEmpty() && tipoMateria == tipoAula;
                bool coincidenciaGeneral =
                    tipoMateria.isEmpty() && tipoAula.compare(TiposAulaUI::nombreTipoGeneral(),
                                                              Qt::CaseInsensitive) == 0;

                if (coincidenciaDirecta || coincidenciaGeneral) {
                    aulasOcupadasEstimadas++;
                    aulasDisponibles.removeAt(i); // Marcamos esta aula como usada para la estimación
                    break; // Pasamos a la siguiente materia
                }
            }
        }
    }

    int porcentajeOcupacion = 0;
    if (totalAulas > 0) {
        porcentajeOcupacion = qMin(100, (aulasOcupadasEstimadas * 100) / totalAulas);
    }

    if (m_occValLabel) m_occValLabel->setText(QString::number(porcentajeOcupacion) + "%");
    if (m_progressBar) m_progressBar->setValue(porcentajeOcupacion);

    // Refrescar Notificaciones Recientes dinámicamente
    QLayoutItem *child;
    while ((child = m_notifLayout->takeAt(0)) != nullptr) {
        if (child->widget()) { child->widget()->deleteLater(); }
        delete child;
    }

    bool isDark = qApp->property("isDarkMode").toBool();
    auto crearItemNotif = [this, isDark](const QString& titulo, const QString& tiempo) {
        QWidget *itemWidget = new QWidget(this);
        QVBoxLayout *itemLayout = new QVBoxLayout(itemWidget);
        itemLayout->setContentsMargins(0, 4, 0, 4);
        itemLayout->setSpacing(2);

        QLabel *lblTitle = new QLabel(titulo, itemWidget);
        lblTitle->setObjectName("NotifItemTitle");
        lblTitle->setWordWrap(true);
        QString titleColor = isDark ? "#ffffff;" : "#1e293b;";
        lblTitle->setStyleSheet(QString("font-size: 12px; font-weight: 700; color: %1 border: none; background: transparent;").arg(titleColor));

        QLabel *lblTime = new QLabel(tiempo, itemWidget);
        lblTime->setObjectName("NotifItemTime");
        QString timeColor = isDark ? "#aaaaaa;" : "#64748b;";
        lblTime->setStyleSheet(QString("font-size: 11px; color: %1 border: none; background: transparent;").arg(timeColor));

        itemLayout->addWidget(lblTitle);
        itemLayout->addWidget(lblTime);
        m_notifLayout->addWidget(itemWidget);
    };

    crearItemNotif(QString("Catálogo activo: %1 materias").arg(totalMaterias), "Evaluando compatibilidad");
    crearItemNotif(QString("Infraestructura: %1 aulas registradas").arg(totalAulas),
                   totalAulas > 0 ? QString("%1 espacios pre-asignables").arg(aulasOcupadasEstimadas) : "⚠️ Sin aulas registradas");
    crearItemNotif(QString("Planes de estudio: %1").arg(totalPlanes),
                   totalPlanes > 0 ? "Listos para asignar a cursos" : "⚠️ Sin planes registrados");
}

void SubjectListWidget::insertarMateriaEnTabla(int id, const QString& nombre,
                                               const QString& requisitos,
                                               const QString& tipoAula, int idTipoAula) {
    Q_UNUSED(requisitos);
    int fila = m_table->rowCount();
    m_table->insertRow(fila);

    QTableWidgetItem *itemNombre = new QTableWidgetItem(nombre);
    QTableWidgetItem *itemTipo =
        new QTableWidgetItem(tipoAula.isEmpty() ? TiposAulaUI::nombreTipoGeneral() : tipoAula);

    itemNombre->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    itemTipo->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_table->setItem(fila, 0, itemNombre);
    m_table->setItem(fila, 1, itemTipo);

    QWidget *panelAcciones = new QWidget();
    QHBoxLayout *layoutAcciones = new QHBoxLayout(panelAcciones);
    layoutAcciones->setContentsMargins(0, 0, 0, 0);
    layoutAcciones->setSpacing(10);
    layoutAcciones->setAlignment(Qt::AlignCenter);

    QPushButton *btnEditar = new QPushButton("✏️ Editar");
    btnEditar->setObjectName("SubjectEditButton");
    btnEditar->setFixedSize(90, 34);
    btnEditar->setCursor(Qt::PointingHandCursor);

    QPushButton *btnEliminar = new QPushButton("🗑️ Eliminar");
    btnEliminar->setObjectName("SubjectDeleteButton");
    btnEliminar->setFixedSize(100, 34);
    btnEliminar->setCursor(Qt::PointingHandCursor);

    layoutAcciones->addWidget(btnEditar);
    layoutAcciones->addWidget(btnEliminar);

    int materiaId = id;

    connect(btnEliminar, &QPushButton::clicked, this, [this, materiaId, nombre]() {
        if (QMessageBox::question(this, "Confirmar eliminación",
                                  "¿Está seguro de eliminar la asignatura '" + nombre + "'?") == QMessageBox::Yes) {
            if (m_nucleo->eliminarMateria(materiaId)) {
                cargarMaterias();
            } else {
                QMessageBox::warning(this, "Acción Bloqueada",
                                     "No se puede eliminar esta asignatura porque actualmente está vinculada a un docente registrado.\n\n"
                                     "Por favor, edite o elimine al docente correspondiente antes de borrar esta materia del catálogo.");
            }
        }
    });

    connect(btnEditar, &QPushButton::clicked, this, [this, materiaId, nombre, requisitos, idTipoAula]() {
        QVector<TipoAulaDTO> tipos;
        if (const auto resultado = m_nucleo->listarTiposAula(); resultado.ok)
            tipos = resultado.valor;

        gestor::frontend::forms::SubjectFormDialog dialogo(this);
        dialogo.setWindowTitle("Editar Asignatura");
        dialogo.cargarTiposAula(tipos, idTipoAula);
        dialogo.cargarDatos(nombre, requisitos, idTipoAula);

        connect(&dialogo, &gestor::frontend::forms::SubjectFormDialog::materiaGuardada,
                this, [this, materiaId](const QString& nNombre, const QString& nRequisitos,
                                        int nIdTipoAula) {
                    auto resultado = m_nucleo->actualizarMateria(materiaId, nNombre, nRequisitos,
                                                                 nIdTipoAula);
                    if (!resultado.ok) {
                        QMessageBox::warning(this, "Error", "No se pudo actualizar la asignatura: " + resultado.mensajeError);
                    } else {
                        cargarMaterias();
                    }
                });
        dialogo.exec();
    });

    m_table->setCellWidget(fila, 2, panelAcciones);
}

void SubjectListWidget::insertarPlanEnTabla(const PlanDTO& plan, int numMaterias,
                                            int horasTotales) {
    const int fila = m_planTable->rowCount();
    m_planTable->insertRow(fila);

    auto* itemCodigo = new QTableWidgetItem(plan.codigo);
    auto* itemNombre = new QTableWidgetItem(plan.nombre);
    auto* itemMaterias = new QTableWidgetItem(QString::number(numMaterias));
    auto* itemHoras = new QTableWidgetItem(
        QString("%1 h").arg(horasTotales));

    if (!plan.descripcion.trimmed().isEmpty()) {
        itemNombre->setToolTip(plan.descripcion.trimmed());
    }

    itemCodigo->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    itemNombre->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    itemMaterias->setTextAlignment(Qt::AlignCenter);
    itemHoras->setTextAlignment(Qt::AlignCenter);

    m_planTable->setItem(fila, 0, itemCodigo);
    m_planTable->setItem(fila, 1, itemNombre);
    m_planTable->setItem(fila, 2, itemMaterias);
    m_planTable->setItem(fila, 3, itemHoras);

    QWidget *panelAcciones = new QWidget();
    QHBoxLayout *layoutAcciones = new QHBoxLayout(panelAcciones);
    layoutAcciones->setContentsMargins(0, 0, 0, 0);
    layoutAcciones->setSpacing(8);
    layoutAcciones->setAlignment(Qt::AlignCenter);

    QPushButton *btnEditar = new QPushButton("✏️ Editar");
    btnEditar->setObjectName("PlanEditButton");
    btnEditar->setFixedSize(90, 34);
    btnEditar->setCursor(Qt::PointingHandCursor);

    QPushButton *btnDuplicar = new QPushButton("📄 Duplicar");
    btnDuplicar->setObjectName("PlanDuplicateButton");
    btnDuplicar->setFixedSize(100, 34);
    btnDuplicar->setCursor(Qt::PointingHandCursor);

    QPushButton *btnEliminar = new QPushButton("🗑️ Eliminar");
    btnEliminar->setObjectName("PlanDeleteButton");
    btnEliminar->setFixedSize(100, 34);
    btnEliminar->setCursor(Qt::PointingHandCursor);

    layoutAcciones->addWidget(btnEditar);
    layoutAcciones->addWidget(btnDuplicar);
    layoutAcciones->addWidget(btnEliminar);

    connect(btnEditar, &QPushButton::clicked, this, [this, plan]() { editarPlan(plan); });
    connect(btnDuplicar, &QPushButton::clicked, this, [this, plan]() { duplicarPlan(plan); });
    connect(btnEliminar, &QPushButton::clicked, this, [this, plan]() { eliminarPlan(plan); });

    m_planTable->setCellWidget(fila, 4, panelAcciones);
}

void SubjectListWidget::editarPlan(const PlanDTO& plan) {
    if (!m_nucleo) return;

    const auto carga = m_nucleo->listarCargaDePlan(plan.codigo);
    if (!carga.ok) {
        QMessageBox::warning(this, "No se pudo abrir el plan", carga.mensajeError);
        return;
    }

    gestor::frontend::forms::PlanFormDialog dialogo(m_nucleo, this);
    dialogo.prepararEdicion(plan, carga.valor);
    if (dialogo.exec() == QDialog::Accepted) {
        cargarPlanes();
        actualizarPanelLateral();
    }
}

void SubjectListWidget::duplicarPlan(const PlanDTO& plan) {
    if (!m_nucleo) return;

    const auto carga = m_nucleo->listarCargaDePlan(plan.codigo);
    if (!carga.ok) {
        QMessageBox::warning(this, "No se pudo duplicar el plan", carga.mensajeError);
        return;
    }

    QSet<int> cursos;
    for (const CargaHorariaDTO& item : carga.valor) {
        cursos.insert(item.curso);
    }
    const bool global = cursos.isEmpty()
                        || (cursos.size() == 1
                            && *cursos.constBegin() >= 1
                            && *cursos.constBegin() <= 5);

    gestor::frontend::forms::PlanFormDialog dialogo(m_nucleo, this);
    dialogo.prepararCreacion(sugerirCodigoPlan(),
                             plan.nombre + " (copia)",
                             plan.descripcion,
                             carga.valor,
                             global);

    if (dialogo.exec() == QDialog::Accepted) {
        cargarPlanes();
        actualizarPanelLateral();
    }
}

void SubjectListWidget::eliminarPlan(const PlanDTO& plan) {
    if (!m_nucleo) return;

    // Impacto real: materias que quedarán desvinculadas del plan.
    QStringList nombresMaterias;
    QSet<int> ids;
    if (const auto carga = m_nucleo->listarCargaDePlan(plan.codigo); carga.ok) {
        for (const CargaHorariaDTO& item : carga.valor) {
            if (ids.contains(item.idMateria)) continue;
            ids.insert(item.idMateria);
            nombresMaterias << item.nombreMateria;
        }
    }

    QString detalle;
    if (nombresMaterias.isEmpty()) {
        detalle = "El plan no tiene materias asociadas.";
    } else {
        detalle = QString("También se eliminarán %1 vínculo(s) con estas materias:\n\n• %2")
                      .arg(nombresMaterias.size())
                      .arg(nombresMaterias.join("\n• "));
    }

    QMessageBox mensaje(this);
    mensaje.setIcon(QMessageBox::Warning);
    mensaje.setWindowTitle("Eliminar plan de estudio");
    mensaje.setText(QString("¿Eliminar el plan \"%1\" (%2)?")
                        .arg(plan.nombre, plan.codigo));
    mensaje.setInformativeText(detalle);
    mensaje.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mensaje.setDefaultButton(QMessageBox::No);

    if (mensaje.exec() != QMessageBox::Yes) {
        return;
    }

    if (!m_nucleo->eliminarConCascada("plan", plan.codigo)) {
        QMessageBox::warning(this, "No se pudo eliminar",
                             "El plan no se pudo eliminar. Intente de nuevo.");
        return;
    }

    cargarPlanes();
    actualizarPanelLateral();
}

QString SubjectListWidget::sugerirCodigoPlan() const {
    int maximo = 1000;
    if (m_nucleo) {
        const auto resultado = m_nucleo->listarPlanes();
        if (resultado.ok) {
            for (const PlanDTO& plan : resultado.valor) {
                bool ok = false;
                const int numero = plan.codigo.toInt(&ok);
                if (ok && numero > maximo) {
                    maximo = numero;
                }
            }
        }
    }
    return QString::number(maximo + 1);
}

void SubjectListWidget::abrirFormularioNuevo() {
    if (!m_nucleo) return;

    // Su pestaña de plan (#97) usa el formulario de plan, no el de asignatura.
    if (m_tabs && m_tabs->currentIndex() == 1) {
        abrirFormularioPlanNuevo();
        return;
    }

    // 016 RF-2: si el catálogo no se puede leer, no se abre el formulario de asignatura.
    const auto tipos = m_nucleo->listarTiposAula();
    if (!tipos.ok) {
        QMessageBox::warning(this, "Catálogo no disponible",
                             "No se pudo leer el catálogo de tipos de aula. " + tipos.mensajeError);
        return;
    }

    // 016 RF-2: resolver el general antes de abrir el diálogo; si no está, no se usa otro.
    const auto idGeneral = TiposAulaUI::idTipoGeneral(tipos.valor);
    if (!idGeneral.ok) {
        QMessageBox::warning(this, "Tipo general no disponible", idGeneral.mensajeError);
        return;
    }

    gestor::frontend::forms::SubjectFormDialog dialogo(this);
    dialogo.cargarTiposAula(tipos.valor, idGeneral.valor);
    connect(&dialogo, &gestor::frontend::forms::SubjectFormDialog::materiaGuardada,
            this, [this](const QString& nombre, const QString& requisitos, int idTipoAula) {
                auto resultado = m_nucleo->crearMateria(nombre, requisitos, idTipoAula);
                if (!resultado.ok) {
                    QMessageBox::warning(this, "Error", "No se pudo crear la asignatura: " + resultado.mensajeError);
                } else {
                    cargarMaterias();
                }
            });
    dialogo.exec();
}

void SubjectListWidget::abrirFormularioPlanNuevo() {
    if (!m_nucleo) return;

    const auto materias = m_nucleo->listarMaterias();
    if (!materias.ok) {
        QMessageBox::warning(this, "Error BD", materias.mensajeError);
        return;
    }
    if (materias.valor.isEmpty()) {
        QMessageBox::information(
            this, "Sin asignaturas",
            "Registre al menos una asignatura en la pestaña «Asignaturas» antes de "
            "crear un plan de estudio.");
        return;
    }

    gestor::frontend::forms::PlanFormDialog dialogo(m_nucleo, this);
    dialogo.prepararCreacion(sugerirCodigoPlan());
    if (dialogo.exec() == QDialog::Accepted) {
        cargarPlanes();
        actualizarPanelLateral();
    }
}

void SubjectListWidget::onRespuestaRecibida(const QJsonObject& respuesta) {
    Q_UNUSED(respuesta);
}

void SubjectListWidget::enviarCrearMateria(const QString& nombre, const QString& requisitos) {
    if (!m_client) return;
    QJsonObject payload;
    payload["nombre"] = nombre;
    payload["requisitos"] = requisitos;
    m_client->enviarSolicitud("crear_materia", payload);
}

void SubjectListWidget::enviarActualizarMateria(int id, const QString& nombre, const QString& requisitos) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = id;
    payload["nombre"] = nombre;
    payload["requisitos"] = requisitos;
    m_client->enviarSolicitud("actualizar_materia", payload);
}

void SubjectListWidget::enviarEliminarMateria(int id) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = id;
    m_client->enviarSolicitud("eliminar_materia", payload);
}

// ==========================================
// TEMA
// ==========================================
void SubjectListWidget::actualizarTema(bool modoOscuro) {
    const QString textoTitulo = modoOscuro ? "#ffffff" : "#0f172a";
    const QString textoSuave = modoOscuro ? "#aaaaaa" : "#64748b";
    const QString textoPanel = modoOscuro ? "#ffffff" : "#0f172a";
    const QString textoPanelSuave = modoOscuro ? "#aaaaaa" : "#475569";

    if (m_titulo) {
        m_titulo->setStyleSheet(
            QString("font-size: 24px; font-weight: 800; color: %1; border: none; background: transparent;")
                .arg(textoTitulo));
    }
    if (m_subtitulo) {
        m_subtitulo->setStyleSheet(
            QString("font-size: 14px; color: %1; border: none; background: transparent;")
                .arg(textoSuave));
    }

    if (auto st = findChild<QLabel*>("SummaryTitle")) {
        st->setStyleSheet(QString("font-size: 15px; font-weight: 800; color: %1; border: none; background: transparent;")
                              .arg(textoPanel));
    }
    if (auto ol = findChild<QLabel*>("OccLabel")) {
        ol->setStyleSheet(QString("font-size: 13px; color: %1; border: none; background: transparent; font-weight: 600; line-height: 1.2;")
                              .arg(textoPanelSuave));
    }
    if (auto ov = findChild<QLabel*>("OccVal")) {
        ov->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1; border: none; background: transparent;")
                              .arg(textoPanel));
    }
    if (auto nt = findChild<QLabel*>("NotifTitle")) {
        nt->setStyleSheet(QString("font-size: 15px; font-weight: 800; color: %1; border: none; background: transparent;")
                              .arg(textoPanel));
    }
    if (auto hint = findChild<QLabel*>("PlanHint")) {
        hint->setStyleSheet(QString("font-size: 12px; color: %1; border: none; background: transparent;")
                                .arg(textoSuave));
    }

    for (QLabel *lbl : findChildren<QLabel*>("NotifItemTitle")) {
        lbl->setStyleSheet(
            QString("font-size: 12px; font-weight: 700; color: %1; border: none; background: transparent;")
                .arg(modoOscuro ? "#ffffff" : "#1e293b"));
    }
    for (QLabel *lbl : findChildren<QLabel*>("NotifItemTime")) {
        lbl->setStyleSheet(
            QString("font-size: 11px; color: %1; border: none; background: transparent;")
                .arg(modoOscuro ? "#aaaaaa" : "#64748b"));
    }

    if (auto panel = findChild<QFrame*>("SubjectSidePanel")) {
        panel->setStyleSheet(
            modoOscuro
                ? "QFrame { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; color: white; }"
                : "QFrame { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    }
    if (auto line = findChild<QFrame*>("SubjectLineDiv")) {
        line->setStyleSheet(
            modoOscuro
                ? "color: #333333; border: none; background-color: #333333; max-height: 1px;"
                : "color: #f1f5f9; border: none; background-color: #f1f5f9; max-height: 1px;");
    }
    if (auto bar = findChild<QProgressBar*>("SubjectProgressBar")) {
        bar->setStyleSheet(
            modoOscuro
                ? "QProgressBar { background-color: #333333; border-radius: 4px; border: none; } QProgressBar::chunk { background-color: #3b82f6; border-radius: 4px; }"
                : "QProgressBar { background-color: #e2e8f0; border-radius: 4px; border: none; } QProgressBar::chunk { background-color: #1e3a8a; border-radius: 4px; }");
    }

    if (m_tabs) {
        m_tabs->setStyleSheet(
            modoOscuro
                ? "QTabWidget::pane { border: none; background: transparent; }"
                  "QTabBar::tab { background: #1e1e1e; color: #aaaaaa; padding: 9px 18px;"
                  " border: 1px solid #333333; border-bottom: none;"
                  " border-top-left-radius: 7px; border-top-right-radius: 7px;"
                  " margin-right: 4px; font-weight: 700; }"
                  "QTabBar::tab:selected { background: #252525; color: #ffffff;"
                  " border-color: #3b82f6; }"
                  "QTabBar::tab:hover { color: #ffffff; }"
                : "QTabWidget::pane { border: none; background: transparent; }"
                  "QTabBar::tab { background: #f1f5f9; color: #475569; padding: 9px 18px;"
                  " border: 1px solid #e2e8f0; border-bottom: none;"
                  " border-top-left-radius: 7px; border-top-right-radius: 7px;"
                  " margin-right: 4px; font-weight: 700; }"
                  "QTabBar::tab:selected { background: #ffffff; color: #1e3a8a;"
                  " border-color: #1e3a8a; }"
                  "QTabBar::tab:hover { color: #0f172a; }");
    }

    if (m_table) {
        m_table->setStyleSheet(hojaTabla(modoOscuro));
    }
    if (m_planTable) {
        m_planTable->setStyleSheet(hojaTabla(modoOscuro));
    }

    for (auto* boton : findChildren<QPushButton*>("SubjectEditButton")) {
        boton->setStyleSheet(hojaBotonEditar(modoOscuro));
    }
    for (auto* boton : findChildren<QPushButton*>("SubjectDeleteButton")) {
        boton->setStyleSheet(hojaBotonEliminar(modoOscuro));
    }
    for (auto* boton : findChildren<QPushButton*>("PlanEditButton")) {
        boton->setStyleSheet(hojaBotonEditar(modoOscuro));
    }
    for (auto* boton : findChildren<QPushButton*>("PlanDuplicateButton")) {
        boton->setStyleSheet(hojaBotonDuplicar(modoOscuro));
    }
    for (auto* boton : findChildren<QPushButton*>("PlanDeleteButton")) {
        boton->setStyleSheet(hojaBotonEliminar(modoOscuro));
    }

    if (m_registerButton) {
        m_registerButton->setStyleSheet(
            modoOscuro
                ? "QPushButton { background-color: #3b82f6; color: white; font-weight: 700;"
                  " padding: 10px 20px; border-radius: 6px; font-size: 13px; }"
                  "QPushButton:hover { background-color: #2563eb; }"
                : "QPushButton { background-color: #1e3a8a; color: white; font-weight: 700;"
                  " padding: 10px 20px; border-radius: 6px; font-size: 13px; }"
                  "QPushButton:hover { background-color: #162a69; }");
    }
}
