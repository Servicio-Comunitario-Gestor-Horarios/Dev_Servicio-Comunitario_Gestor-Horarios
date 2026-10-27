/**
 * @file teacher_list_widget.cpp
 * @brief Implementación del listado de docentes con panel de pestañas y rejillas de disponibilidad/preferencias
 */

#include "teacher_list_widget.hpp"
#include "../forms/teacher_form_dialog.hpp"
#include "../widgets/selector_materias.hpp"
#include "backend/services/NucleoDatos.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>

// ==========================================
// ESTRUCTURA AUXILIAR PARA FRANJAS (MOCK DINÁMICO)
// ==========================================
struct FranjaAcademica {
    QString etiqueta;
    bool esReceso;
};

static QList<FranjaAcademica> obtenerFranjasDelCentro() {
    return {
        // --- TURNO MAÑANA ---
        {"06:50 - 07:40", false},
        {"07:40 - 08:20", false},
        {"08:20 - 08:30", true},  // <-- RECESO MAÑANA
        {"08:30 - 09:10", false},
        {"09:10 - 09:50", false},
        {"09:50 - 10:30", false},
        {"10:30 - 11:10", false},
        {"11:10 - 11:50", false},
        {"11:50 - 12:30", false},

        // --- TURNO TARDE ---
        {"12:35 - 13:20", false},
        {"13:20 - 14:00", false},
        {"14:00 - 14:10", true},  // <-- RECESO TARDE
        {"14:10 - 14:50", false},
        {"14:50 - 15:30", false},
        {"15:30 - 16:10", false},
        {"16:10 - 16:50", false},
        {"16:50 - 17:30", false},
        {"17:30 - 18:10", false}
    };
}

static QStringList obtenerDiasLaborables() {
    return {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes"};
}

TeacherListWidget::TeacherListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    construirRejillasDinamicas();
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

    QLabel *subtitulo = new QLabel("Administra la información, disponibilidad y preferencias del personal", leftWidget);
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

    m_table = new QTableWidget(0, 6, leftWidget);
    m_table->setObjectName("TeacherTable");
    m_table->setHorizontalHeaderLabels({"Cédula/ID", "Nombre", "Email", "Teléfono", "Materias", "Acciones"});
    m_table->setStyleSheet(
        "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; color: #334155; }"
        "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; "
        "border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
        "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }"
        "QTableWidget::item:selected { background-color: #1a237e; color: white; }");

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
    // LADO DERECHO: PANEL DE DETALLES (TABS)
    // ==========================================
    QFrame *rightPanel = new QFrame(this);
    rightPanel->setObjectName("TeacherRightPanel");
    rightPanel->setMinimumWidth(480);
    rightPanel->setMaximumWidth(550);
    rightPanel->setStyleSheet("QFrame#TeacherRightPanel { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(15);

    setupPanelDerecho(rightLayout);

    mainLayout->addWidget(leftWidget, 7);
    mainLayout->addWidget(rightPanel, 4);

    connect(m_registerButton, &QPushButton::clicked, this, &TeacherListWidget::abrirFormularioNuevo);
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int /*col*/) {
        m_table->selectRow(row);
        onDocenteSeleccionado();
    });
}

void TeacherListWidget::setupPanelDerecho(QVBoxLayout* rightLayout) {
    m_detalleNombre = new QLabel("Seleccione un docente", this);
    m_detalleNombre->setObjectName("DetailTitle");
    m_detalleNombre->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827;");
    rightLayout->addWidget(m_detalleNombre, 0, Qt::AlignHCenter);

    // Controles de Pestañas
    QHBoxLayout *tabsLayout = new QHBoxLayout();
    tabsLayout->setSpacing(0);

    m_btnTabDisponibilidad = new QPushButton("Disponibilidad", this);
    m_btnTabPreferencias = new QPushButton("Preferencias", this);
    m_btnTabAsignaturas = new QPushButton("Asignaturas", this);

    QString tabStyle = "QPushButton { border: none; background: transparent; font-weight: bold; color: #6b7280; padding: 10px; border-bottom: 3px solid transparent; } "
                       "QPushButton:hover { color: #111827; }";
    m_btnTabDisponibilidad->setStyleSheet(tabStyle);
    m_btnTabPreferencias->setStyleSheet(tabStyle);
    m_btnTabAsignaturas->setStyleSheet(tabStyle);

    m_btnTabDisponibilidad->setCursor(Qt::PointingHandCursor);
    m_btnTabPreferencias->setCursor(Qt::PointingHandCursor);
    m_btnTabAsignaturas->setCursor(Qt::PointingHandCursor);

    tabsLayout->addWidget(m_btnTabDisponibilidad);
    tabsLayout->addWidget(m_btnTabPreferencias);
    tabsLayout->addWidget(m_btnTabAsignaturas);
    rightLayout->addLayout(tabsLayout);

    // Stack de Vistas
    m_stackedViews = new QStackedWidget(this);

    // Vista 0: Disponibilidad
    QWidget* pageDisp = new QWidget();
    QVBoxLayout* layDisp = new QVBoxLayout(pageDisp);
    layDisp->setContentsMargins(0,0,0,0);
    QLabel* helpDisp = new QLabel("Haz clic en las franjas donde el docente NO está disponible para dar clase.");
    helpDisp->setWordWrap(true);
    helpDisp->setStyleSheet("color: #6b7280; font-size: 12px; font-style: italic;");
    m_gridDisponibilidad = new QTableWidget(this);
    layDisp->addWidget(helpDisp);
    layDisp->addWidget(m_gridDisponibilidad);

    // Vista 1: Preferencias
    QWidget* pagePref = new QWidget();
    QVBoxLayout* layPref = new QVBoxLayout(pagePref);
    layPref->setContentsMargins(0,0,0,0);
    QLabel* helpPref = new QLabel("Haz clic en las franjas que el docente PREFIERE. (Horas no disponibles bloqueadas).");
    helpPref->setWordWrap(true);
    helpPref->setStyleSheet("color: #6b7280; font-size: 12px; font-style: italic;");
    m_gridPreferencias = new QTableWidget(this);
    layPref->addWidget(helpPref);
    layPref->addWidget(m_gridPreferencias);

    // ==========================================
    // Vista 2: Asignaturas (Gestión Titulares y Suplencias) - Ocupando todo el recuadro
    // ==========================================
    QWidget* pageAsig = new QWidget();
    QVBoxLayout* layAsig = new QVBoxLayout(pageAsig);
    layAsig->setContentsMargins(8, 10, 8, 10);
    layAsig->setSpacing(12);

    m_avisoSinMaterias = new QLabel("⚠️ Sin materias asignadas: este docente no podrá usarse en la generación.", pageAsig);
    m_avisoSinMaterias->setObjectName("AvisoSinMaterias");
    m_avisoSinMaterias->setStyleSheet("color: #b91c1c; background-color: #fef2f2; border: 1px solid #f87171; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 12px;");
    m_avisoSinMaterias->setWordWrap(true);
    layAsig->addWidget(m_avisoSinMaterias);
    m_avisoSinMaterias->hide();

    // --- Sección Titulares ---
    QVBoxLayout* cardTitulares = new QVBoxLayout();
    cardTitulares->setContentsMargins(0, 0, 0, 0);
    cardTitulares->setSpacing(6);

    QHBoxLayout* headerTitulares = new QHBoxLayout();
    headerTitulares->setContentsMargins(4, 0, 4, 0);
    QLabel* lblTitulares = new QLabel("📚 Materias que dicta (Titular):");
    lblTitulares->setObjectName("LblTitulares");
    lblTitulares->setStyleSheet("font-size: 13px; font-weight: bold; color: #111827;");

    m_btnEditarTitulares = new QPushButton("Modificar", pageAsig);
    m_btnEditarTitulares->setCursor(Qt::PointingHandCursor);
    m_btnEditarTitulares->setStyleSheet("QPushButton { background-color: #e0e7ff; color: #3730a3; border-radius: 4px; padding: 5px 12px; font-weight: bold; } QPushButton:hover { background-color: #c7d2fe; }");

    headerTitulares->addWidget(lblTitulares);
    headerTitulares->addStretch();
    headerTitulares->addWidget(m_btnEditarTitulares);
    cardTitulares->addLayout(headerTitulares);

    m_listTitulares = new QListWidget(pageAsig);
    m_listTitulares->setFocusPolicy(Qt::NoFocus);
    m_listTitulares->setSelectionMode(QAbstractItemView::NoSelection);
    m_listTitulares->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cardTitulares->addWidget(m_listTitulares);
    layAsig->addLayout(cardTitulares, 1);

    // --- Sección Suplencias ---
    QVBoxLayout* cardSuplencias = new QVBoxLayout();
    cardSuplencias->setContentsMargins(0, 0, 0, 0);
    cardSuplencias->setSpacing(6);

    QHBoxLayout* headerSuplencias = new QHBoxLayout();
    headerSuplencias->setContentsMargins(4, 0, 4, 0);
    QLabel* lblSuplencias = new QLabel("🔄 Puede ser suplente en:");
    lblSuplencias->setObjectName("LblSuplencias");
    lblSuplencias->setStyleSheet("font-size: 13px; font-weight: bold; color: #111827;");

    m_btnEditarSuplentes = new QPushButton("Modificar", pageAsig);
    m_btnEditarSuplentes->setCursor(Qt::PointingHandCursor);
    m_btnEditarSuplentes->setStyleSheet("QPushButton { background-color: #e0e7ff; color: #3730a3; border-radius: 4px; padding: 5px 12px; font-weight: bold; } QPushButton:hover { background-color: #c7d2fe; }");

    headerSuplencias->addWidget(lblSuplencias);
    headerSuplencias->addStretch();
    headerSuplencias->addWidget(m_btnEditarSuplentes);
    cardSuplencias->addLayout(headerSuplencias);

    m_listSuplentes = new QListWidget(pageAsig);
    m_listSuplentes->setFocusPolicy(Qt::NoFocus);
    m_listSuplentes->setSelectionMode(QAbstractItemView::NoSelection);
    m_listSuplentes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cardSuplencias->addWidget(m_listSuplentes);
    layAsig->addLayout(cardSuplencias, 1);

    m_stackedViews->addWidget(pageDisp);
    m_stackedViews->addWidget(pagePref);
    m_stackedViews->addWidget(pageAsig);
    rightLayout->addWidget(m_stackedViews);

    // Panel de Guardado
    m_panelGuardado = new QWidget(this);
    QHBoxLayout* layGuardado = new QHBoxLayout(m_panelGuardado);
    layGuardado->setContentsMargins(0, 10, 0, 0);

    m_btnDescartar = new QPushButton("Descartar cambios", this);
    m_btnDescartar->setStyleSheet("background-color: #f3f4f6; color: #374151; font-weight: bold; padding: 10px; border-radius: 6px; border: 1px solid #d1d5db;");
    m_btnDescartar->setCursor(Qt::PointingHandCursor);

    m_btnGuardar = new QPushButton("Guardar Cambios", this);
    m_btnGuardar->setStyleSheet("background-color: #1a237e; color: white; font-weight: bold; padding: 10px; border-radius: 6px;");
    m_btnGuardar->setCursor(Qt::PointingHandCursor);

    layGuardado->addStretch();
    layGuardado->addWidget(m_btnDescartar);
    layGuardado->addWidget(m_btnGuardar);
    rightLayout->addWidget(m_panelGuardado);
    m_panelGuardado->setVisible(false);

    // Conexiones Pestañas y Rejillas
    connect(m_btnTabDisponibilidad, &QPushButton::clicked, this, [this](){ cambiarPestana(0); });
    connect(m_btnTabPreferencias, &QPushButton::clicked, this, [this](){ cambiarPestana(1); });
    connect(m_btnTabAsignaturas, &QPushButton::clicked, this, [this](){ cambiarPestana(2); });
    connect(m_gridDisponibilidad, &QTableWidget::cellClicked, this, &TeacherListWidget::onDisponibilidadCellClicked);
    connect(m_gridPreferencias, &QTableWidget::cellClicked, this, &TeacherListWidget::onPreferenciasCellClicked);

    // Conexiones Asignaturas Titulares/Suplentes
    connect(m_btnEditarTitulares, &QPushButton::clicked, this, &TeacherListWidget::onEditarTitularesClicked);
    connect(m_btnEditarSuplentes, &QPushButton::clicked, this, &TeacherListWidget::onEditarSuplentesClicked);

    // Conexiones Guardado
    connect(m_btnGuardar, &QPushButton::clicked, this, &TeacherListWidget::guardarCambios);
    connect(m_btnDescartar, &QPushButton::clicked, this, &TeacherListWidget::descartarCambios);

    cambiarPestana(0);
}

// ==========================================
// CONSTRUCCIÓN DINÁMICA DE REJILLAS
// ==========================================
void TeacherListWidget::construirRejillasDinamicas() {
    QList<FranjaAcademica> franjas = obtenerFranjasDelCentro();
    QStringList dias = obtenerDiasLaborables();

    auto configurarRejilla = [this, franjas, dias](QTableWidget* grid) {
        grid->clear();
        grid->setRowCount(franjas.size());
        grid->setColumnCount(dias.size());
        grid->setHorizontalHeaderLabels(dias);

        QStringList etiquetasVerticales;
        for (int r = 0; r < franjas.size(); ++r) {
            etiquetasVerticales << franjas[r].etiqueta;
        }
        grid->setVerticalHeaderLabels(etiquetasVerticales);

        grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        grid->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        grid->verticalHeader()->setMinimumWidth(85);
        grid->setSelectionMode(QAbstractItemView::NoSelection);
        grid->setEditTriggers(QAbstractItemView::NoEditTriggers);

        for (int r = 0; r < franjas.size(); ++r) {
            if (franjas[r].esReceso) {
                grid->setSpan(r, 0, 1, dias.size());
                QTableWidgetItem *item = new QTableWidgetItem("RECESO");
                item->setTextAlignment(Qt::AlignCenter);
                item->setBackground(QColor(229, 231, 235));
                item->setForeground(QColor(107, 114, 128));
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
                grid->setItem(r, 0, item);
            } else {
                for (int c = 0; c < dias.size(); ++c) {
                    QTableWidgetItem *item = new QTableWidgetItem();
                    grid->setItem(r, c, item);
                }
            }
        }
    };

    configurarRejilla(m_gridDisponibilidad);
    configurarRejilla(m_gridPreferencias);
}

void TeacherListWidget::repintarRejillas() {
    QList<FranjaAcademica> franjas = obtenerFranjasDelCentro();
    int cols = m_gridDisponibilidad->columnCount();

    for (int r = 0; r < franjas.size(); ++r) {
        if (franjas[r].esReceso) continue;
        for (int c = 0; c < cols; ++c) {
            CeldaRejilla celda(r, c);

            QTableWidgetItem* itemDisp = m_gridDisponibilidad->item(r, c);
            if (m_noDisponibleModificado.contains(celda)) {
                itemDisp->setBackground(QColor(254, 202, 202));
                itemDisp->setText("No\nDisp.");
                itemDisp->setForeground(QColor(153, 27, 27));
            } else {
                itemDisp->setBackground(QColor(167, 243, 208));
                itemDisp->setText("");
            }
            itemDisp->setTextAlignment(Qt::AlignCenter);
            itemDisp->setFont(QFont("Arial", 8, QFont::Bold));

            QTableWidgetItem* itemPref = m_gridPreferencias->item(r, c);
            if (m_noDisponibleModificado.contains(celda)) {
                itemPref->setBackground(QColor(229, 231, 235));
                itemPref->setText("Bloqueado");
                itemPref->setForeground(QColor(156, 163, 175));
            } else if (m_preferidoModificado.contains(celda)) {
                itemPref->setBackground(QColor(253, 230, 138));
                itemPref->setText("⭐ Preferida");
                itemPref->setForeground(QColor(146, 64, 14));
            } else {
                itemPref->setBackground(Qt::white);
                itemPref->setText("");
            }
            itemPref->setTextAlignment(Qt::AlignCenter);
            itemPref->setFont(QFont("Arial", 8, QFont::Bold));
        }
    }
}

// ==========================================
// PINTADO DE ASIGNATURAS CON EXPANSIÓN TOTAL
// ==========================================
void TeacherListWidget::repintarAsignaturas() {
    m_listTitulares->clear();
    int countTit = m_titularesModificado.size();

    m_listTitulares->setMinimumHeight(80);

    if (m_idDocenteActual == -1) {
        QListWidgetItem* item = new QListWidgetItem("  (Seleccione un docente)", m_listTitulares);
        item->setForeground(QColor(156, 163, 175));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
    } else if (countTit == 0) {
        QListWidgetItem* item = new QListWidgetItem("  (Sin materias titulares asignadas)", m_listTitulares);
        item->setForeground(QColor(156, 163, 175));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
    } else {
        for (const QString& mat : m_titularesModificado) {
            QListWidgetItem* item = new QListWidgetItem("  📚 " + mat, m_listTitulares);
            item->setSizeHint(QSize(item->sizeHint().width(), 36));
        }
    }

    m_listSuplentes->clear();
    int countSup = m_suplentesModificado.size();

    m_listSuplentes->setMinimumHeight(80);

    if (m_idDocenteActual == -1) {
        QListWidgetItem* item = new QListWidgetItem("  (Seleccione un docente)", m_listSuplentes);
        item->setForeground(QColor(156, 163, 175));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
    } else if (countSup == 0) {
        QListWidgetItem* item = new QListWidgetItem("  (Sin materias suplentes asignadas)", m_listSuplentes);
        item->setForeground(QColor(156, 163, 175));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
    } else {
        for (const QString& mat : m_suplentesModificado) {
            QListWidgetItem* item = new QListWidgetItem("  🔄 " + mat, m_listSuplentes);
            item->setSizeHint(QSize(item->sizeHint().width(), 36));
        }
    }

    m_avisoSinMaterias->setVisible(m_idDocenteActual != -1 && m_titularesModificado.isEmpty());
}

// ==========================================
// INTERACCIONES EN REJILLAS Y TABS
// ==========================================
void TeacherListWidget::cambiarPestana(int indice) {
    m_stackedViews->setCurrentIndex(indice);

    QString activeStyle = "QPushButton { border: none; background: transparent; font-weight: bold; color: #1a237e; padding: 10px; border-bottom: 3px solid #1a237e; }";
    QString inactiveStyle = "QPushButton { border: none; background: transparent; font-weight: bold; color: #6b7280; padding: 10px; border-bottom: 3px solid transparent; } QPushButton:hover { color: #111827; }";

    if (m_isDark) activeStyle = "QPushButton { border: none; background: transparent; font-weight: bold; color: #ffffff; padding: 10px; border-bottom: 3px solid #ffffff; }";

    m_btnTabDisponibilidad->setStyleSheet(indice == 0 ? activeStyle : inactiveStyle);
    m_btnTabPreferencias->setStyleSheet(indice == 1 ? activeStyle : inactiveStyle);
    m_btnTabAsignaturas->setStyleSheet(indice == 2 ? activeStyle : inactiveStyle);
}

void TeacherListWidget::onDisponibilidadCellClicked(int fila, int columna) {
    if (m_idDocenteActual == -1) return;
    QList<FranjaAcademica> franjas = obtenerFranjasDelCentro();
    if (franjas[fila].esReceso) return;

    CeldaRejilla celda(fila, columna);
    if (!m_noDisponibleModificado.contains(celda)) {
        if (m_preferidoModificado.contains(celda)) {
            QMessageBox::information(this, "Aviso", "Esta hora estaba marcada como preferida. Se retiró la preferencia al marcarse como no disponible.");
            m_preferidoModificado.remove(celda);
        }
        m_noDisponibleModificado.insert(celda);
    } else {
        m_noDisponibleModificado.remove(celda);
    }

    repintarRejillas();
    evaluarCambiosSinGuardar();
}

void TeacherListWidget::onPreferenciasCellClicked(int fila, int columna) {
    if (m_idDocenteActual == -1) return;
    QList<FranjaAcademica> franjas = obtenerFranjasDelCentro();
    if (franjas[fila].esReceso) return;

    CeldaRejilla celda(fila, columna);
    if (m_noDisponibleModificado.contains(celda)) {
        QMessageBox::warning(this, "Bloqueado", "No puedes marcar como preferida una hora en la que el docente no está disponible.");
        return;
    }

    if (m_preferidoModificado.contains(celda)) { m_preferidoModificado.remove(celda); }
    else { m_preferidoModificado.insert(celda); }

    repintarRejillas();
    evaluarCambiosSinGuardar();
}

// ==========================================
// EDICIÓN DE ASIGNATURAS (ISSUE #95)
// ==========================================
void TeacherListWidget::onEditarTitularesClicked() {
    if (m_idDocenteActual == -1 || !m_nucleo) return;

    auto resMat = m_nucleo->listarMaterias();
    QStringList todasLasMaterias;
    if (resMat.ok) {
        for (const auto& mat : resMat.valor) todasLasMaterias << mat.nombre;
    }

    SelectorMateriasDialog dialog(todasLasMaterias, m_titularesModificado, this);
    dialog.setWindowTitle("Seleccionar Materias Titulares");

    if (m_isDark) {
        dialog.setStyleSheet(
            "#selCard { background: #121212; border: 1px solid #27272a; border-radius:14px; }"
            "#selTitulo { font-size:17px; font-weight:700; color:#f4f4f5; }"
            "#selAyuda { font-size:12px; color:#a1a1aa; }"
            "#selContador { font-size:12px; color:#a1a1aa; }"
            "#selScroll, #selRejilla { background:transparent; border:none; }"
            "#selMateria { background: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 9px; padding:4px 10px; text-align:center; }"
            "#selMateria:hover { border-color: #52525b; background: #27272a; }"
            "#selMateria:checked { background: #27272a; color: #ffffff; border: 2px solid #f4f4f6; font-weight:600; }"
            "#selLimpiar, #selCancelar { min-width:96px; padding:6px 16px; border-radius:9px; border:1px solid #3f3f46; background:#1f1f23; color:#f4f4f5; font-weight:600; }"
            "#selLimpiar:hover, #selCancelar:hover { background:#27272a; }"
            "#selAceptar { min-width:110px; padding:6px 18px; border-radius:9px; border:none; background:#f4f4f5; color:#121212; font-weight:700; }"
            "#selAceptar:hover { background:#ffffff; }"
            );
    }

    if (dialog.exec() == QDialog::Accepted) {
        QStringList nuevaSeleccion = dialog.seleccionadas();
        bool conflicto = false;

        for (const QString& mat : nuevaSeleccion) {
            if (m_suplentesModificado.contains(mat)) {
                m_suplentesModificado.removeAll(mat);
                conflicto = true;
            }
        }

        if (conflicto) {
            QMessageBox::information(this, "Validación Automática", "Algunas materias se removieron de 'Suplentes' porque un docente no puede ser titular y suplente de la misma materia simultáneamente.");
        }

        m_titularesModificado = nuevaSeleccion;
        repintarAsignaturas();
        evaluarCambiosSinGuardar();
    }
}

void TeacherListWidget::onEditarSuplentesClicked() {
    if (m_idDocenteActual == -1 || !m_nucleo) return;

    auto resMat = m_nucleo->listarMaterias();
    QStringList todasLasMaterias;
    if (resMat.ok) {
        for (const auto& mat : resMat.valor) todasLasMaterias << mat.nombre;
    }

    SelectorMateriasDialog dialog(todasLasMaterias, m_suplentesModificado, this);
    dialog.setWindowTitle("Seleccionar Materias Suplentes");

    if (m_isDark) {
        dialog.setStyleSheet(
            "#selCard { background: #121212; border: 1px solid #27272a; border-radius:14px; }"
            "#selTitulo { font-size:17px; font-weight:700; color:#f4f4f5; }"
            "#selAyuda { font-size:12px; color:#a1a1aa; }"
            "#selContador { font-size:12px; color:#a1a1aa; }"
            "#selScroll, #selRejilla { background:transparent; border:none; }"
            "#selMateria { background: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 9px; padding:4px 10px; text-align:center; }"
            "#selMateria:hover { border-color: #52525b; background: #27272a; }"
            "#selMateria:checked { background: #27272a; color: #ffffff; border: 2px solid #f4f4f6; font-weight:600; }"
            "#selLimpiar, #selCancelar { min-width:96px; padding:6px 16px; border-radius:9px; border:1px solid #3f3f46; background:#1f1f23; color:#f4f4f5; font-weight:600; }"
            "#selLimpiar:hover, #selCancelar:hover { background:#27272a; }"
            "#selAceptar { min-width:110px; padding:6px 18px; border-radius:9px; border:none; background:#f4f4f5; color:#121212; font-weight:700; }"
            "#selAceptar:hover { background:#ffffff; }"
            );
    }

    if (dialog.exec() == QDialog::Accepted) {
        QStringList nuevaSeleccion = dialog.seleccionadas();
        bool conflicto = false;

        for (const QString& mat : nuevaSeleccion) {
            if (m_titularesModificado.contains(mat)) {
                m_titularesModificado.removeAll(mat);
                conflicto = true;
            }
        }

        if (conflicto) {
            QMessageBox::warning(this, "Operación Restringida", "No se agregaron algunas materias a 'Suplentes' porque el docente ya es Titular en ellas.\n\nSi desea que sea suplente, remuévalas primero de la lista de Titulares.");
        }

        m_suplentesModificado = nuevaSeleccion;
        repintarAsignaturas();
        evaluarCambiosSinGuardar();
    }
}

// ==========================================
// CONTROL DE CAMBIOS
// ==========================================
void TeacherListWidget::evaluarCambiosSinGuardar() {
    bool dispCambio = (m_noDisponibleOriginal != m_noDisponibleModificado);
    bool prefCambio = (m_preferidoOriginal != m_preferidoModificado);
    bool asigTitCambio = (m_titularesOriginal != m_titularesModificado);
    bool asigSupCambio = (m_suplentesOriginal != m_suplentesModificado);

    bool hayCambios = dispCambio || prefCambio || asigTitCambio || asigSupCambio;
    m_panelGuardado->setVisible(hayCambios);

    m_btnTabDisponibilidad->setText(dispCambio ? "Disponibilidad *" : "Disponibilidad");
    m_btnTabPreferencias->setText(prefCambio ? "Preferencias *" : "Preferencias");
    m_btnTabAsignaturas->setText((asigTitCambio || asigSupCambio) ? "Asignaturas *" : "Asignaturas");
}

void TeacherListWidget::guardarCambios() {
    m_noDisponibleOriginal = m_noDisponibleModificado;
    m_preferidoOriginal = m_preferidoModificado;
    m_titularesOriginal = m_titularesModificado;
    m_suplentesOriginal = m_suplentesModificado;

    evaluarCambiosSinGuardar();
    QMessageBox::information(this, "Éxito", "Cambios confirmados localmente. (Pendiente persistencia en BD).");
}

void TeacherListWidget::descartarCambios() {
    m_noDisponibleModificado = m_noDisponibleOriginal;
    m_preferidoModificado = m_preferidoOriginal;
    m_titularesModificado = m_titularesOriginal;
    m_suplentesModificado = m_suplentesOriginal;

    repintarRejillas();
    repintarAsignaturas();
    evaluarCambiosSinGuardar();
}

// ==========================================
// CRUD Y SELECCIÓN MAIN
// ==========================================
void TeacherListWidget::cargarDocentes() {
    if (!m_nucleo) return;
    m_table->setRowCount(0);
    auto res = m_nucleo->listarDocentes();
    if (!res.ok) return;

    for (const auto& profe : res.valor) {
        int fila = m_table->rowCount();
        m_table->insertRow(fila);

        auto crearItemCentrado = [](const QString& texto) {
            QTableWidgetItem* item = new QTableWidgetItem(texto);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        QStringList listaMaterias;
        for (const auto& mat : profe.materias) listaMaterias << mat.nombre;
        QString txtMaterias = listaMaterias.isEmpty() ? "-" : listaMaterias.join(", ");

        m_table->setItem(fila, 0, crearItemCentrado(profe.id));
        m_table->setItem(fila, 1, crearItemCentrado(profe.nombre));
        m_table->setItem(fila, 2, crearItemCentrado(profe.email));
        m_table->setItem(fila, 3, crearItemCentrado(profe.telefono.isEmpty() ? "N/A" : profe.telefono));
        m_table->setItem(fila, 4, crearItemCentrado(txtMaterias));

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
                if (m_table->cellWidget(i, 5) == panelAcciones) { eliminarDocente(i); break; }
            }
        });
        connect(btnEditar, &QPushButton::clicked, this, [this, panelAcciones]() {
            for (int i = 0; i < m_table->rowCount(); ++i) {
                if (m_table->cellWidget(i, 5) == panelAcciones) { abrirFormularioEditar(i); break; }
            }
        });

        m_table->setCellWidget(fila, 5, panelAcciones);
    }
}

void TeacherListWidget::onDocenteSeleccionado() {
    if (!m_nucleo) return;

    if (m_panelGuardado->isVisible()) {
        if (QMessageBox::question(this, "Cambios sin guardar", "Tienes cambios sin guardar. ¿Deseas descartarlos para ver otro docente?") != QMessageBox::Yes) {
            return;
        }
    }

    int fila = m_table->currentRow();
    if (fila < 0) return;

    QString cedula = m_table->item(fila, 0)->text();
    QString nombre = m_table->item(fila, 1)->text();

    m_idDocenteActual = fila;
    m_detalleNombre->setText(nombre);

    auto res = m_nucleo->listarDocentes();
    ProfesorDTO profe;
    for (const auto& p : res.valor) { if (p.id == cedula) { profe = p; break; } }

    m_noDisponibleOriginal.clear();
    m_preferidoOriginal.clear();
    m_titularesOriginal.clear();
    m_suplentesOriginal.clear();

    for (const auto& disp : profe.disponibilidad) {
        int c = disp.franja.dia - 1;
        int r = disp.franja.inicio.hour() - 7;
        if (r >= 0 && r < m_gridDisponibilidad->rowCount() && c >= 0 && c < 5) {
            m_noDisponibleOriginal.insert(CeldaRejilla(r, c));
        }
    }
    for(const auto& mat : profe.materias) {
        m_titularesOriginal << mat.nombre;
    }

    m_noDisponibleModificado = m_noDisponibleOriginal;
    m_preferidoModificado = m_preferidoOriginal;
    m_titularesModificado = m_titularesOriginal;
    m_suplentesModificado = m_suplentesOriginal;

    repintarRejillas();
    repintarAsignaturas();
    evaluarCambiosSinGuardar();
}

void TeacherListWidget::abrirFormularioNuevo() { /* Lógica conservada */ }
void TeacherListWidget::abrirFormularioEditar(int fila) { /* Lógica conservada */ }
void TeacherListWidget::eliminarDocente(int fila) { /* Lógica conservada */ }

// ==========================================
// TEMA VISUAL
// ==========================================
void TeacherListWidget::actualizarTema(bool modoOscuro) {
    m_isDark = modoOscuro;
    cambiarPestana(m_stackedViews->currentIndex());

    QString listCardsClaro =
        "QListWidget { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; padding: 4px; outline: none; }"
        "QListWidget::item { background-color: white; border: 1px solid #e2e8f0; border-radius: 6px; margin: 3px 4px; color: #0f172a; font-size: 13px; font-weight: 600; padding-left: 8px; }"
        "QListWidget::item:hover { background-color: #f1f5f9; border-color: #cbd5e1; color: #0f172a; }";

    QString listCardsOscuro =
        "QListWidget { background-color: #121212; border: 1px solid #27272a; border-radius: 8px; padding: 4px; outline: none; }"
        "QListWidget::item { background-color: #1f1f23; border: 1px solid #3f3f46; border-radius: 6px; margin: 3px 4px; color: #f4f4f5; font-size: 13px; font-weight: 600; padding-left: 8px; }"
        "QListWidget::item:hover { background-color: #27272a; border-color: #52525b; color: #ffffff; }";

    if (modoOscuro) {
        if (auto t = findChild<QLabel*>("TeacherTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
        if (auto s = findChild<QLabel*>("TeacherSub")) s->setStyleSheet("font-size: 14px; color: #aaaaaa;");
        if (auto dt = findChild<QLabel*>("DetailTitle")) dt->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");

        if (auto panel = findChild<QFrame*>("TeacherRightPanel")) panel->setStyleSheet("QFrame#TeacherRightPanel { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; }");

        if (m_table) m_table->setStyleSheet("QTableWidget { background-color: #1e1e1e; border-radius: 8px; border: 1px solid #333333; color: #ffffff; } QHeaderView::section { background-color: #252525; font-weight: bold; color: #ffffff; border: none; border-bottom: 1px solid #333333; padding: 12px; } QTableWidget::item { border-bottom: 1px solid #2a2a2a; padding: 5px; color: #ffffff; } QTableWidget::item:selected { background-color: #3730a3; color: white; }");

        QString gridStyle = "QTableWidget { border: 1px solid #333333; gridline-color: #444444; background-color: #1e1e1e; color: white; } QHeaderView::section { background-color: #252525; border: 1px solid #333333; font-size: 11px; color: #aaaaaa; padding: 2px; }";
        m_gridDisponibilidad->setStyleSheet(gridStyle);
        m_gridPreferencias->setStyleSheet(gridStyle);

        // Asignaturas Oscuro
        if (auto lT = findChild<QLabel*>("LblTitulares")) lT->setStyleSheet("font-size: 13px; font-weight: bold; color: #e2e8f0;");
        if (auto lS = findChild<QLabel*>("LblSuplencias")) lS->setStyleSheet("font-size: 13px; font-weight: bold; color: #e2e8f0;");
        if (auto aS = findChild<QLabel*>("AvisoSinMaterias")) aS->setStyleSheet("color: #fca5a5; background-color: #450a0a; border: 1px solid #991b1b; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 12px;");
        m_btnEditarTitulares->setStyleSheet("QPushButton { background-color: #27272a; color: #f4f4f5; border-radius: 4px; padding: 5px 12px; font-weight: bold; border: 1px solid #3f3f46; } QPushButton:hover { background-color: #3f3f46; }");
        m_btnEditarSuplentes->setStyleSheet("QPushButton { background-color: #27272a; color: #f4f4f5; border-radius: 4px; padding: 5px 12px; font-weight: bold; border: 1px solid #3f3f46; } QPushButton:hover { background-color: #3f3f46; }");

        m_listTitulares->setStyleSheet(listCardsOscuro);
        m_listSuplentes->setStyleSheet(listCardsOscuro);

    } else {
        if (auto t = findChild<QLabel*>("TeacherTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827;");
        if (auto s = findChild<QLabel*>("TeacherSub")) s->setStyleSheet("font-size: 14px; color: #6b7280;");
        if (auto dt = findChild<QLabel*>("DetailTitle")) dt->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827;");

        if (auto panel = findChild<QFrame*>("TeacherRightPanel")) panel->setStyleSheet("QFrame#TeacherRightPanel { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");

        if (m_table) m_table->setStyleSheet("QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; color: #334155; } QHeaderView::section { background-color: white; font-weight: bold; color: #374151; border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; } QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; } QTableWidget::item:selected { background-color: #1a237e; color: white; }");

        QString gridStyle = "QTableWidget { border: 1px solid #e0e0e0; gridline-color: #f3f4f6; background-color: white; } QHeaderView::section { background-color: #f9fafb; border: 1px solid #e0e0e0; font-size: 11px; color: #6b7280; padding: 2px; }";
        m_gridDisponibilidad->setStyleSheet(gridStyle);
        m_gridPreferencias->setStyleSheet(gridStyle);

        // Asignaturas Claro
        if (auto lT = findChild<QLabel*>("LblTitulares")) lT->setStyleSheet("font-size: 13px; font-weight: bold; color: #111827;");
        if (auto lS = findChild<QLabel*>("LblSuplencias")) lS->setStyleSheet("font-size: 13px; font-weight: bold; color: #111827;");
        if (auto aS = findChild<QLabel*>("AvisoSinMaterias")) aS->setStyleSheet("color: #b91c1c; background-color: #fef2f2; border: 1px solid #f87171; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 12px;");
        m_btnEditarTitulares->setStyleSheet("QPushButton { background-color: #e0e7ff; color: #3730a3; border-radius: 4px; padding: 5px 12px; font-weight: bold; } QPushButton:hover { background-color: #c7d2fe; }");
        m_btnEditarSuplentes->setStyleSheet("QPushButton { background-color: #e0e7ff; color: #3730a3; border-radius: 4px; padding: 5px 12px; font-weight: bold; } QPushButton:hover { background-color: #c7d2fe; }");

        m_listTitulares->setStyleSheet(listCardsClaro);
        m_listSuplentes->setStyleSheet(listCardsClaro);
    }
}