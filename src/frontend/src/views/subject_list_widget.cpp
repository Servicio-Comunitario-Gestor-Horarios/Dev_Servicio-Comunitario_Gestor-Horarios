/**
 * @file subject_list_widget.cpp
 * @brief Implementación del widget de gestión de asignaturas con cabecera de acciones centrada
 */

#include "subject_list_widget.hpp"
#include "../forms/subject_form_dialog.hpp"
#include "../../../middleware/include/middleware/internalclient.h"
#include "../../../middleware/include/middleware/messages.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFrame>
#include <QProgressBar>

SubjectListWidget::SubjectListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void SubjectListWidget::setClient(InternalClient* client) {
    m_client = client;
    connect(m_client, &InternalClient::respuestaRecibida,
            this, &SubjectListWidget::onRespuestaRecibida);
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
    titulo->setStyleSheet("font-size: 24px; font-weight: 800; color: #0f172a; border: none; background: transparent;");

    QLabel *subtitulo = new QLabel("Administra las asignaturas y su tipo de aula requerido", this);
    subtitulo->setStyleSheet("font-size: 14px; color: #64748b; border: none; background: transparent;");

    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton("+ Registrar Asignatura", this);
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
    m_table->setHorizontalHeaderLabels({"Nombre", "Tipo de aula", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; gridline-color: transparent; outline: none; }"
        "QHeaderView::section { background-color: #f8fafc; font-weight: 700; color: #475569; "
        "border: none; border-bottom: 1px solid #e2e8f0; padding: 14px 20px; font-size: 13px; }"
        "QTableWidget::item { border: none; border-bottom: 1px solid #f1f5f9; padding: 10px 20px; color: #0f172a; font-size: 13px; }");

    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(2, 260);

    // Alineación inteligente: Nombre y Tipo de aula a la izquierda, Acciones centrada
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Forzamos específicamente que el título de la columna 3 ("Acciones") quede centrado sobre los botones
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
    // COLUMNA DERECHA: PANEL LATERAL DE RESUMEN
    // ==========================================
    QFrame *sidePanel = new QFrame(this);
    sidePanel->setFixedWidth(310);
    sidePanel->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");

    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(24, 28, 24, 28);
    sideLayout->setSpacing(22);

    QLabel *summaryTitle = new QLabel("Controles de Resumen", sidePanel);
    summaryTitle->setStyleSheet("font-size: 15px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
    sideLayout->addWidget(summaryTitle);

    QHBoxLayout *occupancyLayout = new QHBoxLayout();
    occupancyLayout->setAlignment(Qt::AlignVCenter);

    QLabel *occLabel = new QLabel("Ocupación de\nAulas:", sidePanel);
    occLabel->setStyleSheet("font-size: 13px; color: #475569; border: none; background: transparent; font-weight: 600; line-height: 1.2;");

    QLabel *occVal = new QLabel("65%", sidePanel);
    occVal->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
    occVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    occupancyLayout->addWidget(occLabel);
    occupancyLayout->addStretch();
    occupancyLayout->addWidget(occVal);
    sideLayout->addLayout(occupancyLayout);

    QProgressBar *progressBar = new QProgressBar(sidePanel);
    progressBar->setValue(65);
    progressBar->setFixedHeight(8);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { background-color: #e2e8f0; border-radius: 4px; border: none; }"
        "QProgressBar::chunk { background-color: #1e3a8a; border-radius: 4px; }");
    sideLayout->addWidget(progressBar);

    QFrame *lineDiv = new QFrame(sidePanel);
    lineDiv->setFrameShape(QFrame::HLine);
    lineDiv->setStyleSheet("color: #f1f5f9; border: none; background-color: #f1f5f9; max-height: 1px;");
    sideLayout->addWidget(lineDiv);

    QLabel *notifTitle = new QLabel("Notificaciones Recientes", sidePanel);
    notifTitle->setStyleSheet("font-size: 15px; font-weight: 800; color: #0f172a; border: none; background: transparent;");
    sideLayout->addWidget(notifTitle);

    auto createNotifItem = [sidePanel](const QString& titleText, const QString& timeText) -> QWidget* {
        QWidget *itemWidget = new QWidget(sidePanel);
        itemWidget->setStyleSheet("background: transparent; border: none;");
        QVBoxLayout *itemLayout = new QVBoxLayout(itemWidget);
        itemLayout->setContentsMargins(0, 6, 0, 6);
        itemLayout->setSpacing(3);

        QLabel *lblTitle = new QLabel(titleText, itemWidget);
        lblTitle->setWordWrap(true);
        lblTitle->setStyleSheet("font-size: 12px; font-weight: 700; color: #1e293b; border: none; background: transparent;");

        QLabel *lblTime = new QLabel(timeText, itemWidget);
        lblTime->setStyleSheet("font-size: 11px; color: #64748b; border: none; background: transparent;");

        itemLayout->addWidget(lblTitle);
        itemLayout->addWidget(lblTime);
        return itemWidget;
    };

    sideLayout->addWidget(createNotifItem("Aula 201: Matemáticas en curso", "7 hours ago"));
    sideLayout->addWidget(createNotifItem("Nuevo registro: Historia", "2 hours ago"));
    sideLayout->addWidget(createNotifItem("Mantenimiento en Lab-3", "2 hours ago"));
    sideLayout->addStretch();

    masterLayout->addLayout(leftContentLayout, 3);
    masterLayout->addWidget(sidePanel, 1);

    connect(m_registerButton, &QPushButton::clicked, this, &SubjectListWidget::abrirFormularioNuevo);
}

void SubjectListWidget::onRespuestaRecibida(const QJsonObject& respuesta) {
    QString op = respuesta["op"].toString();
    if (op != m_pendiente.op) return;

    if (respuesta["status"].toString() == "ok") {
        QJsonObject data = m_pendiente.data;
        if (op == Middleware::OP_CREAR_MATERIA) {
            insertarMateriaEnTabla(data["nombre"].toString(), data["tipoAula"].toString());
        } else if (op == Middleware::OP_ACTUALIZAR_MATERIA) {
            int f = m_pendiente.fila;
            m_table->item(f, 0)->setText(data["nombre"].toString());
            m_table->item(f, 1)->setText(data["tipoAula"].toString());
        } else if (op == Middleware::OP_ELIMINAR_MATERIA) {
            m_table->removeRow(m_pendiente.fila);
        }
    } else {
        QMessageBox::warning(this, "Error", "No se pudo completar la operación: " + respuesta["data"].toString());
    }
    m_pendiente = SolicitudPendiente();
}

void SubjectListWidget::enviarCrearMateria(const QString& nombre, const QString& tipoAula) {
    if (!m_client) return;
    QJsonObject payload;
    payload["nombre"] = nombre;
    payload["tipoAula"] = tipoAula;
    m_pendiente = {Middleware::OP_CREAR_MATERIA, payload, -1};
    m_client->enviarSolicitud(Middleware::OP_CREAR_MATERIA, payload);
}

void SubjectListWidget::enviarActualizarMateria(int fila, const QString& nombreOriginal,
                                                const QString& nombre, const QString& tipoAula) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = nombreOriginal;
    payload["nombre"] = nombre;
    payload["tipoAula"] = tipoAula;
    m_pendiente = {Middleware::OP_ACTUALIZAR_MATERIA, payload, fila};
    m_client->enviarSolicitud(Middleware::OP_ACTUALIZAR_MATERIA, payload);
}

void SubjectListWidget::enviarEliminarMateria(int fila, const QString& nombre) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = nombre;
    m_pendiente = {Middleware::OP_ELIMINAR_MATERIA, payload, fila};
    m_client->enviarSolicitud(Middleware::OP_ELIMINAR_MATERIA, payload);
}

void SubjectListWidget::insertarMateriaEnTabla(const QString& nombre, const QString& tipoAula) {
    int fila = m_table->rowCount();
    m_table->insertRow(fila);

    QTableWidgetItem *itemNombre = new QTableWidgetItem(nombre);
    QTableWidgetItem *itemTipo = new QTableWidgetItem(tipoAula);

    itemNombre->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    itemTipo->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_table->setItem(fila, 0, itemNombre);
    m_table->setItem(fila, 1, itemTipo);

    // Contenedor de Acciones perfectamente centrado
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

    // Lógica de Eliminación
    connect(btnEliminar, &QPushButton::clicked, this, [this, panelAcciones]() {
        if (QMessageBox::question(this, "Confirmar eliminación",
                                  "¿Está seguro de eliminar esta asignatura?") == QMessageBox::Yes) {
            for (int i = 0; i < m_table->rowCount(); ++i) {
                if (m_table->cellWidget(i, 2) == panelAcciones) {
                    QString nombre = m_table->item(i, 0)->text();
                    enviarEliminarMateria(i, nombre);
                    break;
                }
            }
        }
    });

    // Lógica de Edición
    connect(btnEditar, &QPushButton::clicked, this, [this, panelAcciones]() {
        int filaEditar = -1;
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (m_table->cellWidget(i, 2) == panelAcciones) {
                filaEditar = i;
                break;
            }
        }
        if (filaEditar == -1) return;

        QString nombre = m_table->item(filaEditar, 0)->text();
        QString tipo = m_table->item(filaEditar, 1)->text();

        gestor::frontend::forms::SubjectFormDialog dialogo(this);
        dialogo.setWindowTitle("Editar Asignatura");
        dialogo.cargarDatos(nombre, tipo);

        connect(&dialogo, &gestor::frontend::forms::SubjectFormDialog::materiaGuardada,
                this, [this, filaEditar, nombre](const QString& nNombre, const QString& nTipo) {
                    enviarActualizarMateria(filaEditar, nombre, nNombre, nTipo);
                });
        dialogo.exec();
    });

    m_table->setCellWidget(fila, 2, panelAcciones);
}

void SubjectListWidget::abrirFormularioNuevo() {
    gestor::frontend::forms::SubjectFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::SubjectFormDialog::materiaGuardada,
            this, [this](const QString& nombre, const QString& tipoAula) {
                enviarCrearMateria(nombre, tipoAula);
            });
    dialogo.exec();
}