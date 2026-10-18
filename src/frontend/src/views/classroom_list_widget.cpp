/* Proyecto:      Sistema de Gestión de Horarios
 * Archivo:         [classroom_list_widget.cpp]
 * Autor:           Paola Peña
 * Descripción:     Implementación de la interfaz gráfica (Frontend) utilizando
 *                  C++ y Qt6 para el módulo de gestión de aulas conectado a NucleoDatos.
 */

#include "classroom_list_widget.hpp"
#include "../forms/classroom_form_dialog.hpp"
#include "backend/services/NucleoDatos.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFrame>

ClassroomListWidget::ClassroomListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void ClassroomListWidget::setNucleoDatos(NucleoDatos* nucleo) {
    m_nucleo = nucleo;
    cargarAulas();
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
    titulo->setObjectName("ClassroomTitle");
    titulo->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827; border: none;");

    QLabel *subtitulo = new QLabel("Administra los espacios físicos, laboratorios y su capacidad", this);
    subtitulo->setObjectName("ClassroomSub");
    subtitulo->setStyleSheet("font-size: 14px; color: #6b7280; border: none;");

    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton("+ Agregar Aulas", this);
    m_registerButton->setObjectName("ClassroomRegBtn");
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
    m_table->setObjectName("ClassroomTable");
    m_table->setHorizontalHeaderLabels({"Nombre", "Capacidad", "Edificio", "Piso", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; color: #334155; }"
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

    contentLayout->addWidget(m_table, 7);

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
    resumenTitulo->setObjectName("ResumenTitulo");
    resumenTitulo->setStyleSheet("font-size: 16px; font-weight: bold; color: #111827; border: none;");
    resumenTitulo->setWordWrap(true);
    rightLayout->addWidget(resumenTitulo);
    rightLayout->addSpacing(10);

    auto crearTarjeta = [rightPanel](const QString& iconoTxt, const QString& texto, QLabel*& outBadge) -> QFrame* {
        QFrame *card = new QFrame(rightPanel);
        card->setObjectName("CardInstalacion");
        card->setStyleSheet("QFrame { background-color: white; border: 1px solid #e5e7eb; border-radius: 8px; }");
        card->setFixedHeight(60);

        QHBoxLayout *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(15, 0, 15, 0);

        QLabel *icono = new QLabel(iconoTxt, card);
        icono->setStyleSheet("font-size: 20px; border: none; background: transparent;");

        QLabel *nombre = new QLabel(texto, card);
        nombre->setObjectName("CardNombre");
        nombre->setStyleSheet("font-size: 14px; font-weight: bold; color: #111827; border: none; background: transparent;");

        outBadge = new QLabel("0", card);
        outBadge->setObjectName("CardBadge");
        outBadge->setAlignment(Qt::AlignCenter);
        outBadge->setFixedSize(32, 24);
        outBadge->setStyleSheet("background-color: #e5e7eb; border-radius: 12px; font-weight: bold; color: #374151; border: none; font-size: 12px;");

        cardLayout->addWidget(icono);
        cardLayout->addSpacing(10);
        cardLayout->addWidget(nombre);
        cardLayout->addStretch();
        cardLayout->addWidget(outBadge);

        return card;
    };

    rightLayout->addWidget(crearTarjeta("🏢", "Aula Teórica", m_badgeAulasTeoricas));
    rightLayout->addWidget(crearTarjeta("🔬", "Laboratorio", m_badgeLaboratorios));
    rightLayout->addWidget(crearTarjeta("🏀", "Cancha", m_badgeCanchas));

    contentLayout->addWidget(rightPanel, 3);

    mainLayout->addLayout(topSectionLayout);
    mainLayout->addLayout(contentLayout);

    connect(m_registerButton, &QPushButton::clicked, this, &ClassroomListWidget::abrirFormularioNuevo);
}

// ==========================================
// CARGAR Y LISTAR DESDE NUCLEO DATOS
// ==========================================
void ClassroomListWidget::cargarAulas() {
    if (!m_nucleo) return;

    m_table->setRowCount(0);
    auto res = m_nucleo->listarAulas();

    if (!res.ok) {
        QMessageBox::warning(this, "Error BD", "No se pudo cargar la lista de aulas.");
        return;
    }

    int contadorTeoricas = 0;
    int contadorLabs = 0;
    int contadorCanchas = 0;

    for (const auto& aula : res.valor) {
        int fila = m_table->rowCount();
        m_table->insertRow(fila);

        auto crearItemCentrado = [](const QString& texto) {
            QTableWidgetItem* item = new QTableWidgetItem(texto);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        m_table->setItem(fila, 0, crearItemCentrado(aula.nombre));
        m_table->setItem(fila, 1, crearItemCentrado(QString::number(aula.capacidad)));
        m_table->setItem(fila, 2, crearItemCentrado(aula.edificio.isEmpty() ? "N/A" : aula.edificio));
        m_table->setItem(fila, 3, crearItemCentrado(aula.piso.isEmpty() ? "N/A" : aula.piso));

        // Clasificar tipos para el resumen
        QString nombreLower = aula.nombre.toLower();
        if (nombreLower.contains("lab") || nombreLower.contains("laboratorio")) {
            contadorLabs++;
        } else if (nombreLower.contains("cancha") || nombreLower.contains("deporte")) {
            contadorCanchas++;
        } else {
            contadorTeoricas++;
        }

        // Panel de Acciones (Editar / Eliminar)
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

        int aulaId = aula.id;

        connect(btnEliminar, &QPushButton::clicked, this, [this, aulaId]() {
            if (QMessageBox::question(this, "Confirmar eliminación", "¿Estás seguro de eliminar esta aula?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                if (m_nucleo->eliminarAula(aulaId)) {
                    cargarAulas();
                } else {
                    QMessageBox::warning(this, "Error", "No se pudo eliminar el aula de la base de datos.");
                }
            }
        });

        connect(btnEditar, &QPushButton::clicked, this, [this, aulaId, aula]() {
            gestor::frontend::forms::ClassroomFormDialog dialogo(this);
            dialogo.setWindowTitle("Editar Aula");
            dialogo.cargarDatos(aula.nombre, aula.capacidad, aula.edificio, aula.piso);

            connect(&dialogo, &gestor::frontend::forms::ClassroomFormDialog::aulaGuardada,
                    this, [this, aulaId](const QString& nNombre, int nCapacidad, const QString& nEdificio, const QString& nPiso) {
                        auto resultado = m_nucleo->actualizarAula(aulaId, nNombre, nCapacidad, nEdificio, nPiso);
                        if (!resultado.ok) {
                            QMessageBox::warning(this, "Error", "No se pudo actualizar el aula: " + resultado.mensajeError);
                        } else {
                            cargarAulas();
                        }
                    });
            dialogo.exec();
        });
    }

    // Actualizar los contadores visuales del panel derecho
    if (m_badgeAulasTeoricas) m_badgeAulasTeoricas->setText(QString::number(contadorTeoricas));
    if (m_badgeLaboratorios) m_badgeLaboratorios->setText(QString::number(contadorLabs));
    if (m_badgeCanchas) m_badgeCanchas->setText(QString::number(contadorCanchas));
}

void ClassroomListWidget::abrirFormularioNuevo() {
    if (!m_nucleo) return;

    gestor::frontend::forms::ClassroomFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::ClassroomFormDialog::aulaGuardada,
            this, [this](const QString& nombre, int capacidad, const QString& edificio, const QString& piso) {
                auto resultado = m_nucleo->crearAula(nombre, capacidad, edificio, piso);
                if (!resultado.ok) {
                    QMessageBox::warning(this, "Error", "No se pudo crear el aula: " + resultado.mensajeError);
                } else {
                    cargarAulas();
                }
            });
    dialogo.exec();
}

// ==========================================
// TEMA VISUAL (Modo Oscuro / Claro)
// ==========================================
void ClassroomListWidget::actualizarTema(bool modoOscuro) {
    if (modoOscuro) {
        if (auto t = findChild<QLabel*>("ClassroomTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff; border: none;");
        if (auto s = findChild<QLabel*>("ClassroomSub")) s->setStyleSheet("font-size: 14px; color: #aaaaaa; border: none;");
        if (auto rt = findChild<QLabel*>("ResumenTitulo")) rt->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff; border: none;");

        if (auto panel = findChild<QFrame*>("ContenedorResumen")) {
            panel->setStyleSheet("QFrame#ContenedorResumen { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; color: white; }");
        }

        for (QFrame *card : findChildren<QFrame*>("CardInstalacion")) {
            card->setStyleSheet("QFrame { background-color: #252525; border: 1px solid #333333; border-radius: 8px; color: white; }");
        }
        for (QLabel *lbl : findChildren<QLabel*>("CardNombre")) {
            lbl->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; border: none; background: transparent;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("CardBadge")) {
            lbl->setStyleSheet("background-color: #333333; border-radius: 12px; font-weight: bold; color: #ffffff; border: none; font-size: 12px;");
        }

        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: #1e1e1e; border-radius: 8px; border: 1px solid #333333; color: #ffffff; }"
                "QHeaderView::section { background-color: #252525; font-weight: bold; color: #ffffff; border: none; border-bottom: 1px solid #333333; padding: 12px; }"
                "QTableWidget::item { border-bottom: 1px solid #2a2a2a; padding: 5px; color: #ffffff; }"
                );
        }
    } else {
        if (auto t = findChild<QLabel*>("ClassroomTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827; border: none;");
        if (auto s = findChild<QLabel*>("ClassroomSub")) s->setStyleSheet("font-size: 14px; color: #6b7280; border: none;");
        if (auto rt = findChild<QLabel*>("ResumenTitulo")) rt->setStyleSheet("font-size: 16px; font-weight: bold; color: #111827; border: none;");

        if (auto panel = findChild<QFrame*>("ContenedorResumen")) {
            panel->setStyleSheet("QFrame#ContenedorResumen { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");
        }

        for (QFrame *card : findChildren<QFrame*>("CardInstalacion")) {
            card->setStyleSheet("QFrame { background-color: white; border: 1px solid #e5e7eb; border-radius: 8px; }");
        }
        for (QLabel *lbl : findChildren<QLabel*>("CardNombre")) {
            lbl->setStyleSheet("font-size: 14px; font-weight: bold; color: #111827; border: none; background: transparent;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("CardBadge")) {
            lbl->setStyleSheet("background-color: #e5e7eb; border-radius: 12px; font-weight: bold; color: #374151; border: none; font-size: 12px;");
        }

        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; color: #334155; }"
                "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
                "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }"
                );
        }
    }
}