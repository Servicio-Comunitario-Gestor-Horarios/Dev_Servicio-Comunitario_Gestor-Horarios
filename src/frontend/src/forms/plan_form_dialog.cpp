#include "plan_form_dialog.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QHeaderView>
#include <QSpinBox>
#include <QSet>

namespace gestor::frontend::forms {

PlanFormDialog::PlanFormDialog(const QStringList& materiasTotales, QWidget *parent) : QDialog(parent) {
    setWindowTitle("Gestión de Plan de Estudio");
    resize(650, 500);

    bool isDark = qApp->property("isDarkMode").toBool();

    QString bgColor = isDark ? "#1e1e1e" : "white";
    QString textColor = isDark ? "#ffffff" : "#374151";
    QString inputBg = isDark ? "#2a2a2a" : "white";
    QString inputBorder = isDark ? "#444444" : "#d1d5db";
    QString labelColor = isDark ? "#ffffff" : "#111827";

    QString hoverBg = isDark ? "#333333" : "#f1f5f9";
    QString hoverText = isDark ? "#ffffff" : "#111827";

    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(bgColor, textColor));

    // CORRECCIÓN: Separamos los estilos de QListWidget y QTableWidget para evitar el corte en el medio
    QString inputStyle = QString(
                             "QLineEdit, QTextEdit, QListWidget, QTableWidget { "
                             "   border: 1px solid %1; border-radius: 6px; padding: 4px; "
                             "   background-color: %2; color: %3; font-size: 13px; outline: none; "
                             "}"
                             "QLineEdit:focus, QTextEdit:focus, QListWidget:focus, QTableWidget:focus { "
                             "   border: 1px solid #3b82f6; "
                             "}"
                             "QListWidget::item { "
                             "   padding: 4px; border: none; border-radius: 4px; " // La lista izquierda sí lleva esquinas redondeadas
                             "}"
                             "QTableWidget::item { "
                             "   padding: 4px; border: none; border-radius: 0px; " // La tabla derecha NO lleva redondeo para que se unan las celdas
                             "}"
                             "QListWidget::item:selected, QListWidget::item:selected:!active { "
                             "   background-color: #3b82f6; color: white; "
                             "}"
                             "QTableWidget::item:selected, QTableWidget::item:selected:!active { "
                             "   background-color: #3b82f6; color: white; "
                             "}"
                             "QListWidget::item:hover, QTableWidget::item:hover { "
                             "   background-color: %4; color: %5; "
                             "}"
                             ).arg(inputBorder, inputBg, textColor, hoverBg, hoverText);

    m_campoCodigo = new QLineEdit(this);
    m_campoCodigo->setStyleSheet(inputStyle);
    m_campoCodigo->setPlaceholderText("Ej: 202601");
    QRegularExpression rx("^[0-9]+$");
    m_campoCodigo->setValidator(new QRegularExpressionValidator(rx, this));

    m_campoNombre = new QLineEdit(this);
    m_campoNombre->setStyleSheet(inputStyle);
    m_campoNombre->setPlaceholderText("Nombre del plan");

    m_campoDescripcion = new QTextEdit(this);
    m_campoDescripcion->setStyleSheet(inputStyle);
    m_campoDescripcion->setPlaceholderText("Descripción breve...");
    m_campoDescripcion->setFixedHeight(60);

    m_listaDisponibles = new QListWidget(this);
    m_listaDisponibles->setStyleSheet(inputStyle);
    m_listaDisponibles->addItems(materiasTotales);
    m_listaDisponibles->setSpacing(2);

    m_tableSeleccionadas = new QTableWidget(0, 2, this);
    m_tableSeleccionadas->setStyleSheet(inputStyle);
    m_tableSeleccionadas->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableSeleccionadas->horizontalHeader()->setVisible(false);
    m_tableSeleccionadas->verticalHeader()->setVisible(false);
    m_tableSeleccionadas->setShowGrid(false);
    m_tableSeleccionadas->setFocusPolicy(Qt::NoFocus);
    m_tableSeleccionadas->verticalHeader()->setDefaultSectionSize(40);

    QHeaderView *header = m_tableSeleccionadas->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tableSeleccionadas->setColumnWidth(1, 90);

    m_btnAgregar = new QPushButton(">>", this);
    m_btnQuitar = new QPushButton("<<", this);
    QString btnTransferStyle = QString("QPushButton { background-color: %1; border: 1px solid %2; border-radius: 4px; padding: 4px; color: %3; font-weight: bold; } QPushButton:hover { background-color: #3b82f6; color: white; border: none; }").arg(inputBg, inputBorder, textColor);
    m_btnAgregar->setStyleSheet(btnTransferStyle);
    m_btnQuitar->setStyleSheet(btnTransferStyle);
    m_btnAgregar->setCursor(Qt::PointingHandCursor);
    m_btnQuitar->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *transferLayout = new QVBoxLayout();
    transferLayout->addStretch();
    transferLayout->addWidget(m_btnAgregar);
    transferLayout->addWidget(m_btnQuitar);
    transferLayout->addStretch();

    QHBoxLayout *listasLayout = new QHBoxLayout();

    QVBoxLayout *dispLayout = new QVBoxLayout();
    QLabel *lblDisp = new QLabel("Materias Disponibles:");
    lblDisp->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px;").arg(textColor));
    dispLayout->addWidget(lblDisp);
    dispLayout->addWidget(m_listaDisponibles);

    QVBoxLayout *selLayout = new QVBoxLayout();
    QHBoxLayout *selHeaderLayout = new QHBoxLayout();

    QLabel *lblSel = new QLabel("Materias del Plan:");
    lblSel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px;").arg(textColor));

    QLabel *lblTotal = new QLabel("Total Horas: 0");
    lblTotal->setObjectName("lblTotalHoras");
    lblTotal->setStyleSheet(QString("color: #3b82f6; font-weight: bold; font-size: 12px;"));
    lblTotal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    selHeaderLayout->addWidget(lblSel);
    selHeaderLayout->addStretch();
    selHeaderLayout->addWidget(lblTotal);

    selLayout->addLayout(selHeaderLayout);
    selLayout->addWidget(m_tableSeleccionadas);

    listasLayout->addLayout(dispLayout);
    listasLayout->addLayout(transferLayout);
    listasLayout->addLayout(selLayout);

    m_botonCancelar = new QPushButton("Cancelar", this);
    m_botonCancelar->setStyleSheet(isDark ?
                                       "QPushButton { background-color: #2a2a2a; border: 1px solid #444444; border-radius: 6px; padding: 10px 20px; color: #ffffff; font-weight: bold; } QPushButton:hover { background-color: #333333; }" :
                                       "QPushButton { background-color: white; border: 1px solid #d1d5db; border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } QPushButton:hover { background-color: #f3f4f6; }");
    m_botonCancelar->setCursor(Qt::PointingHandCursor);

    m_botonGuardar = new QPushButton("Guardar", this);
    m_botonGuardar->setStyleSheet(isDark ?
                                      "QPushButton { background-color: #3b82f6; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #2563eb; }" :
                                      "QPushButton { background-color: #1a237e; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #283593; }");
    m_botonGuardar->setCursor(Qt::PointingHandCursor);

    QFormLayout *formLayout = new QFormLayout();
    QString formLabelStyle = QString("<span style='font-weight:bold; color:%1; font-size: 13px;'>").arg(labelColor);
    formLayout->addRow(formLabelStyle + "Código numérico:</span>", m_campoCodigo);
    formLayout->addRow(formLabelStyle + "Nombre del Plan:</span>", m_campoNombre);
    formLayout->addRow(formLabelStyle + "Descripción:</span>", m_campoDescripcion);

    QHBoxLayout *botonesLayout = new QHBoxLayout();
    botonesLayout->addStretch();
    botonesLayout->addWidget(m_botonCancelar);
    botonesLayout->addWidget(m_botonGuardar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(listasLayout);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(botonesLayout);

    connect(m_btnAgregar, &QPushButton::clicked, this, &PlanFormDialog::agregarMateria);
    connect(m_btnQuitar, &QPushButton::clicked, this, &PlanFormDialog::quitarMateria);
    connect(m_botonGuardar, &QPushButton::clicked, this, &PlanFormDialog::guardarPlan);
    connect(m_botonCancelar, &QPushButton::clicked, this, &QDialog::reject);
}

void PlanFormDialog::agregarMateria() {
    QList<QListWidgetItem*> seleccionados = m_listaDisponibles->selectedItems();

    bool isDark = qApp->property("isDarkMode").toBool();
    QString frameBg = isDark ? "#2a2a2a" : "#ffffff";
    QString frameBorder = isDark ? "#444444" : "#cbd5e1";
    QString numColor = isDark ? "#ffffff" : "#1e40af";
    QString arrowColor = isDark ? "#ffffff" : "#475569";
    QString btnHover = isDark ? "rgba(255,255,255,0.1)" : "rgba(0,0,0,0.05)";

    for (QListWidgetItem* item : seleccionados) {
        int row = m_tableSeleccionadas->rowCount();
        m_tableSeleccionadas->insertRow(row);

        QTableWidgetItem *nombreItem = new QTableWidgetItem(item->text());
        nombreItem->setFlags(nombreItem->flags() ^ Qt::ItemIsEditable);
        m_tableSeleccionadas->setItem(row, 0, nombreItem);

        QFrame *frame = new QFrame();
        frame->setFixedSize(70, 28);
        frame->setStyleSheet(QString("QFrame { border: 1px solid %1; border-radius: 4px; background: %2; }").arg(frameBorder, frameBg));
        QHBoxLayout *frameLayout = new QHBoxLayout(frame);
        frameLayout->setContentsMargins(0, 0, 0, 0);
        frameLayout->setSpacing(0);

        QSpinBox *spinHoras = new QSpinBox();
        spinHoras->setRange(1, 20);
        spinHoras->setValue(4);
        spinHoras->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spinHoras->setAlignment(Qt::AlignCenter);
        spinHoras->setStyleSheet(QString("QSpinBox { border: none; background: transparent; color: %1; font-weight: 500; font-size: 14px; }").arg(numColor));

        QVBoxLayout *btnLayout = new QVBoxLayout();
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(0);

        QPushButton *btnUp = new QPushButton("▲");
        btnUp->setFixedSize(20, 14);
        btnUp->setCursor(Qt::PointingHandCursor);
        btnUp->setStyleSheet(QString("QPushButton { border: none; border-left: 1px solid %1; border-bottom: 1px solid %1; border-top-right-radius: 4px; background: transparent; color: %2; font-family: Arial; font-size: 8px; padding-bottom: 1px; } QPushButton:hover { background: %3; }").arg(frameBorder, arrowColor, btnHover));

        QPushButton *btnDown = new QPushButton("▼");
        btnDown->setFixedSize(20, 14);
        btnDown->setCursor(Qt::PointingHandCursor);
        btnDown->setStyleSheet(QString("QPushButton { border: none; border-left: 1px solid %1; border-bottom-right-radius: 4px; background: transparent; color: %2; font-family: Arial; font-size: 8px; padding-top: 1px; } QPushButton:hover { background: %3; }").arg(frameBorder, arrowColor, btnHover));

        btnLayout->addWidget(btnUp);
        btnLayout->addWidget(btnDown);

        frameLayout->addWidget(spinHoras);
        frameLayout->addLayout(btnLayout);

        connect(btnUp, &QPushButton::clicked, spinHoras, &QSpinBox::stepUp);
        connect(btnDown, &QPushButton::clicked, spinHoras, &QSpinBox::stepDown);
        connect(spinHoras, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlanFormDialog::actualizarTotalHoras);

        QWidget *cellWidget = new QWidget();
        cellWidget->setStyleSheet("background: transparent; border: none;");
        QHBoxLayout *cellLayout = new QHBoxLayout(cellWidget);
        cellLayout->setContentsMargins(0, 0, 8, 0);
        cellLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        cellLayout->addWidget(frame);

        m_tableSeleccionadas->setCellWidget(row, 1, cellWidget);

        delete m_listaDisponibles->takeItem(m_listaDisponibles->row(item));
    }
    actualizarTotalHoras();
}

void PlanFormDialog::quitarMateria() {
    QList<QTableWidgetItem*> seleccionados = m_tableSeleccionadas->selectedItems();
    QSet<int> rowsToDelete;

    for (auto item : seleccionados) {
        rowsToDelete.insert(item->row());
    }

    QList<int> sortedRows = rowsToDelete.values();
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

    for (int r : sortedRows) {
        QString nombre = m_tableSeleccionadas->item(r, 0)->text();
        m_listaDisponibles->addItem(nombre);
        m_tableSeleccionadas->removeRow(r);
    }
    actualizarTotalHoras();
}

void PlanFormDialog::actualizarTotalHoras() {
    int total = 0;
    for (int i = 0; i < m_tableSeleccionadas->rowCount(); ++i) {
        QWidget *cellWidget = m_tableSeleccionadas->cellWidget(i, 1);
        if (cellWidget) {
            QSpinBox *spin = cellWidget->findChild<QSpinBox*>();
            if (spin) total += spin->value();
        }
    }

    if (QLabel *lbl = findChild<QLabel*>("lblTotalHoras")) {
        lbl->setText(QString("Total Horas: %1").arg(total));
    }
}

bool PlanFormDialog::validarCampos() {
    QString errores;
    bool ok = true;

    if (m_campoCodigo->text().trimmed().isEmpty()) {
        errores += "• El código del plan es obligatorio (solo números).\n";
        ok = false;
    }
    if (m_campoNombre->text().trimmed().isEmpty()) {
        errores += "• El nombre del plan es obligatorio.\n";
        ok = false;
    }
    if (m_tableSeleccionadas->rowCount() == 0) {
        errores += "• Debe asignar al menos una materia al plan.\n";
        ok = false;
    }

    if (!ok) {
        QMessageBox::warning(this, "Error de Validación", "Por favor corrija los siguientes errores:\n\n" + errores);
    }
    return ok;
}

void PlanFormDialog::guardarPlan() {
    if (!validarCampos()) return;

    QList<MateriaPlanInfo> materiasAsignadas;
    int totalHoras = 0;

    for (int i = 0; i < m_tableSeleccionadas->rowCount(); ++i) {
        MateriaPlanInfo info;
        info.nombre = m_tableSeleccionadas->item(i, 0)->text();

        QWidget *cellWidget = m_tableSeleccionadas->cellWidget(i, 1);
        if (cellWidget) {
            QSpinBox *spin = cellWidget->findChild<QSpinBox*>();
            info.horas = spin ? spin->value() : 0;
        } else {
            info.horas = 0;
        }

        totalHoras += info.horas;
        materiasAsignadas.append(info);
    }

    emit planGuardado(m_campoCodigo->text().trimmed(),
                      m_campoNombre->text().trimmed(),
                      m_campoDescripcion->toPlainText().trimmed(),
                      materiasAsignadas,
                      totalHoras);
    accept();
}

void PlanFormDialog::cargarDatos(const QString& codigo, const QString& nombre, const QString& descripcion, const QList<MateriaPlanInfo>& materiasSeleccionadas) {
    m_campoCodigo->setText(codigo);
    m_campoNombre->setText(nombre);
    m_campoDescripcion->setText(descripcion);

    bool isDark = qApp->property("isDarkMode").toBool();
    QString frameBg = isDark ? "#2a2a2a" : "#ffffff";
    QString frameBorder = isDark ? "#444444" : "#cbd5e1";
    QString numColor = isDark ? "#ffffff" : "#1e40af";
    QString arrowColor = isDark ? "#ffffff" : "#475569";
    QString btnHover = isDark ? "rgba(255,255,255,0.1)" : "rgba(0,0,0,0.05)";

    for (const auto& materia : materiasSeleccionadas) {
        QList<QListWidgetItem*> items = m_listaDisponibles->findItems(materia.nombre, Qt::MatchExactly);
        if (!items.isEmpty()) {
            delete m_listaDisponibles->takeItem(m_listaDisponibles->row(items.first()));
        }

        int row = m_tableSeleccionadas->rowCount();
        m_tableSeleccionadas->insertRow(row);

        QTableWidgetItem *nombreItem = new QTableWidgetItem(materia.nombre);
        nombreItem->setFlags(nombreItem->flags() ^ Qt::ItemIsEditable);
        m_tableSeleccionadas->setItem(row, 0, nombreItem);

        QFrame *frame = new QFrame();
        frame->setFixedSize(70, 28);
        frame->setStyleSheet(QString("QFrame { border: 1px solid %1; border-radius: 4px; background: %2; }").arg(frameBorder, frameBg));
        QHBoxLayout *frameLayout = new QHBoxLayout(frame);
        frameLayout->setContentsMargins(0, 0, 0, 0);
        frameLayout->setSpacing(0);

        QSpinBox *spinHoras = new QSpinBox();
        spinHoras->setRange(1, 20);
        spinHoras->setValue(materia.horas);
        spinHoras->setButtonSymbols(QAbstractSpinBox::NoButtons);
        spinHoras->setAlignment(Qt::AlignCenter);
        spinHoras->setStyleSheet(QString("QSpinBox { border: none; background: transparent; color: %1; font-weight: 500; font-size: 14px; }").arg(numColor));

        QVBoxLayout *btnLayout = new QVBoxLayout();
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(0);

        QPushButton *btnUp = new QPushButton("▲");
        btnUp->setFixedSize(20, 14);
        btnUp->setCursor(Qt::PointingHandCursor);
        btnUp->setStyleSheet(QString("QPushButton { border: none; border-left: 1px solid %1; border-bottom: 1px solid %1; border-top-right-radius: 4px; background: transparent; color: %2; font-family: Arial; font-size: 8px; padding-bottom: 1px; } QPushButton:hover { background: %3; }").arg(frameBorder, arrowColor, btnHover));

        QPushButton *btnDown = new QPushButton("▼");
        btnDown->setFixedSize(20, 14);
        btnDown->setCursor(Qt::PointingHandCursor);
        btnDown->setStyleSheet(QString("QPushButton { border: none; border-left: 1px solid %1; border-bottom-right-radius: 4px; background: transparent; color: %2; font-family: Arial; font-size: 8px; padding-top: 1px; } QPushButton:hover { background: %3; }").arg(frameBorder, arrowColor, btnHover));

        btnLayout->addWidget(btnUp);
        btnLayout->addWidget(btnDown);

        frameLayout->addWidget(spinHoras);
        frameLayout->addLayout(btnLayout);

        connect(btnUp, &QPushButton::clicked, spinHoras, &QSpinBox::stepUp);
        connect(btnDown, &QPushButton::clicked, spinHoras, &QSpinBox::stepDown);
        connect(spinHoras, QOverload<int>::of(&QSpinBox::valueChanged), this, &PlanFormDialog::actualizarTotalHoras);

        QWidget *cellWidget = new QWidget();
        cellWidget->setStyleSheet("background: transparent; border: none;");
        QHBoxLayout *cellLayout = new QHBoxLayout(cellWidget);
        cellLayout->setContentsMargins(0, 0, 8, 0);
        cellLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        cellLayout->addWidget(frame);

        m_tableSeleccionadas->setCellWidget(row, 1, cellWidget);
    }
    actualizarTotalHoras();
}

} // namespace gestor::frontend::forms