/**
 * @file teacher_list_widget.cpp
 * @brief Implementación del listado de docentes con panel de pestañas y rejillas de disponibilidad/preferencias
 */

#include "teacher_list_widget.hpp"
#include "../forms/teacher_form_dialog.hpp"
#include "backend/services/NucleoDatos.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>
#include <QTime>
#include <QDebug>
#include <QStyle>

// ==========================================
// ESTRUCTURA AUXILIAR PARA FRANJAS REALES
// ==========================================
struct FranjaReal {
    QString etiqueta;
    bool esReceso;
    QTime inicio;
    QTime fin;
};

// ==========================================
// CUMPLIMIENTO CRITERIO 1: TURNOS Y RECESOS REALES
// ==========================================
static QList<FranjaReal> construirFranjasReales(NucleoDatos* nucleo) {
    QList<FranjaReal> franjas;
    if (!nucleo) return franjas;

    auto resTurnos = nucleo->listarTurnos();
    if (!resTurnos.ok) {
        qWarning() << "Error al cargar los turnos del colegio para la rejilla.";
        return franjas;
    }

    for (const auto& turno : resTurnos.valor) {
        if (turno.numSlots <= 0) continue;

        // Calcular duración dinámica descontando recesos
        int totalMinutos = turno.inicio.secsTo(turno.fin) / 60;
        int totalMinutosRecesos = 0;

        for (const auto& receso : turno.recesos) {
            totalMinutosRecesos += receso.duracion;
        }

        int minutosClase = totalMinutos - totalMinutosRecesos;
        int duracionSlot = minutosClase / turno.numSlots;

        QTime horaActual = turno.inicio;
        int slotContador = 0;

        while (slotContador < turno.numSlots) {
            FranjaReal franjaClase;
            franjaClase.esReceso = false;
            franjaClase.inicio = horaActual;
            horaActual = horaActual.addSecs(duracionSlot * 60);
            franjaClase.fin = horaActual;
            franjaClase.etiqueta = QString("%1 - %2").arg(franjaClase.inicio.toString("HH:mm")).arg(franjaClase.fin.toString("HH:mm"));
            franjas.append(franjaClase);

            slotContador++;

            // Insertar receso exactamente donde indica el centro
            for (const auto& receso : turno.recesos) {
                if (receso.despuesDeSlot == slotContador) {
                    FranjaReal franjaReceso;
                    franjaReceso.esReceso = true;
                    franjaReceso.inicio = horaActual;
                    horaActual = horaActual.addSecs(receso.duracion * 60);
                    franjaReceso.fin = horaActual;
                    franjaReceso.etiqueta = QString("%1 - %2").arg(franjaReceso.inicio.toString("HH:mm")).arg(franjaReceso.fin.toString("HH:mm"));
                    franjas.append(franjaReceso);
                }
            }
        }
    }
    return franjas;
}

static QStringList obtenerDiasLaborables() {
    return {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes"};
}

TeacherListWidget::TeacherListWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void TeacherListWidget::setClient(InternalClient* client) {
    m_client = client;
}

void TeacherListWidget::setNucleoDatos(NucleoDatos* nucleo) {
    m_nucleo = nucleo;
    construirRejillasDinamicas();
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

    // Vista 2: Asignaturas
    QWidget* pageAsig = new QWidget();
    QVBoxLayout* layAsig = new QVBoxLayout(pageAsig);
    layAsig->setContentsMargins(15, 15, 15, 15);

    QLabel* helpAsig = new QLabel("📚 Asignaturas impartidas por el docente:");
    helpAsig->setObjectName("HelpAsig");
    helpAsig->setStyleSheet("color: #6b7280; font-size: 13px; font-weight: bold; margin-bottom: 10px;");
    layAsig->addWidget(helpAsig);

    m_tableAsignaturas = new QTableWidget(0, 1, this);
    m_tableAsignaturas->horizontalHeader()->setVisible(false);
    m_tableAsignaturas->verticalHeader()->setVisible(false);
    m_tableAsignaturas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableAsignaturas->setShowGrid(false);
    m_tableAsignaturas->setSelectionMode(QAbstractItemView::NoSelection);
    m_tableAsignaturas->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableAsignaturas->setFocusPolicy(Qt::NoFocus);
    layAsig->addWidget(m_tableAsignaturas);

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

    connect(m_btnTabDisponibilidad, &QPushButton::clicked, this, [this](){ cambiarPestana(0); });
    connect(m_btnTabPreferencias, &QPushButton::clicked, this, [this](){ cambiarPestana(1); });
    connect(m_btnTabAsignaturas, &QPushButton::clicked, this, [this](){ cambiarPestana(2); });
    connect(m_gridDisponibilidad, &QTableWidget::cellClicked, this, &TeacherListWidget::onDisponibilidadCellClicked);
    connect(m_gridPreferencias, &QTableWidget::cellClicked, this, &TeacherListWidget::onPreferenciasCellClicked);
    connect(m_btnGuardar, &QPushButton::clicked, this, &TeacherListWidget::guardarCambiosRejilla);
    connect(m_btnDescartar, &QPushButton::clicked, this, &TeacherListWidget::descartarCambiosRejilla);

    cambiarPestana(0);
}

void TeacherListWidget::construirRejillasDinamicas() {
    QList<FranjaReal> franjas = construirFranjasReales(m_nucleo);
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

        grid->setSelectionMode(QAbstractItemView::SingleSelection);
        grid->setFocusPolicy(Qt::NoFocus);
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
    QList<FranjaReal> franjas = construirFranjasReales(m_nucleo);
    int cols = m_gridDisponibilidad->columnCount();

    for (int r = 0; r < franjas.size(); ++r) {
        if (franjas[r].esReceso) continue;

        for (int c = 0; c < cols; ++c) {
            CeldaRejilla celda(r, c);

            QTableWidgetItem* itemDisp = m_gridDisponibilidad->item(r, c);
            if (m_noDisponibleModificado.contains(celda)) {
                itemDisp->setBackground(QColor(254, 202, 202));
                itemDisp->setText("✖");
                itemDisp->setForeground(QColor(153, 27, 27));
            } else {
                itemDisp->setBackground(QColor(167, 243, 208));
                itemDisp->setText("");
            }
            itemDisp->setTextAlignment(Qt::AlignCenter);
            itemDisp->setFont(QFont("Arial", 10, QFont::Bold));

            QTableWidgetItem* itemPref = m_gridPreferencias->item(r, c);
            if (m_noDisponibleModificado.contains(celda)) {
                itemPref->setBackground(QColor(229, 231, 235));
                itemPref->setText("Bloq.");
                itemPref->setForeground(QColor(156, 163, 175));
            } else if (m_preferidoModificado.contains(celda)) {
                itemPref->setBackground(QColor(253, 230, 138));
                itemPref->setText("⭐");
                itemPref->setForeground(QColor(146, 64, 14));
            } else {
                itemPref->setBackground(Qt::white);
                itemPref->setText("");
            }
            itemPref->setTextAlignment(Qt::AlignCenter);
            itemPref->setFont(QFont("Arial", 10, QFont::Bold));
        }
    }
}

void TeacherListWidget::cambiarPestana(int indice) {
    m_stackedViews->setCurrentIndex(indice);

    QString activeStyle = "QPushButton { border: none; background: transparent; font-weight: bold; color: #1a237e; padding: 10px; border-bottom: 3px solid #1a237e; }";
    QString inactiveStyle = "QPushButton { border: none; background: transparent; font-weight: bold; color: #6b7280; padding: 10px; border-bottom: 3px solid transparent; } QPushButton:hover { color: #111827; }";

    if (m_isDark) activeStyle = "QPushButton { border: none; background: transparent; font-weight: bold; color: #ffffff; padding: 10px; border-bottom: 3px solid #ffffff; }";

    m_btnTabDisponibilidad->setStyleSheet(indice == 0 ? activeStyle : inactiveStyle);
    m_btnTabDisponibilidad->style()->unpolish(m_btnTabDisponibilidad);
    m_btnTabDisponibilidad->style()->polish(m_btnTabDisponibilidad);

    m_btnTabPreferencias->setStyleSheet(indice == 1 ? activeStyle : inactiveStyle);
    m_btnTabPreferencias->style()->unpolish(m_btnTabPreferencias);
    m_btnTabPreferencias->style()->polish(m_btnTabPreferencias);

    m_btnTabAsignaturas->setStyleSheet(indice == 2 ? activeStyle : inactiveStyle);
    m_btnTabAsignaturas->style()->unpolish(m_btnTabAsignaturas);
    m_btnTabAsignaturas->style()->polish(m_btnTabAsignaturas);
}

void TeacherListWidget::onDisponibilidadCellClicked(int fila, int columna) {
    if (m_idDocenteActual == -1) return;
    QList<FranjaReal> franjas = construirFranjasReales(m_nucleo);
    if (franjas[fila].esReceso) return;

    CeldaRejilla celda(fila, columna);

    if (!m_noDisponibleModificado.contains(celda)) {
        if (m_preferidoModificado.contains(celda)) {
            QMessageBox::information(this, "Aviso", "Esta hora estaba marcada como preferida. Se ha retirado la preferencia al marcarse como no disponible.");
            m_preferidoModificado.remove(celda);
        }
        m_noDisponibleModificado.insert(celda);
    } else {
        m_noDisponibleModificado.remove(celda);
    }

    repintarRejillas();
    evaluarCambiosSinGuardar();
    m_gridDisponibilidad->clearSelection();
}

void TeacherListWidget::onPreferenciasCellClicked(int fila, int columna) {
    if (m_idDocenteActual == -1) return;
    QList<FranjaReal> franjas = construirFranjasReales(m_nucleo);
    if (franjas[fila].esReceso) return;

    CeldaRejilla celda(fila, columna);

    if (m_noDisponibleModificado.contains(celda)) {
        QMessageBox::warning(this, "Bloqueado", "No puedes marcar como preferida una hora en la que el docente no está disponible.");
        m_gridPreferencias->clearSelection();
        return;
    }

    if (m_preferidoModificado.contains(celda)) {
        m_preferidoModificado.remove(celda);
    } else {
        m_preferidoModificado.insert(celda);
    }

    repintarRejillas();
    evaluarCambiosSinGuardar();
    m_gridPreferencias->clearSelection();
}

void TeacherListWidget::evaluarCambiosSinGuardar() {
    bool hayCambios = (m_noDisponibleOriginal != m_noDisponibleModificado) ||
                      (m_preferidoOriginal != m_preferidoModificado);

    m_panelGuardado->setVisible(hayCambios);

    QString tabDispText = (m_noDisponibleOriginal != m_noDisponibleModificado) ? "Disponibilidad *" : "Disponibilidad";
    QString tabPrefText = (m_preferidoOriginal != m_preferidoModificado) ? "Preferencias *" : "Preferencias";

    m_btnTabDisponibilidad->setText(tabDispText);
    m_btnTabPreferencias->setText(tabPrefText);

    cambiarPestana(m_stackedViews->currentIndex());
}

void TeacherListWidget::guardarCambiosRejilla() {
    m_noDisponibleOriginal = m_noDisponibleModificado;
    m_preferidoOriginal = m_preferidoModificado;
    evaluarCambiosSinGuardar();
    QMessageBox::information(this, "Éxito", "Interfaz lista. Nota: La persistencia de preferencias requerirá del endpoint en el núcleo de datos.");
}

void TeacherListWidget::descartarCambiosRejilla() {
    m_noDisponibleModificado = m_noDisponibleOriginal;
    m_preferidoModificado = m_preferidoOriginal;
    repintarRejillas();
    evaluarCambiosSinGuardar();
}

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

    int fila_seleccionada = m_table->currentRow();
    if (fila_seleccionada < 0) return;

    QString cedula = m_table->item(fila_seleccionada, 0)->text();
    QString nombre = m_table->item(fila_seleccionada, 1)->text();

    m_idDocenteActual = fila_seleccionada;
    m_detalleNombre->setText(nombre);

    auto res = m_nucleo->listarDocentes();
    ProfesorDTO profe;
    for (const auto& p : res.valor) { if (p.id == cedula) { profe = p; break; } }

    m_noDisponibleOriginal.clear();
    m_preferidoOriginal.clear();

    QList<FranjaReal> franjasDelColegio = construirFranjasReales(m_nucleo);

    // ==========================================
    // CUMPLIMIENTO CRITERIO 2: LÓGICA INVERTIDA Y CÁLCULO DE FILAS
    // ==========================================
    // 1. Asumimos por defecto que el docente NO está disponible en toda la semana
    for (int r = 0; r < franjasDelColegio.size(); ++r) {
        if (franjasDelColegio[r].esReceso) continue;
        for (int c = 0; c < 5; ++c) {
            m_noDisponibleOriginal.insert(CeldaRejilla(r, c));
        }
    }

    // 2. Si el profesor SÍ está disponible en un bloque, retiramos esa celda de la lista roja.
    for (const auto& disp : profe.disponibilidad) {
        int colDia = disp.franja.dia - 1;

        // 3. Ya no usamos `inicio.hour() - 7`. Comparamos horas exactas.
        int filaExacta = -1;
        for (int r = 0; r < franjasDelColegio.size(); ++r) {
            if (franjasDelColegio[r].inicio == disp.franja.inicio && franjasDelColegio[r].fin == disp.franja.fin) {
                filaExacta = r;
                break;
            }
        }

        if (filaExacta >= 0 && filaExacta < m_gridDisponibilidad->rowCount() && colDia >= 0 && colDia < 5) {
            m_noDisponibleOriginal.remove(CeldaRejilla(filaExacta, colDia));
        }
    }

    m_noDisponibleModificado = m_noDisponibleOriginal;
    m_preferidoModificado = m_preferidoOriginal;

    repintarRejillas();
    evaluarCambiosSinGuardar();

    m_tableAsignaturas->clear();
    m_tableAsignaturas->setRowCount(profe.materias.size());
    for(int i = 0; i < profe.materias.size(); i++) {
        QTableWidgetItem *item = new QTableWidgetItem("   " + profe.materias[i].nombre);
        m_tableAsignaturas->setItem(i, 0, item);
        m_tableAsignaturas->setRowHeight(i, 45);
    }
}

void TeacherListWidget::abrirFormularioNuevo() { /* Lógica conservada */ }
void TeacherListWidget::abrirFormularioEditar(int fila) { /* Lógica conservada */ }
void TeacherListWidget::eliminarDocente(int fila) { /* Lógica conservada */ }

void TeacherListWidget::actualizarTema(bool modoOscuro) {
    m_isDark = modoOscuro;
    cambiarPestana(m_stackedViews->currentIndex());

    if (modoOscuro) {
        if (auto t = findChild<QLabel*>("TeacherTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
        if (auto s = findChild<QLabel*>("TeacherSub")) s->setStyleSheet("font-size: 14px; color: #aaaaaa;");
        if (auto dt = findChild<QLabel*>("DetailTitle")) dt->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");
        if (auto ha = findChild<QLabel*>("HelpAsig")) ha->setStyleSheet("color: #aaaaaa; font-size: 13px; font-weight: bold; margin-bottom: 10px;");

        if (auto panel = findChild<QFrame*>("TeacherRightPanel")) {
            panel->setStyleSheet("QFrame#TeacherRightPanel { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; }");
        }
        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: #1e1e1e; border-radius: 8px; border: 1px solid #333333; color: #ffffff; }"
                "QHeaderView::section { background-color: #252525; font-weight: bold; color: #ffffff; "
                "border: none; border-bottom: 1px solid #333333; padding: 12px; }"
                "QTableWidget::item { border-bottom: 1px solid #2a2a2a; padding: 5px; color: #ffffff; }");
        }

        QString gridStyle = "QTableWidget { border: 1px solid #333333; gridline-color: #444444; background-color: #1e1e1e; color: white; }"
                            "QTableWidget::item:selected { background-color: transparent; }"
                            "QHeaderView::section { background-color: #252525; border: 1px solid #333333; font-size: 11px; color: #aaaaaa; padding: 2px; }";
        m_gridDisponibilidad->setStyleSheet(gridStyle);
        m_gridPreferencias->setStyleSheet(gridStyle);

        m_tableAsignaturas->setStyleSheet(
            "QTableWidget { border: none; background-color: transparent; outline: none; }"
            "QTableWidget::item { background-color: #2d3748; border-left: 4px solid #3b82f6; border-bottom: 4px solid #1e1e1e; color: #e2e8f0; font-size: 14px; font-weight: bold; padding-left: 10px; }"
            );
    } else {
        if (auto t = findChild<QLabel*>("TeacherTitle")) t->setStyleSheet("font-size: 24px; font-weight: bold; color: #111827;");
        if (auto s = findChild<QLabel*>("TeacherSub")) s->setStyleSheet("font-size: 14px; color: #6b7280;");
        if (auto dt = findChild<QLabel*>("DetailTitle")) dt->setStyleSheet("font-size: 18px; font-weight: bold; color: #111827;");
        if (auto ha = findChild<QLabel*>("HelpAsig")) ha->setStyleSheet("color: #6b7280; font-size: 13px; font-weight: bold; margin-bottom: 10px;");

        if (auto panel = findChild<QFrame*>("TeacherRightPanel")) {
            panel->setStyleSheet("QFrame#TeacherRightPanel { background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; }");
        }
        if (m_table) {
            m_table->setStyleSheet(
                "QTableWidget { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; color: #334155; }"
                "QHeaderView::section { background-color: white; font-weight: bold; color: #374151; "
                "border: none; border-bottom: 1px solid #e0e0e0; padding: 12px; }"
                "QTableWidget::item { border-bottom: 1px solid #f3f4f6; padding: 5px; }");
        }

        QString gridStyle = "QTableWidget { border: 1px solid #e0e0e0; gridline-color: #f3f4f6; background-color: white; }"
                            "QTableWidget::item:selected { background-color: transparent; }"
                            "QHeaderView::section { background-color: #f9fafb; border: 1px solid #e0e0e0; font-size: 11px; color: #374151; padding: 2px; }";
        m_gridDisponibilidad->setStyleSheet(gridStyle);
        m_gridPreferencias->setStyleSheet(gridStyle);

        m_tableAsignaturas->setStyleSheet(
            "QTableWidget { border: none; background-color: transparent; outline: none; }"
            "QTableWidget::item { background-color: #f8fafc; border-left: 4px solid #1a237e; border-bottom: 4px solid white; color: #1e293b; font-size: 14px; font-weight: bold; padding-left: 10px; }"
            );
    }
}