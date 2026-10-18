/**
 * @file subject_list_widget.cpp
 * @brief Implementación del widget de gestión de asignaturas conectado a NucleoDatos y panel dinámico
 */

#include "subject_list_widget.hpp"
#include "../forms/subject_form_dialog.hpp"
#include "backend/services/NucleoDatos.hpp"
#include "../../../middleware/include/middleware/internalclient.h"
#include "../../../middleware/include/middleware/messages.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFrame>
#include <QProgressBar>
#include <QApplication>

SubjectListWidget::SubjectListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
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
    // COLUMNA IZQUIERDA: CONTENIDO PRINCIPAL (Tabla)
    // ==========================================
    QVBoxLayout *leftContentLayout = new QVBoxLayout();
    leftContentLayout->setSpacing(24);

    // Cabecera
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);

    QLabel *titulo = new QLabel("Gestión de Asignaturas", this);
    titulo->setObjectName("SubjectTitle");
    titulo->setStyleSheet("font-size: 24px; font-weight: 800; color: #0f172a; border: none; background: transparent;");

    QLabel *subtitulo = new QLabel("Administra las asignaturas y sus requisitos académicos", this);
    subtitulo->setObjectName("SubjectSub");
    subtitulo->setStyleSheet("font-size: 14px; color: #64748b; border: none; background: transparent;");

    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

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

    // Tabla de Asignaturas
    m_table = new QTableWidget(0, 3, this);
    m_table->setObjectName("SubjectTable");
    m_table->setHorizontalHeaderLabels({"Nombre", "Requisitos", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; gridline-color: transparent; outline: none; color: #0f172a; }"
        "QHeaderView::section { background-color: #f8fafc; font-weight: 700; color: #475569; "
        "border: none; border-bottom: 1px solid #e2e8f0; padding: 14px 20px; font-size: 13px; }"
        "QTableWidget::item { border: none; border-bottom: 1px solid #f1f5f9; padding: 10px 20px; color: #0f172a; font-size: 13px; }");

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

    leftContentLayout->addWidget(m_table);

    // ==========================================
    // COLUMNA DERECHA: PANEL LATERAL DINÁMICO
    // ==========================================
    QFrame *sidePanel = new QFrame(this);
    sidePanel->setObjectName("SubjectSidePanel");
    sidePanel->setFixedWidth(310);
    sidePanel->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");

    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(24, 28, 24, 28);
    sideLayout->setSpacing(22);

    QLabel *summaryTitle = new QLabel("Controles de Resumen", sidePanel);
    summaryTitle->setObjectName("SummaryTitle");
    summaryTitle->setStyleSheet("font-size: 15px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
    sideLayout->addWidget(summaryTitle);

    QHBoxLayout *occupancyLayout = new QHBoxLayout();
    occupancyLayout->setAlignment(Qt::AlignVCenter);

    QLabel *occLabel = new QLabel("Ocupación de\nAulas:", sidePanel);
    occLabel->setObjectName("OccLabel");
    occLabel->setStyleSheet("font-size: 13px; color: #475569; border: none; background: transparent; font-weight: 600; line-height: 1.2;");

    m_occValLabel = new QLabel("0%", sidePanel);
    m_occValLabel->setObjectName("OccVal");
    m_occValLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
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
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #e2e8f0; border-radius: 4px; border: none; }"
        "QProgressBar::chunk { background-color: #1e3a8a; border-radius: 4px; }");
    sideLayout->addWidget(m_progressBar);

    QFrame *lineDiv = new QFrame(sidePanel);
    lineDiv->setObjectName("SubjectLineDiv");
    lineDiv->setFrameShape(QFrame::HLine);
    lineDiv->setStyleSheet("color: #f1f5f9; border: none; background-color: #f1f5f9; max-height: 1px;");
    sideLayout->addWidget(lineDiv);

    QLabel *notifTitle = new QLabel("Notificaciones Recientes", sidePanel);
    notifTitle->setObjectName("NotifTitle");
    notifTitle->setStyleSheet("font-size: 15px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
    sideLayout->addWidget(notifTitle);

    // Contenedor dinámico de notificaciones
    m_notifLayout = new QVBoxLayout();
    m_notifLayout->setSpacing(8);
    sideLayout->addLayout(m_notifLayout);
    sideLayout->addStretch();

    masterLayout->addLayout(leftContentLayout, 3);
    masterLayout->addWidget(sidePanel, 1);

    connect(m_registerButton, &QPushButton::clicked, this, &SubjectListWidget::abrirFormularioNuevo);
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
        return;
    }

    for (const auto& materia : res.valor) {
        insertarMateriaEnTabla(materia.id, materia.nombre, materia.requisitos);
    }

    // Actualizar dinámicamente el panel lateral con datos reales de aulas y materias
    actualizarPanelLateral();
}

void SubjectListWidget::actualizarPanelLateral() {
    if (!m_nucleo) return;

    auto resAulas = m_nucleo->listarAulas();
    auto resMaterias = m_nucleo->listarMaterias();

    int totalAulas = resAulas.ok ? resAulas.valor.size() : 0;
    int totalMaterias = resMaterias.ok ? resMaterias.valor.size() : 0;

    int aulasOcupadasEstimadas = 0;

    // Lógica de emparejamiento inteligente (Estimación de compatibilidad)
    if (totalAulas > 0 && totalMaterias > 0) {
        // Hacemos una copia temporal de las aulas para no contar la misma dos veces
        auto aulasDisponibles = resAulas.valor;

        for (const auto& materia : resMaterias.valor) {
            QString req = materia.requisitos.toLower().trimmed();

            for (int i = 0; i < aulasDisponibles.size(); ++i) {
                QString nombreAula = aulasDisponibles[i].nombre.toLower().trimmed();

                // Si el requisito coincide con el nombre del aula (ej. "cancha" con "cancha")
                // o si es un aula general (Aula de estudio) y el aula no es especializada
                bool coincidenciaDirecta = (!req.isEmpty() && (nombreAula.contains(req) || req.contains(nombreAula)));
                bool coincidenciaGeneral = (req.contains("estudio") || req.isEmpty()) &&
                                           !nombreAula.contains("cancha") && !nombreAula.contains("lab");

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
}

void SubjectListWidget::insertarMateriaEnTabla(int id, const QString& nombre, const QString& requisitos) {
    int fila = m_table->rowCount();
    m_table->insertRow(fila);

    QTableWidgetItem *itemNombre = new QTableWidgetItem(nombre);
    QTableWidgetItem *itemReq = new QTableWidgetItem(requisitos.isEmpty() ? "Ninguno" : requisitos);

    itemNombre->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    itemReq->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_table->setItem(fila, 0, itemNombre);
    m_table->setItem(fila, 1, itemReq);

    QWidget *panelAcciones = new QWidget();
    QHBoxLayout *layoutAcciones = new QHBoxLayout(panelAcciones);
    layoutAcciones->setContentsMargins(0, 0, 0, 0);
    layoutAcciones->setSpacing(10);
    layoutAcciones->setAlignment(Qt::AlignCenter);

    QPushButton *btnEditar = new QPushButton("✏️ Editar");
    btnEditar->setFixedSize(90, 34);
    btnEditar->setStyleSheet(
        "QPushButton { color: #2563eb; background: #eff6ff; border: none; font-weight: 700; font-size: 12px; border-radius: 6px; }"
        "QPushButton:hover { background-color: #dbeafe; }");
    btnEditar->setCursor(Qt::PointingHandCursor);

    QPushButton *btnEliminar = new QPushButton("🗑️ Eliminar");
    btnEliminar->setFixedSize(100, 34);
    btnEliminar->setStyleSheet(
        "QPushButton { color: #dc2626; background: #ffeeec; border: none; font-weight: 700; font-size: 12px; border-radius: 6px; }"
        "QPushButton:hover { background-color: #fee2e2; }");
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

    connect(btnEditar, &QPushButton::clicked, this, [this, materiaId, nombre, requisitos]() {
        gestor::frontend::forms::SubjectFormDialog dialogo(this);
        dialogo.setWindowTitle("Editar Asignatura");
        dialogo.cargarDatos(nombre, requisitos);

        connect(&dialogo, &gestor::frontend::forms::SubjectFormDialog::materiaGuardada,
                this, [this, materiaId](const QString& nNombre, const QString& nRequisitos) {
                    auto resultado = m_nucleo->actualizarMateria(materiaId, nNombre, nRequisitos);
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

void SubjectListWidget::abrirFormularioNuevo() {
    if (!m_nucleo) return;

    gestor::frontend::forms::SubjectFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::SubjectFormDialog::materiaGuardada,
            this, [this](const QString& nombre, const QString& requisitos) {
                auto resultado = m_nucleo->crearMateria(nombre, requisitos);
                if (!resultado.ok) {
                    QMessageBox::warning(this, "Error", "No se pudo crear la asignatura: " + resultado.mensajeError);
                } else {
                    cargarMaterias();
                }
            });
    dialogo.exec();
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
    if (modoOscuro) {
        if (auto t = findChild<QLabel*>("SubjectTitle")) t->setStyleSheet("font-size: 24px; font-weight: 800; color: #ffffff; border: none; background: transparent;");
        if (auto s = findChild<QLabel*>("SubjectSub")) s->setStyleSheet("font-size: 14px; color: #aaaaaa; border: none; background: transparent;");
        if (auto st = findChild<QLabel*>("SummaryTitle")) st->setStyleSheet("font-size: 15px; font-weight: 800; color: #ffffff; border: none; background: transparent;");
        if (auto ol = findChild<QLabel*>("OccLabel")) ol->setStyleSheet("font-size: 13px; color: #aaaaaa; border: none; background: transparent; font-weight: 600; line-height: 1.2;");
        if (auto ov = findChild<QLabel*>("OccVal")) ov->setStyleSheet("font-size: 16px; font-weight: 800; color: #ffffff; border: none; background: transparent;");
        if (auto nt = findChild<QLabel*>("NotifTitle")) nt->setStyleSheet("font-size: 15px; font-weight: 800; color: #ffffff; border: none; background: transparent;");

        for (QLabel *lbl : findChildren<QLabel*>("NotifItemTitle")) {
            lbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #ffffff; border: none; background: transparent;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("NotifItemTime")) {
            lbl->setStyleSheet("font-size: 11px; color: #aaaaaa; border: none; background: transparent;");
        }

        if (auto panel = findChild<QFrame*>("SubjectSidePanel")) {
            panel->setStyleSheet("QFrame { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; color: white; }");
        }
        if (auto line = findChild<QFrame*>("SubjectLineDiv")) {
            line->setStyleSheet("color: #333333; border: none; background-color: #333333; max-height: 1px;");
        }
        if (auto bar = findChild<QProgressBar*>("SubjectProgressBar")) {
            bar->setStyleSheet("QProgressBar { background-color: #333333; border-radius: 4px; border: none; } QProgressBar::chunk { background-color: #3b82f6; border-radius: 4px; }");
        }

        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; gridline-color: transparent; outline: none; color: #ffffff; }"
                "QHeaderView::section { background-color: #252525; font-weight: 700; color: #ffffff; "
                "border: none; border-bottom: 1px solid #333333; padding: 14px 20px; font-size: 13px; }"
                "QTableWidget::item { border: none; border-bottom: 1px solid #2a2a2a; padding: 10px 20px; color: #ffffff; font-size: 13px; }");
        }
    } else {
        if (auto t = findChild<QLabel*>("SubjectTitle")) t->setStyleSheet("font-size: 24px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
        if (auto s = findChild<QLabel*>("SubjectSub")) s->setStyleSheet("font-size: 14px; color: #64748b; border: none; background: transparent;");
        if (auto st = findChild<QLabel*>("SummaryTitle")) st->setStyleSheet("font-size: 15px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
        if (auto ol = findChild<QLabel*>("OccLabel")) ol->setStyleSheet("font-size: 13px; color: #475569; border: none; background: transparent; font-weight: 600; line-height: 1.2;");
        if (auto ov = findChild<QLabel*>("OccVal")) ov->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
        if (auto nt = findChild<QLabel*>("NotifTitle")) nt->setStyleSheet("font-size: 15px; font-weight: 800; color: #0f172a; border: none; background: transparent;");

        for (QLabel *lbl : findChildren<QLabel*>("NotifItemTitle")) {
            lbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #1e293b; border: none; background: transparent;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("NotifItemTime")) {
            lbl->setStyleSheet("font-size: 11px; color: #64748b; border: none; background: transparent;");
        }

        if (auto panel = findChild<QFrame*>("SubjectSidePanel")) {
            panel->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
        }
        if (auto line = findChild<QFrame*>("SubjectLineDiv")) {
            line->setStyleSheet("color: #f1f5f9; border: none; background-color: #f1f5f9; max-height: 1px;");
        }
        if (auto bar = findChild<QProgressBar*>("SubjectProgressBar")) {
            bar->setStyleSheet("QProgressBar { background-color: #e2e8f0; border-radius: 4px; border: none; } QProgressBar::chunk { background-color: #1e3a8a; border-radius: 4px; }");
        }

        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; gridline-color: transparent; outline: none; color: #0f172a; }"
                "QHeaderView::section { background-color: #f8fafc; font-weight: 700; color: #475569; "
                "border: none; border-bottom: 1px solid #e2e8f0; padding: 14px 20px; font-size: 13px; }"
                "QTableWidget::item { border: none; border-bottom: 1px solid #f1f5f9; padding: 10px 20px; color: #0f172a; font-size: 13px; }");
        }
    }
}