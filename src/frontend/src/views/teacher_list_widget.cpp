/**
 * @file teacher_list_widget.cpp
 * @brief Implementación del listado de docentes conectado a SQLite
 */

#include "teacher_list_widget.hpp"
#include "../forms/teacher_form_dialog.hpp"
#include "backend/services/NucleoDatos.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>

TeacherListWidget::TeacherListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void TeacherListWidget::setClient(InternalClient* client) {
    m_client = client;
}

void TeacherListWidget::setNucleoDatos(NucleoDatos* nucleo) {
    m_nucleo = nucleo;
    cargarDocentes();
}

void TeacherListWidget::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // ==========================================
    // LADO IZQUIERDO: CONTROLES Y TABLA
    // ==========================================
    QWidget *leftWidget = new QWidget(this);
    leftWidget->setObjectName("TeacherLeftWidget");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *topSectionLayout = new QHBoxLayout();
    QVBoxLayout *textLayout = new QVBoxLayout();

    QLabel *titulo = new QLabel("Gestión de Docentes", leftWidget);
    titulo->setObjectName("TeacherTitle");
    titulo->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827;");

    QLabel *subtitulo = new QLabel("Administra la información, disponibilidad y materias del personal académico", leftWidget);
    subtitulo->setObjectName("TeacherSub");
    subtitulo->setStyleSheet("font-size: 14px; color: #6b7280;");

    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton("+ Registrar Docente", leftWidget);
    m_registerButton->setObjectName("TeacherRegBtn");
    m_registerButton->setCursor(Qt::PointingHandCursor);
    m_registerButton->setStyleSheet(
        "QPushButton { background-color: #1a237e; color: white; font-weight: bold; "
        "padding: 10px 20px; border-radius: 6px; font-size: 14px; } "
        "QPushButton:hover { background-color: #283593; }");

    topSectionLayout->addLayout(textLayout);
    topSectionLayout->addStretch();
    topSectionLayout->addWidget(m_registerButton, 0, Qt::AlignVCenter);

    // Tabla de 6 columnas
    m_table = new QTableWidget(0, 6, leftWidget);
    m_table->setObjectName("TeacherTable");
    m_table->setHorizontalHeaderLabels({"Cédula/ID", "Nombre", "Email", "Teléfono", "Materias", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; color: #334155; }"
        "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; "
        "border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
        "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }");

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

    // ==========================================
    // LADO DERECHO: PANEL DE DETALLES
    // ==========================================
    QFrame *rightPanel = new QFrame(this);
    rightPanel->setObjectName("TeacherRightPanel");
    rightPanel->setMinimumWidth(400); // <-- Ancho aumentado para que no se corten las letras
    rightPanel->setMaximumWidth(480);
    rightPanel->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(15);

    m_detalleNombre = new QLabel("Detalles del Docente", rightPanel);
    m_detalleNombre->setObjectName("DetailTitle");
    m_detalleNombre->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827;");
    rightLayout->addWidget(m_detalleNombre, 0, Qt::AlignHCenter);

    QHBoxLayout *tabsLayout = new QHBoxLayout();
    QLabel *tab1 = new QLabel("Disponibilidad\nHoraria", rightPanel);
    tab1->setObjectName("TabActive");
    tab1->setAlignment(Qt::AlignCenter);
    tab1->setStyleSheet("font-weight: bold; color: #111827; border-bottom: 3px solid #111827; padding-bottom: 5px;");

    QLabel *tab2 = new QLabel("Horas\nPreferidas", rightPanel);
    tab2->setObjectName("TabInact1");
    tab2->setAlignment(Qt::AlignCenter);
    tab2->setStyleSheet("color: #6b7280;");

    QLabel *tab3 = new QLabel("Asignaturas", rightPanel);
    tab3->setObjectName("TabInact2");
    tab3->setAlignment(Qt::AlignCenter);
    tab3->setStyleSheet("color: #6b7280;");

    tabsLayout->addWidget(tab1);
    tabsLayout->addWidget(tab2);
    tabsLayout->addWidget(tab3);
    rightLayout->addLayout(tabsLayout);

    m_gridDisponibilidad = new QTableWidget(12, 5, rightPanel);
    m_gridDisponibilidad->setObjectName("AvailabilityGrid");
    m_gridDisponibilidad->setHorizontalHeaderLabels({"Lunes", "Martes", "Mierc.", "Jueves", "Viernes"});
    m_gridDisponibilidad->setVerticalHeaderLabels({"7am", "8am", "9am", "10am", "11am", "12pm", "1pm", "2pm", "3pm", "4pm", "5pm", "6pm"});
    m_gridDisponibilidad->setStyleSheet(
        "QTableWidget { border: none; gridline-color: white; background-color: transparent; }"
        "QHeaderView::section { background-color: transparent; border: none; font-size: 11px; color: #6b7280; padding: 2px; }");

    // Obligar a los encabezados a tener un tamaño mínimo para no aplastar el texto
    m_gridDisponibilidad->horizontalHeader()->setMinimumHeight(35);
    m_gridDisponibilidad->verticalHeader()->setMinimumWidth(45);
    m_gridDisponibilidad->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_gridDisponibilidad->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_gridDisponibilidad->setSelectionMode(QAbstractItemView::NoSelection);
    m_gridDisponibilidad->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Dibujar grid por defecto
    for (int r = 0; r < 12; ++r) {
        for (int c = 0; c < 5; ++c) {
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setBackground(QColor(167, 243, 208)); // Verde por defecto
            m_gridDisponibilidad->setItem(r, c, item);
        }
    }
    rightLayout->addWidget(m_gridDisponibilidad);

    QHBoxLayout *legendLayout = new QHBoxLayout();
    QLabel *legendGreen = new QLabel(rightPanel);
    legendGreen->setFixedSize(16, 10);
    legendGreen->setStyleSheet("background-color: #a7f3d0; border-radius: 2px;");
    QLabel *legendGreenTxt = new QLabel("Available", rightPanel);
    legendGreenTxt->setObjectName("LegendTxt1");
    legendGreenTxt->setStyleSheet("color: #374151; font-size: 11px;");

    QLabel *legendRed = new QLabel(rightPanel);
    legendRed->setFixedSize(16, 10);
    legendRed->setStyleSheet("background-color: #fecaca; border-radius: 2px;");
    QLabel *legendRedTxt = new QLabel("Not Available", rightPanel);
    legendRedTxt->setObjectName("LegendTxt2");
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
    manageBtn->setObjectName("ManageBtn");
    manageBtn->setStyleSheet("background-color: #111827; color: white; padding: 12px; border-radius: 6px; font-weight: bold;");
    rightLayout->addWidget(manageBtn);

    mainLayout->addWidget(leftWidget, 7);
    mainLayout->addWidget(rightPanel, 3);

    // ==========================================
    // CONEXIONES
    // ==========================================
    connect(m_registerButton, &QPushButton::clicked, this, &TeacherListWidget::abrirFormularioNuevo);

    // Conexión segura a cualquier clic en la fila de la tabla
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int /*col*/) {
        m_table->selectRow(row);
        onDocenteSeleccionado();
    });
}

// ==========================================
// OPERACIONES CON LA BASE DE DATOS
// ==========================================

void TeacherListWidget::cargarDocentes() {
    if (!m_nucleo) return;

    m_table->setRowCount(0);
    auto res = m_nucleo->listarDocentes();

    if (!res.ok) {
        QMessageBox::warning(this, "Error BD", "No se pudo cargar la lista de docentes.");
        return;
    }

    for (const auto& profe : res.valor) {
        int fila = m_table->rowCount();
        m_table->insertRow(fila);

        // Función rápida para texto centrado
        auto crearItemCentrado = [](const QString& texto) {
            QTableWidgetItem* item = new QTableWidgetItem(texto);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        // Materias en formato texto
        QStringList listaMaterias;
        for (const auto& mat : profe.materias) {
            listaMaterias << mat.nombre;
        }
        QString txtMaterias = listaMaterias.isEmpty() ? "-" : listaMaterias.join(", ");

        m_table->setItem(fila, 0, crearItemCentrado(profe.id));
        m_table->setItem(fila, 1, crearItemCentrado(profe.nombre));
        m_table->setItem(fila, 2, crearItemCentrado(profe.email));
        m_table->setItem(fila, 3, crearItemCentrado(profe.telefono.isEmpty() ? "N/A" : profe.telefono));
        m_table->setItem(fila, 4, crearItemCentrado(txtMaterias));

        // Botones
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
}

void TeacherListWidget::onDocenteSeleccionado() {
    if (!m_nucleo || !m_gridDisponibilidad) return;

    int fila = m_table->currentRow();
    if (fila < 0) {
        m_detalleNombre->setText("Detalles del Docente");
        return;
    }

    QString cedula = m_table->item(fila, 0)->text();
    QString nombre = m_table->item(fila, 1)->text();

    m_detalleNombre->setText("Disponibilidad: " + nombre);

    auto res = m_nucleo->listarDocentes();
    if (!res.ok) return;

    ProfesorDTO profe;
    bool encontrado = false;
    for (const auto& p : res.valor) {
        if (p.id == cedula) {
            profe = p;
            encontrado = true;
            break;
        }
    }

    if (!encontrado) return;

    // Blanquear panel (Todo a verde disponible)
    for (int r = 0; r < 12; ++r) {
        for (int c = 0; c < 5; ++c) {
            QTableWidgetItem *item = m_gridDisponibilidad->item(r, c);
            if (!item) {
                item = new QTableWidgetItem();
                m_gridDisponibilidad->setItem(r, c, item);
            }
            item->setBackground(QColor(167, 243, 208));
            item->setText("");
            item->setToolTip("Disponible");
        }
    }

    // Pintar ocupados (rojo) calculando días y horas
    for (const auto& disp : profe.disponibilidad) {

        // 0=domingo, 1=lunes. La tabla empieza en Lunes (0). Restamos 1.
        int columna = disp.franja.dia - 1;

        // Restamos 7 para que 7am sea la fila 0
        int filaHora = disp.franja.inicio.hour() - 7;

        if (filaHora >= 0 && filaHora < 12 && columna >= 0 && columna < 5) {
            QTableWidgetItem *item = m_gridDisponibilidad->item(filaHora, columna);
            item->setBackground(QColor(254, 202, 202));
            item->setText("Not\nAvailable");
            item->setFont(QFont("Arial", 6));
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QColor(153, 27, 27));
            item->setToolTip("No Disponible");
        }
    }
}

void TeacherListWidget::abrirFormularioNuevo() {
    if (!m_nucleo) return;

    gestor::frontend::forms::TeacherFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::TeacherFormDialog::profesorGuardado,
            this, [this](const QString& cedula, const QString& nombre,
                   const QString& email, const QString& telefono,
                   const QString& materias) {

                auto resultado = m_nucleo->crearDocente(cedula, nombre, email, telefono);
                if (!resultado.ok) {
                    QMessageBox::warning(this, "Error", "No se pudo crear el docente: " + resultado.mensajeError);
                } else {
                    // Si hay materias, se crean (si no existen) y se asignan al momento
                    if (!materias.isEmpty()) {
                        QStringList listaMaterias = materias.split(",");
                        for (const QString& mat : listaMaterias) {
                            QString materiaLimpia = mat.trimmed();
                            if (!materiaLimpia.isEmpty()) {
                                m_nucleo->crearMateria(materiaLimpia);
                                m_nucleo->asignarMateriaPorNombre(cedula, materiaLimpia);
                            }
                        }
                    }
                    cargarDocentes();
                }
            });
    dialogo.exec();
}

void TeacherListWidget::abrirFormularioEditar(int fila) {
    if (!m_nucleo) return;

    QString cedula = m_table->item(fila, 0)->text();
    QString nombre = m_table->item(fila, 1)->text();
    QString email = m_table->item(fila, 2)->text();
    QString telefono = m_table->item(fila, 3)->text();
    QString materias = m_table->item(fila, 4)->text();
    if (materias == "-") materias = "";

    gestor::frontend::forms::TeacherFormDialog dialogo(this);
    dialogo.setWindowTitle("Editar Docente");
    dialogo.cargarDatos(cedula, nombre, email, telefono, materias);

    connect(&dialogo, &gestor::frontend::forms::TeacherFormDialog::profesorGuardado,
            this, [this, cedula](const QString& nCedula, const QString& nNombre,
                           const QString& nEmail, const QString& nTelefono,
                           const QString& nMaterias) {

                auto resultado = m_nucleo->actualizarDocente(nCedula, nNombre, nEmail, nTelefono);
                if (!resultado.ok) {
                    QMessageBox::warning(this, "Error", "No se pudo actualizar el docente: " + resultado.mensajeError);
                } else {
                    // Limpiar materias viejas y asignar las nuevas
                    m_nucleo->limpiarMaterias(nCedula);

                    if (!nMaterias.isEmpty()) {
                        QStringList listaMaterias = nMaterias.split(",");
                        for (const QString& mat : listaMaterias) {
                            QString materiaLimpia = mat.trimmed();
                            if (!materiaLimpia.isEmpty()) {
                                m_nucleo->crearMateria(materiaLimpia);
                                m_nucleo->asignarMateriaPorNombre(nCedula, materiaLimpia);
                            }
                        }
                    }
                    cargarDocentes();
                }
            });
    dialogo.exec();
}

void TeacherListWidget::eliminarDocente(int fila) {
    if (!m_nucleo) return;

    QString cedula = m_table->item(fila, 0)->text();
    if (QMessageBox::question(this, "Confirmar", "¿Está seguro de eliminar a este docente?") == QMessageBox::Yes) {
        if (m_nucleo->eliminarDocente(cedula)) {
            cargarDocentes();
        } else {
            QMessageBox::warning(this, "Error", "No se pudo eliminar el docente de la base de datos.");
        }
    }
}

// ==========================================
// TEMA VISUAL
// ==========================================
void TeacherListWidget::actualizarTema(bool modoOscuro) {
    if (modoOscuro) {
        if (auto t = findChild<QLabel*>("TeacherTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
        if (auto s = findChild<QLabel*>("TeacherSub")) s->setStyleSheet("font-size: 14px; color: #aaaaaa;");
        if (auto dt = findChild<QLabel*>("DetailTitle")) dt->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");

        if (auto t1 = findChild<QLabel*>("TabActive")) t1->setStyleSheet("font-weight: bold; color: #ffffff; border-bottom: 3px solid #ffffff; padding-bottom: 5px;");
        if (auto t2 = findChild<QLabel*>("TabInact1")) t2->setStyleSheet("color: #aaaaaa;");
        if (auto t3 = findChild<QLabel*>("TabInact2")) t3->setStyleSheet("color: #aaaaaa;");
        if (auto l1 = findChild<QLabel*>("LegendTxt1")) l1->setStyleSheet("color: #dddddd; font-size: 11px;");
        if (auto l2 = findChild<QLabel*>("LegendTxt2")) l2->setStyleSheet("color: #dddddd; font-size: 11px;");

        if (auto panel = findChild<QFrame*>("TeacherRightPanel")) {
            panel->setStyleSheet("QFrame { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; color: white; }");
        }

        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: #1e1e1e; border-radius: 8px; border: 1px solid #333333; color: #ffffff; }"
                "QHeaderView::section { background-color: #252525; font-weight: bold; color: #ffffff; "
                "border: none; border-bottom: 1px solid #333333; padding: 12px; }"
                "QTableWidget::item { border-bottom: 1px solid #2a2a2a; padding: 5px; color: #ffffff; }");
        }

        if (m_gridDisponibilidad) {
            m_gridDisponibilidad->setStyleSheet(
                "QTableWidget { border: none; gridline-color: #333333; background-color: #1e1e1e; color: white; }"
                "QHeaderView::section { background-color: #252525; border: none; font-size: 11px; color: #aaaaaa; padding: 2px; }");
        }

        if (auto btn = findChild<QPushButton*>("ManageBtn")) {
            btn->setStyleSheet("background-color: #333333; color: white; padding: 12px; border-radius: 6px; font-weight: bold; border: 1px solid #444444;");
        }
    } else {
        if (auto t = findChild<QLabel*>("TeacherTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827;");
        if (auto s = findChild<QLabel*>("TeacherSub")) s->setStyleSheet("font-size: 14px; color: #6b7280;");
        if (auto dt = findChild<QLabel*>("DetailTitle")) dt->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827;");

        if (auto t1 = findChild<QLabel*>("TabActive")) t1->setStyleSheet("font-weight: bold; color: #111827; border-bottom: 3px solid #111827; padding-bottom: 5px;");
        if (auto t2 = findChild<QLabel*>("TabInact1")) t2->setStyleSheet("color: #6b7280;");
        if (auto t3 = findChild<QLabel*>("TabInact2")) t3->setStyleSheet("color: #6b7280;");
        if (auto l1 = findChild<QLabel*>("LegendTxt1")) l1->setStyleSheet("color: #374151; font-size: 11px;");
        if (auto l2 = findChild<QLabel*>("LegendTxt2")) l2->setStyleSheet("color: #374151; font-size: 11px;");

        if (auto panel = findChild<QFrame*>("TeacherRightPanel")) {
            panel->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");
        }

        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; color: #334155; }"
                "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; "
                "border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
                "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }");
        }

        if (m_gridDisponibilidad) {
            m_gridDisponibilidad->setStyleSheet(
                "QTableWidget { border: none; gridline-color: white; background-color: transparent; }"
                "QHeaderView::section { background-color: white; border: none; font-size: 11px; color: #6b7280; padding: 2px; }");
        }

        if (auto btn = findChild<QPushButton*>("ManageBtn")) {
            btn->setStyleSheet("background-color: #111827; color: white; padding: 12px; border-radius: 6px; font-weight: bold;");
        }
    }
}