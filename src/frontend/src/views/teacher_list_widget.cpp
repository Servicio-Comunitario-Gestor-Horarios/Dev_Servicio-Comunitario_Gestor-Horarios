/* Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        [teacher_list_widget.cpp]
 * Autor:          Paola Peña
 * Fecha:          10 de Julio de 2026
 * Descripción:    Implementación de la interfaz gráfica (Frontend) utilizando
 *                 C++ y Qt6 para el módulo de gestión de docentes. Incluye el
 *                 diseño de la vista principal, la tabla de listado de docentes,
 *                 y el formulario de registro con validación de datos.
 */

#include "teacher_list_widget.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFrame>
#include "../forms/teacher_form_dialog.hpp"

TeacherListWidget::TeacherListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void TeacherListWidget::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // ==========================================
    // LADO IZQUIERDO: Controles y Tabla
    // ==========================================
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *topSectionLayout = new QHBoxLayout();
    QVBoxLayout *textLayout = new QVBoxLayout();
    QLabel *titulo = new QLabel("Gestión de Docentes", leftWidget);
    titulo->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827; border: none;");

    QLabel *subtitulo = new QLabel("Administra la información, disponibilidad y materias del personal académico", leftWidget);
    subtitulo->setStyleSheet("font-size: 14px; color: #6b7280; border: none;");

    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton("+ Registrar Docente", leftWidget);
    m_registerButton->setCursor(Qt::PointingHandCursor);
    m_registerButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #1a237e;"
        "   color: white;"
        "   font-weight: bold;"
        "   padding: 10px 20px;"
        "   border-radius: 6px;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #283593; }"
        );

    topSectionLayout->addLayout(textLayout);
    topSectionLayout->addStretch();
    topSectionLayout->addWidget(m_registerButton, 0, Qt::AlignVCenter);

    m_table = new QTableWidget(0, 6, leftWidget);
    m_table->setHorizontalHeaderLabels({"Cédula/ID", "Nombre", "Email", "Teléfono", "Materias", "Acciones"});

    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }"
        "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; text-align: left; }"
        "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }"
        );

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
    // LADO DERECHO: Panel de Detalles
    // ==========================================
    QFrame *rightPanel = new QFrame(this);
    rightPanel->setMinimumWidth(320);
    rightPanel->setMaximumWidth(400);
    rightPanel->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(15);

    // También cambié el título fijo para que sea más genérico cuando la tabla esté vacía
    QLabel *detalleTitulo = new QLabel("Detalles del Docente", rightPanel);
    detalleTitulo->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827; border: none;");
    rightLayout->addWidget(detalleTitulo, 0, Qt::AlignHCenter);

    QHBoxLayout *tabsLayout = new QHBoxLayout();
    QLabel *tab1 = new QLabel("Disponibilidad\nHoraria", rightPanel);
    tab1->setAlignment(Qt::AlignCenter);
    tab1->setStyleSheet("font-weight: bold; color: #111827; border: none; border-bottom: 3px solid #111827; padding-bottom: 5px;");
    QLabel *tab2 = new QLabel("Horas\nPreferidas", rightPanel);
    tab2->setAlignment(Qt::AlignCenter);
    tab2->setStyleSheet("color: #6b7280; border: none;");
    QLabel *tab3 = new QLabel("Asignaturas", rightPanel);
    tab3->setAlignment(Qt::AlignCenter);
    tab3->setStyleSheet("color: #6b7280; border: none;");

    tabsLayout->addWidget(tab1);
    tabsLayout->addWidget(tab2);
    tabsLayout->addWidget(tab3);
    rightLayout->addLayout(tabsLayout);

    QTableWidget *grid = new QTableWidget(12, 5, rightPanel);
    grid->setHorizontalHeaderLabels({"Lunes", "Martes", "Mierc.", "Jueves", "Viernes"});
    grid->setVerticalHeaderLabels({"7am", "8am", "9am", "10am", "11am", "12pm", "1pm", "2pm", "3pm", "4pm", "5pm", "6pm"});
    grid->setStyleSheet(
        "QTableWidget { border: none; gridline-color: white; }"
        "QHeaderView::section { background-color: white; border: none; font-size: 10px; color: #6b7280; font-weight: normal; }"
        );
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
    legendGreen->setStyleSheet("background-color: #a7f3d0; border-radius: 2px; border: none;");
    QLabel *legendGreenTxt = new QLabel("Available", rightPanel);
    legendGreenTxt->setStyleSheet("border: none; color: #374151; font-size: 11px;");

    QLabel *legendRed = new QLabel(rightPanel);
    legendRed->setFixedSize(16, 10);
    legendRed->setStyleSheet("background-color: #fecaca; border-radius: 2px; border: none;");
    QLabel *legendRedTxt = new QLabel("Not Available", rightPanel);
    legendRedTxt->setStyleSheet("border: none; color: #374151; font-size: 11px;");

    legendLayout->addStretch();
    legendLayout->addWidget(legendGreen);
    legendLayout->addWidget(legendGreenTxt);
    legendLayout->addSpacing(15);
    legendLayout->addWidget(legendRed);
    legendLayout->addWidget(legendRedTxt);
    legendLayout->addStretch();
    rightLayout->addLayout(legendLayout);

    QPushButton *manageBtn = new QPushButton("Gestionar Disponibilidad", rightPanel);
    manageBtn->setStyleSheet("background-color: #111827; color: white; padding: 12px; border-radius: 6px; font-weight: bold; border: none;");
    rightLayout->addWidget(manageBtn);

    mainLayout->addWidget(leftWidget, 7);
    mainLayout->addWidget(rightPanel, 3);

    connect(m_registerButton, &QPushButton::clicked, this, &TeacherListWidget::abrirFormularioNuevo);
}

void TeacherListWidget::abrirFormularioNuevo() {
    gestor::frontend::forms::TeacherFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::TeacherFormDialog::profesorGuardado,
            this, &TeacherListWidget::agregarProfesorATabla);
    dialogo.exec();
}

void TeacherListWidget::agregarProfesorATabla(const QString& id, const QString& nombre, const QString& email, const QString& telefono, const QString& materias) {
    int fila = m_table->rowCount();
    m_table->insertRow(fila);

    // 1. Cédula/ID
    QTableWidgetItem *itemId = new QTableWidgetItem(id);
    itemId->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 0, itemId);

    // 2. Nombre
    QTableWidgetItem *itemNombre = new QTableWidgetItem(nombre);
    itemNombre->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 1, itemNombre);

    // 3. Email
    QTableWidgetItem *itemEmail = new QTableWidgetItem(email.isEmpty() ? "N/A" : email);
    itemEmail->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 2, itemEmail);

    // 4. Teléfono
    QTableWidgetItem *itemTelefono = new QTableWidgetItem(telefono.isEmpty() ? "N/A" : telefono);
    itemTelefono->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 3, itemTelefono);

    // 5. Materias
    QTableWidgetItem *itemMaterias = new QTableWidgetItem(materias.isEmpty() ? "N/A" : materias);
    itemMaterias->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 4, itemMaterias);

    // 6. Acciones
    QTableWidgetItem *acciones = new QTableWidgetItem("...");
    acciones->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(fila, 5, acciones);
}
