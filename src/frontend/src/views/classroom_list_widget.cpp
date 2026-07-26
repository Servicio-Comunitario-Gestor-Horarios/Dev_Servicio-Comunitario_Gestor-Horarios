/* Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        [classroom_list_widget.cpp]
 * Autor:          Paola Peña
 * Fecha:          19 de Julio de 2026
 * Descripción:    Implementación de la interfaz gráfica (Frontend) utilizando
 *                 C++ y Qt6 para el módulo de gestión de aulas. Incluye el
 *                 diseño de la vista principal, la tabla de listado, botones CRUD
 *                 y el panel lateral de resumen de instalaciones.
 */

#include "classroom_list_widget.hpp"
#include "../forms/classroom_form_dialog.hpp"
#include <middleware/internalclient.h>
#include <middleware/messages.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFrame>

ClassroomListWidget::ClassroomListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void ClassroomListWidget::setClient(InternalClient* client) {
    m_client = client;
    connect(m_client, &InternalClient::respuestaRecibida,
            this, &ClassroomListWidget::onRespuestaRecibida);
}

void ClassroomListWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // ==========================================
    // TOP SECTION: Títulos y Botón
    // ==========================================
    QHBoxLayout *topSectionLayout = new QHBoxLayout();
    QVBoxLayout *textLayout = new QVBoxLayout();
    QLabel *titulo = new QLabel("Gestión de Aulas", this);
    titulo->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827; border: none;");
    QLabel *subtitulo = new QLabel("Administra los espacios físicos, laboratorios y su capacidad", this);
    subtitulo->setStyleSheet("font-size: 14px; color: #6b7280; border: none;");

    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton("+ Agregar Aulas", this);
    m_registerButton->setCursor(Qt::PointingHandCursor);
    m_registerButton->setStyleSheet("QPushButton { background-color: #1a237e; color: white; font-weight: bold; padding: 10px 20px; border-radius: 6px; font-size: 14px; } QPushButton:hover { background-color: #283593; }");

    topSectionLayout->addLayout(textLayout);
    topSectionLayout->addStretch();
    topSectionLayout->addWidget(m_registerButton, 0, Qt::AlignVCenter);

    // ==========================================
    // BOTTOM SECTION: Tabla (Izquierda) y Resumen (Derecha)
    // ==========================================
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(25);

    // 1. Configuración de la Tabla (Lado Izquierdo)
    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({"Nombre", "Capacidad", "Edificio", "Piso", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }"
        "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
        "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }"
        );
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(4, 180);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setDefaultSectionSize(50);

    contentLayout->addWidget(m_table, 7); // La tabla toma el 70% del espacio

    // 2. Configuración del Panel de Resumen (Lado Derecho)
    QFrame *rightPanel = new QFrame(this);
    rightPanel->setFixedWidth(320);
    rightPanel->setStyleSheet("QFrame#ContenedorResumen { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");
    rightPanel->setObjectName("ContenedorResumen");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(25, 25, 25, 25);
    rightLayout->setSpacing(15);
    rightLayout->setAlignment(Qt::AlignTop);

    QLabel *resumenTitulo = new QLabel("Resumen Rápido de Instalaciones", rightPanel);
    resumenTitulo->setStyleSheet("font-size: 16px; font-weight: bold; color: #111827; border: none;");
    resumenTitulo->setWordWrap(true);
    rightLayout->addWidget(resumenTitulo);
    rightLayout->addSpacing(10);

    // Función lambda para generar las tarjetas visuales rápidamente
    auto crearTarjeta = [rightPanel](const QString& iconoTxt, const QString& texto, const QString& cantidad) -> QFrame* {
        QFrame *card = new QFrame(rightPanel);
        card->setStyleSheet("QFrame { background-color: white; border: 1px solid #e5e7eb; border-radius: 8px; }");
        card->setFixedHeight(60);

        QHBoxLayout *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(15, 0, 15, 0);

        QLabel *icono = new QLabel(iconoTxt, card);
        icono->setStyleSheet("font-size: 20px; border: none;");

        QLabel *nombre = new QLabel(texto, card);
        nombre->setStyleSheet("font-size: 14px; font-weight: bold; color: #111827; border: none;");

        QLabel *badge = new QLabel(cantidad, card);
        badge->setAlignment(Qt::AlignCenter);
        badge->setFixedSize(32, 24);
        badge->setStyleSheet("background-color: #e5e7eb; border-radius: 12px; font-weight: bold; color: #374151; border: none; font-size: 12px;");

        cardLayout->addWidget(icono);
        cardLayout->addSpacing(10);
        cardLayout->addWidget(nombre);
        cardLayout->addStretch();
        cardLayout->addWidget(badge);

        return card;
    };

    // Agregar las tarjetas al panel
    rightLayout->addWidget(crearTarjeta("🏢", "Aula Teórica", "12"));
    rightLayout->addWidget(crearTarjeta("🔬", "Laboratorio", "4"));
    rightLayout->addWidget(crearTarjeta("🏀", "Cancha", "2"));

    contentLayout->addWidget(rightPanel, 3); // El panel toma el 30% del espacio

    // Añadir todo al layout principal
    mainLayout->addLayout(topSectionLayout);
    mainLayout->addLayout(contentLayout);

    connect(m_registerButton, &QPushButton::clicked, this, &ClassroomListWidget::abrirFormularioNuevo);
}

void ClassroomListWidget::onRespuestaRecibida(const QJsonObject& respuesta) {
    QString op = respuesta["op"].toString();
    if (op != m_pendiente.op) return;

    if (respuesta["status"].toString() == "ok") {
        QJsonObject data = m_pendiente.data;
        if (op == Middleware::OP_CREAR_AULA) {
            insertarAulaEnTabla(data["nombre"].toString(),
                                data["capacidad"].toInt(),
                                data["edificio"].toString(),
                                data["piso"].toString());
        } else if (op == Middleware::OP_ACTUALIZAR_AULA) {
            int f = m_pendiente.fila;
            m_table->item(f, 0)->setText(data["nombre"].toString());
            m_table->item(f, 1)->setText(QString::number(data["capacidad"].toInt()));
            m_table->item(f, 2)->setText(data["edificio"].toString().isEmpty() ? "N/A" : data["edificio"].toString());
            m_table->item(f, 3)->setText(data["piso"].toString().isEmpty() ? "N/A" : data["piso"].toString());
        } else if (op == Middleware::OP_ELIMINAR_AULA) {
            m_table->removeRow(m_pendiente.fila);
        }
    } else {
        QMessageBox::warning(this, "Error", "No se pudo completar la operación: " + respuesta["data"].toString());
    }
    m_pendiente = SolicitudPendiente();
}

void ClassroomListWidget::enviarCrearAula(const QString& nombre, int capacidad,
                                          const QString& edificio, const QString& piso) {
    if (!m_client) return;
    QJsonObject payload;
    payload["nombre"] = nombre;
    payload["capacidad"] = capacidad;
    payload["edificio"] = edificio;
    payload["piso"] = piso;
    m_pendiente = {Middleware::OP_CREAR_AULA, payload, -1};
    m_client->enviarSolicitud(Middleware::OP_CREAR_AULA, payload);
}

void ClassroomListWidget::enviarActualizarAula(int fila, const QString& nombreOriginal,
                                               const QString& nombre, int capacidad,
                                               const QString& edificio, const QString& piso) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = nombreOriginal;
    payload["nombre"] = nombre;
    payload["capacidad"] = capacidad;
    payload["edificio"] = edificio;
    payload["piso"] = piso;
    m_pendiente = {Middleware::OP_ACTUALIZAR_AULA, payload, fila};
    m_client->enviarSolicitud(Middleware::OP_ACTUALIZAR_AULA, payload);
}

void ClassroomListWidget::enviarEliminarAula(int fila, const QString& nombre) {
    if (!m_client) return;
    QJsonObject payload;
    payload["id"] = nombre;
    m_pendiente = {Middleware::OP_ELIMINAR_AULA, payload, fila};
    m_client->enviarSolicitud(Middleware::OP_ELIMINAR_AULA, payload);
}

void ClassroomListWidget::insertarAulaEnTabla(const QString& nombre, int capacidad,
                                              const QString& edificio, const QString& piso) {
    int fila = m_table->rowCount();
    m_table->insertRow(fila);

    QTableWidgetItem *itemNombre = new QTableWidgetItem(nombre);
    itemNombre->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 0, itemNombre);

    QTableWidgetItem *itemCapacidad = new QTableWidgetItem(QString::number(capacidad));
    itemCapacidad->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 1, itemCapacidad);

    QTableWidgetItem *itemEdificio = new QTableWidgetItem(edificio.isEmpty() ? "N/A" : edificio);
    itemEdificio->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 2, itemEdificio);

    QTableWidgetItem *itemPiso = new QTableWidgetItem(piso.isEmpty() ? "N/A" : piso);
    itemPiso->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 3, itemPiso);

    QWidget *panelAcciones = new QWidget();
    QHBoxLayout *layoutAcciones = new QHBoxLayout(panelAcciones);
    layoutAcciones->setContentsMargins(0, 0, 0, 0);
    layoutAcciones->setSpacing(10);

    QPushButton *btnEditar = new QPushButton("✏️ Editar");
    btnEditar->setStyleSheet("color: #2563eb; background: transparent; border: none; font-weight: bold;");
    btnEditar->setCursor(Qt::PointingHandCursor);

    QPushButton *btnEliminar = new QPushButton("🗑️ Eliminar");
    btnEliminar->setStyleSheet("color: #dc2626; background: transparent; border: none; font-weight: bold;");
    btnEliminar->setCursor(Qt::PointingHandCursor);

    layoutAcciones->addStretch();
    layoutAcciones->addWidget(btnEditar);
    layoutAcciones->addWidget(btnEliminar);
    layoutAcciones->addStretch();

    m_table->setCellWidget(fila, 4, panelAcciones);

    // Conectar acción Eliminar
    connect(btnEliminar, &QPushButton::clicked, this, [this, panelAcciones]() {
        if (QMessageBox::question(this, "Confirmar eliminación", "¿Estás seguro de eliminar esta aula?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            for (int i = 0; i < m_table->rowCount(); ++i) {
                if (m_table->cellWidget(i, 4) == panelAcciones) {
                    QString nombre = m_table->item(i, 0)->text();
                    enviarEliminarAula(i, nombre);
                    break;
                }
            }
        }
    });

    // Conectar acción Editar
    connect(btnEditar, &QPushButton::clicked, this, [this, panelAcciones]() {
        int filaEditar = -1;
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (m_table->cellWidget(i, 4) == panelAcciones) {
                filaEditar = i;
                break;
            }
        }
        if (filaEditar != -1) {
            QString vNombre = m_table->item(filaEditar, 0)->text();
            int vCapacidad = m_table->item(filaEditar, 1)->text().toInt();
            QString vEdificio = m_table->item(filaEditar, 2)->text();
            QString vPiso = m_table->item(filaEditar, 3)->text();

            gestor::frontend::forms::ClassroomFormDialog dialogo(this);
            dialogo.setWindowTitle("Editar Aula");
            dialogo.cargarDatos(vNombre, vCapacidad, vEdificio, vPiso);

            connect(&dialogo, &gestor::frontend::forms::ClassroomFormDialog::aulaGuardada,
                    this, [this, filaEditar, vNombre](const QString& nNombre, int nCapacidad, const QString& nEdificio, const QString& nPiso) {
                        enviarActualizarAula(filaEditar, vNombre, nNombre, nCapacidad, nEdificio, nPiso);
                    });
            dialogo.exec();
        }
    });
}

void ClassroomListWidget::abrirFormularioNuevo() {
    gestor::frontend::forms::ClassroomFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::ClassroomFormDialog::aulaGuardada,
            this, [this](const QString& nombre, int capacidad, const QString& edificio, const QString& piso) {
                enviarCrearAula(nombre, capacidad, edificio, piso);
            });
    dialogo.exec();
}