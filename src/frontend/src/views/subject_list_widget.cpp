#include "subject_list_widget.hpp"
#include "../forms/subject_form_dialog.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>

SubjectListWidget::SubjectListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void SubjectListWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // Cabecera
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QVBoxLayout *textLayout = new QVBoxLayout();
    QLabel *titulo = new QLabel("Gestión de Asignaturas", this);
    titulo->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827;");
    QLabel *subtitulo = new QLabel("Administra las asignaturas y su tipo de aula requerido", this);
    subtitulo->setStyleSheet("font-size: 14px; color: #6b7280;");
    textLayout->addWidget(titulo);
    textLayout->addWidget(subtitulo);

    m_registerButton = new QPushButton("+ Registrar Asignatura", this);
    m_registerButton->setCursor(Qt::PointingHandCursor);
    m_registerButton->setStyleSheet(
        "QPushButton { background-color: #1a237e; color: white; font-weight: bold; "
        "padding: 10px 20px; border-radius: 6px; font-size: 14px; } "
        "QPushButton:hover { background-color: #283593; }");

    headerLayout->addLayout(textLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(m_registerButton);

    mainLayout->addLayout(headerLayout);

    // Tabla
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Nombre", "Tipo de aula", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }"
        "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; "
        "border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
        "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }");
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setDefaultSectionSize(45);

    mainLayout->addWidget(m_table);

    connect(m_registerButton, &QPushButton::clicked, this, &SubjectListWidget::abrirFormularioNuevo);
}

void SubjectListWidget::abrirFormularioNuevo() {
    gestor::frontend::forms::SubjectFormDialog dialogo(this);
    connect(&dialogo, &gestor::frontend::forms::SubjectFormDialog::materiaGuardada,
            this, &SubjectListWidget::agregarMateriaATabla);
    dialogo.exec();
}

void SubjectListWidget::agregarMateriaATabla(const QString& nombre, const QString& tipoAula) {
    int fila = m_table->rowCount();
    m_table->insertRow(fila);

    m_table->setItem(fila, 0, new QTableWidgetItem(nombre));
    m_table->setItem(fila, 1, new QTableWidgetItem(tipoAula));

    // Acciones
    QWidget *panelAcciones = new QWidget();
    QHBoxLayout *layoutAcciones = new QHBoxLayout(panelAcciones);
    layoutAcciones->setContentsMargins(0,0,0,0);
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
                                  "¿Está seguro de eliminar esta asignatura?") == QMessageBox::Yes) {
            for (int i=0; i<m_table->rowCount(); ++i) {
                if (m_table->cellWidget(i, 2) == panelAcciones) {
                    m_table->removeRow(i);
                    break;
                }
            }
        }
    });

    connect(btnEditar, &QPushButton::clicked, this, [this, panelAcciones]() {
        int filaEditar = -1;
        for (int i=0; i<m_table->rowCount(); ++i) {
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
                this, [this, filaEditar](const QString& nNombre, const QString& nTipo) {
                    m_table->item(filaEditar, 0)->setText(nNombre);
                    m_table->item(filaEditar, 1)->setText(nTipo);
                });
        dialogo.exec();
    });

    m_table->setCellWidget(fila, 2, panelAcciones);
}