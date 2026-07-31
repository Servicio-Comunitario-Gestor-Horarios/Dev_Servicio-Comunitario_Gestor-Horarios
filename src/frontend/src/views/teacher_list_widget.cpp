#include "teacher_list_widget.hpp"
#include "../forms/teacher_form_dialog.hpp"
#include "../../../middleware/include/middleware/internalclient.h"
#include "../../../middleware/include/middleware/messages.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>

TeacherListWidget::TeacherListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void TeacherListWidget::setClient(InternalClient* client) {
    m_client = client;
    connect(m_client, &InternalClient::respuestaRecibida,
            this, &TeacherListWidget::onRespuestaRecibida);
}

void TeacherListWidget::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // Lado izquierdo: controles y tabla
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *topSectionLayout = new QHBoxLayout();
    QVBoxLayout *textLayout = new QVBoxLayout();
    QLabel *titulo = new QLabel("Gestión de Docentes", leftWidget);
    titulo->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827;");
    QLabel *subtitulo = new QLabel("Administra la información del personal académico", leftWidget);
    subtitulo->setStyleSheet("font-size: 14px; color: #6b7280;");
    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton("+ Registrar Docente", leftWidget);
    m_registerButton->setCursor(Qt::PointingHandCursor);
    m_registerButton->setStyleSheet(
        "QPushButton { background-color: #1a237e; color: white; font-weight: bold; "
        "padding: 10px 20px; border-radius: 6px; font-size: 14px; } "
        "QPushButton:hover { background-color: #283593; }");

    topSectionLayout->addLayout(textLayout);
    topSectionLayout->addStretch();
    topSectionLayout->addWidget(m_registerButton, 0, Qt::AlignVCenter);

    m_table = new QTableWidget(0, 6, leftWidget);
    m_table->setHorizontalHeaderLabels({"Cédula", "ID", "Nombre", "Email", "Teléfono", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }"
        "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; "
        "border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
        "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }");
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setDefaultSectionSize(45);

    leftLayout->addLayout(topSectionLayout);
    leftLayout->addSpacing(20);
    leftLayout->addWidget(m_table);

    // Lado derecho: panel de detalles (se mantiene igual que antes)
    QFrame *rightPanel = new QFrame(this);
    rightPanel->setMinimumWidth(320);
    rightPanel->setMaximumWidth(400);
    rightPanel->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(15);

    QLabel *detalleTitulo = new QLabel("Detalles del Docente", rightPanel);
    detalleTitulo->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827;");
    rightLayout->addWidget(detalleTitulo, 0, Qt::AlignHCenter);

    QHBoxLayout *tabsLayout = new QHBoxLayout();
    QLabel *tab1 = new QLabel("Disponibilidad\nHoraria", rightPanel);
    tab1->setAlignment(Qt::AlignCenter);
    tab1->setStyleSheet("font-weight: bold; color: #111827; border-bottom: 3px solid #111827; padding-bottom: 5px;");
    QLabel *tab2 = new QLabel("Horas\nPreferidas", rightPanel);
    tab2->setAlignment(Qt::AlignCenter);
    tab2->setStyleSheet("color: #6b7280;");
    QLabel *tab3 = new QLabel("Asignaturas", rightPanel);
    tab3->setAlignment(Qt::AlignCenter);
    tab3->setStyleSheet("color: #6b7280;");

    tabsLayout->addWidget(tab1);
    tabsLayout->addWidget(tab2);
    tabsLayout->addWidget(tab3);
    rightLayout->addLayout(tabsLayout);

    QTableWidget *grid = new QTableWidget(12, 5, rightPanel);
    grid->setHorizontalHeaderLabels({"Lunes", "Martes", "Mierc.", "Jueves", "Viernes"});
    grid->setVerticalHeaderLabels({"7am", "8am", "9am", "10am", "11am", "12pm", "1pm", "2pm", "3pm", "4pm", "5pm", "6pm"});
    grid->setStyleSheet(
        "QTableWidget { border: none; gridline-color: white; }"
        "QHeaderView::section { background-color: white; border: none; font-size: 10px; color: #6b7280; }");
    grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    grid->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    grid->setSelectionMode(QAbstractItemView::NoSelection);
    grid->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int r = 0; r < 12; ++r) {
        for (int c = 0; c < 5; ++c) {
            QTableWidgetItem *item = new QTableWidgetItem();
            if ((r >= 5 && r <= 10 && c == 1) || (r >= 5 && r <= 7 && c == 4) || (r >= 7 && r <= 8 && c == 2)) {
                item->setBackground(QColor(254, 202, 202));
                item->setText("Not\nAvailable");
                item->setFont(QFont("Arial", 6));
                item->setTextAlignment(Qt::AlignCenter);
                item->setForeground(QColor(153, 27, 27));
            } else {
                item->setBackground(QColor(167, 243, 208));
            }
            grid->setItem(r, c, item);
        }
    }
    rightLayout->addWidget(grid);

    QHBoxLayout *legendLayout = new QHBoxLayout();
    QLabel *legendGreen = new QLabel(rightPanel);
    legendGreen->setFixedSize(16, 10);
    legendGreen->setStyleSheet("background-color: #a7f3d0; border-radius: 2px;");
    QLabel *legendGreenTxt = new QLabel("Available", rightPanel);
    legendGreenTxt->setStyleSheet("color: #374151; font-size: 11px;");
    QLabel *legendRed = new QLabel(rightPanel);
    legendRed->setFixedSize(16, 10);
    legendRed->setStyleSheet("background-color: #fecaca; border-radius: 2px;");
    QLabel *legendRedTxt = new QLabel("Not Available", rightPanel);
    legendRedTxt->setStyleSheet("color: #374151; font-size: 11px;");

    legendLayout->addStretch();
    legendLayout->addWidget(legendGreen);
    legendLayout->addWidget(legendGreenTxt);
    legendLayout->addSpacing(15);
    legendLayout->addWidget(legendRed);
    legendLayout->addWidget(legendRedTxt);
    legendLayout->addStretch();
    rightLayout->addLayout(legendLayout);

    QPushButton *manageBtn = new QPushButton("Gestionar Disponibilidad", rightPanel);
    manageBtn->setStyleSheet("background-color: #111827; color: white; padding: 12px; border-radius: 6px; font-weight: bold;");
    rightLayout->addWidget(manageBtn);

    mainLayout->addWidget(leftWidget, 7);
    mainLayout->addWidget(rightPanel, 3);

    connect(m_registerButton, &QPushButton::clicked, this, &TeacherListWidget::abrirFormularioNuevo);
}

void TeacherListWidget::onRespuestaRecibida(const QJsonObject& respuesta) {
    QString op = respuesta["op"].toString();
    if (op != m_pendiente.op) return;

    if (respuesta["status"].toString() == "ok") {
        QJsonObject data = m_pendiente.data;
        if (op == Middleware::OP_CREAR_PROFESOR) {
            insertarProfesorEnTabla(data["cedula"].toString(),
                                    data["id_interno"].toString(),
                                    data["nombre"].toString(),
                                    data["email"].toString(),
                                    data["telefono"].toString());
        } else if (op == Middleware::OP_ACTUALIZAR_PROFESOR) {
            int f = m_pendiente.fila;
            m_table->item(f, 0)->setText(data["cedula"].toString());
            m_table->item(f, 1)->setText(data["id_interno"].toString());
            m_table->item(f, 2)->setText(data["nombre"].toString());
            m_table->item(f, 3)->setText(data["email"].toString());
            m_table->item(f, 4)->setText(data["telefono"].toString().isEmpty() ? "N/A" : data["telefono"].toString());
        } else if (op == Middleware::OP_ELIMINAR_PROFESOR) {
            m_table->removeRow(m_pendiente.fila);
        }
    } else {
        QMessageBox::warning(this, "Error", "No se pudo completar la operación: " + respuesta["data"].toString());
    }
    m_pendiente = SolicitudPendiente();
}

void TeacherListWidget::enviarCrearProfesor(const QString& cedula, const QString& id,
                                            const QString& nombre, const QString& email,
                                            const QString& telefono) {
    if (!m_client) return;
    QJsonObject payload;
    payload["cedula"] = cedula;
    payload["id_interno"] = id;
    payload["nombre"] = nombre;
    payload["email"] = email;
    payload["telefono"] = telefono;
    m_pendiente = {Middleware::OP_CREAR_PROFESOR, payload, -1};
    m_client->enviarSolicitud(Middleware::OP_CREAR_PROFESOR, payload);
}

void TeacherListWidget::enviarActualizarProfesor(int fila, const QString& cedula,
                                                 const QString& id, const QString& nombre,
                                                 const QString& email, const QString& telefono) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = cedula;          // identificador para el middleware
    payload["cedula"] = cedula;
    payload["id_interno"] = id;
    payload["nombre"] = nombre;
    payload["email"] = email;
    payload["telefono"] = telefono;
    m_pendiente = {Middleware::OP_ACTUALIZAR_PROFESOR, payload, fila};
    m_client->enviarSolicitud(Middleware::OP_ACTUALIZAR_PROFESOR, payload);
}

void TeacherListWidget::enviarEliminarProfesor(int fila, const QString& cedula) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = cedula;
    m_pendiente = {Middleware::OP_ELIMINAR_PROFESOR, payload, fila};
    m_client->enviarSolicitud(Middleware::OP_ELIMINAR_PROFESOR, payload);
}

void TeacherListWidget::insertarProfesorEnTabla(const QString& cedula, const QString& id,
                                                const QString& nombre, const QString& email,
                                                const QString& telefono) {
    int fila = m_table->rowCount();
    m_table->insertRow(fila);

    m_table->setItem(fila, 0, new QTableWidgetItem(cedula));
    m_table->setItem(fila, 1, new QTableWidgetItem(id));
    m_table->setItem(fila, 2, new QTableWidgetItem(nombre));
    m_table->setItem(fila, 3, new QTableWidgetItem(email));
    m_table->setItem(fila, 4, new QTableWidgetItem(telefono.isEmpty() ? "N/A" : telefono));

    // Acciones
    QWidget *panelAcciones = new QWidget();
    QHBoxLayout *layoutAcciones = new QHBoxLayout(panelAcciones);
    layoutAcciones->setContentsMargins(0, 0, 0, 0);
    layoutAcciones->setSpacing(10);

    QPushButton *btnEditar = new QPushButton("✏️");
    btnEditar->setStyleSheet("color: #2563eb; background: transparent; border: none; font-weight: bold;");
    btnEditar->setCursor(Qt::PointingHandCursor);

    QPushButton *btnEliminar = new QPushButton("🗑️");
    btnEliminar->setStyleSheet("color: #dc2626; background: transparent; border: none; font-weight: bold;");
    btnEliminar->setCursor(Qt::PointingHandCursor);

    layoutAcciones->addStretch();
    layoutAcciones->addWidget(btnEditar);
    layoutAcciones->addWidget(btnEliminar);
    layoutAcciones->addStretch();

    connect(btnEliminar, &QPushButton::clicked, this, [this, panelAcciones]() {
        if (QMessageBox::question(this, "Confirmar eliminación",
                                  "¿Está seguro de eliminar este docente?") == QMessageBox::Yes) {
            for (int i = 0; i < m_table->rowCount(); ++i) {
                if (m_table->cellWidget(i, 5) == panelAcciones) {
                    QString cedula = m_table->item(i, 0)->text();
                    enviarEliminarProfesor(i, cedula);
                    break;
                }
            }
        }
    });

    connect(btnEditar, &QPushButton::clicked, this, [this, panelAcciones]() {
        int filaEditar = -1;
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (m_table->cellWidget(i, 5) == panelAcciones) {
                filaEditar = i;
                break;
            }
        }
        if (filaEditar == -1) return;

        QString cedula = m_table->item(filaEditar, 0)->text();
        QString id = m_table->item(filaEditar, 1)->text();
        QString nombre = m_table->item(filaEditar, 2)->text();
        QString email = m_table->item(filaEditar, 3)->text();
        QString telefono = m_table->item(filaEditar, 4)->text();

        gestor::frontend::forms::TeacherFormDialog dialogo(this);
        dialogo.setWindowTitle("Editar Docente");
        dialogo.cargarDatos(cedula, id, nombre, email, telefono);

        connect(&dialogo, &gestor::frontend::forms::TeacherFormDialog::profesorGuardado,
                this, [this, filaEditar](const QString& nCedula, const QString& nId,
                                   const QString& nNombre, const QString& nEmail,
                                   const QString& nTelefono) {
                    enviarActualizarProfesor(filaEditar, nCedula, nId, nNombre, nEmail, nTelefono);
                });
        dialogo.exec();
    });

    m_table->setCellWidget(fila, 5, panelAcciones);
}

void TeacherListWidget::abrirFormularioNuevo() {
    gestor::frontend::forms::TeacherFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::TeacherFormDialog::profesorGuardado,
            this, [this](const QString& cedula, const QString& id,
                   const QString& nombre, const QString& email,
                   const QString& telefono) {
                enviarCrearProfesor(cedula, id, nombre, email, telefono);
            });
    dialogo.exec();
}