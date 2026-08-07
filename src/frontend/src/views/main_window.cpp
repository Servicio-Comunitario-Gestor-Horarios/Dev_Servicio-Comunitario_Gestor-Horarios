/* Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        [main_window.cpp]
 * Autor:          Paola Peña
 * Fecha:          10 de Julio de 2026
 * Descripción:    Implementación de la interfaz gráfica (Frontend) utilizando
 *                 C++ y Qt6 para el módulo de gestión de docentes. Incluye el
 *                 diseño de la vista principal, la tabla de listado de docentes,
 *                 y el formulario de registro con validación de datos.
 */

#include "main_window.hpp"
#include "teacher_list_widget.hpp"
#include "subject_list_widget.hpp"
#include "dashboard_widget.hpp"
#include "view_placeholder.hpp"
#include "classroom_list_widget.hpp"
#include "../../../middleware/include/middleware/internalclient.h"

#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QPushButton>
#include <QWidgetAction>
#include "../forms/settings_dialog.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Liceo Nacional Robert Serra - Gestión");
    resize(1280, 720);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setObjectName("FondoPrincipal");

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupSidebar();
    setupCentralArea();

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_rightContainer);

    setCentralWidget(centralWidget);

    // Crear cliente y pasar a los widgets
    InternalClient* client = new InternalClient(this);
    if (m_teacherListWidget) m_teacherListWidget->setClient(client);
    if (m_classroomListWidget) m_classroomListWidget->setClient(client);
    if (m_subjectListWidget) m_subjectListWidget->setClient(client);

    // Conectar botones
    connect(m_btnInicio, &QPushButton::clicked, this, &MainWindow::mostrarInicio);
    connect(m_btnDocentes, &QPushButton::clicked, this, &MainWindow::mostrarDocentes);
    connect(m_btnAulas, &QPushButton::clicked, this, &MainWindow::mostrarAulas);
    connect(m_btnAsignaturas, &QPushButton::clicked, this, &MainWindow::mostrarAsignaturas);
    connect(m_btnGeneracion, &QPushButton::clicked, this, &MainWindow::mostrarGeneracion);
    connect(m_btnVisualizacion, &QPushButton::clicked, this, &MainWindow::mostrarVisualizacion);

    // Iniciar el tema por defecto (claro)
    aplicarTemaGlobal(false);
    mostrarInicio();
}

void MainWindow::setupSidebar() {
    m_sidebar = new QFrame(this);
    m_sidebar->setObjectName("Sidebar");
    m_sidebar->setFixedWidth(250);

    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setAlignment(Qt::AlignTop);
    sidebarLayout->setContentsMargins(0, 20, 0, 20);

    QLabel *logoLabel = new QLabel("Liceo Nacional\nRobert Serra", m_sidebar);
    logoLabel->setObjectName("LogoLiceo");
    logoLabel->setStyleSheet("font-weight: bold; font-size: 16px; border: none; background: transparent; margin-left: 20px;");

    m_btnInicio = new QPushButton("🏠 Inicio", m_sidebar);
    m_btnDocentes = new QPushButton("🎓 Docentes", m_sidebar);
    m_btnAulas = new QPushButton("🏫 Aulas", m_sidebar);
    m_btnAsignaturas = new QPushButton("📚 Asignaturas", m_sidebar);
    m_btnGeneracion = new QPushButton("📅 Generación de horario", m_sidebar);
    m_btnVisualizacion = new QPushButton("👁️ Visualización de horario", m_sidebar);

    QList<QPushButton*> botones = {m_btnInicio, m_btnDocentes, m_btnAulas, m_btnAsignaturas, m_btnGeneracion, m_btnVisualizacion};
    for (auto btn : botones) {
        btn->setCursor(Qt::PointingHandCursor);
    }

    QLabel *tituloAcademico = new QLabel("ACADÉMICO", m_sidebar);
    tituloAcademico->setObjectName("TituloAcademico");
    tituloAcademico->setStyleSheet("font-size: 11px; font-weight: bold; border: none; background: transparent; margin-left: 20px; margin-top: 15px;");

    QLabel *tituloHorarios = new QLabel("HORARIOS", m_sidebar);
    tituloHorarios->setObjectName("TituloHorarios");
    tituloHorarios->setStyleSheet("font-size: 11px; font-weight: bold; border: none; background: transparent; margin-left: 20px; margin-top: 15px;");

    sidebarLayout->addWidget(logoLabel);
    sidebarLayout->addSpacing(20);
    sidebarLayout->addWidget(m_btnInicio);
    sidebarLayout->addWidget(tituloAcademico);
    sidebarLayout->addWidget(m_btnDocentes);
    sidebarLayout->addWidget(m_btnAulas);
    sidebarLayout->addWidget(m_btnAsignaturas);
    sidebarLayout->addWidget(tituloHorarios);
    sidebarLayout->addWidget(m_btnGeneracion);
    sidebarLayout->addWidget(m_btnVisualizacion);
}

void MainWindow::setupCentralArea() {
    m_rightContainer = new QWidget(this);
    m_rightContainer->setObjectName("RightContainer");

    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    QFrame *topHeader = new QFrame(this);
    topHeader->setObjectName("TopHeader");
    topHeader->setFixedHeight(60);

    QHBoxLayout *headerLayout = new QHBoxLayout(topHeader);

    QPushButton *btnMenuRayas = new QPushButton("☰", topHeader);
    btnMenuRayas->setObjectName("MenuIcono");
    btnMenuRayas->setCursor(Qt::PointingHandCursor);
    btnMenuRayas->setStyleSheet("QPushButton { font-size: 20px; border: none; background: transparent; padding-left: 15px; }"
                                "QPushButton::menu-indicator { image: none; }");

    QMenu *menuOpciones = new QMenu(this);

    QWidget *headerWidget = new QWidget(this);
    headerWidget->setObjectName("WidgetMenuCorreo");
    QVBoxLayout *menuHeaderLayout = new QVBoxLayout(headerWidget);
    menuHeaderLayout->setContentsMargins(20, 10, 20, 10);
    menuHeaderLayout->setSpacing(2);

    QLabel *lblRole = new QLabel("Administrador", headerWidget);
    lblRole->setObjectName("LblRoleMenu");
    lblRole->setStyleSheet("font-weight: bold; font-size: 15px; background: transparent;");

    QLabel *lblEmail = new QLabel("admin@liceorobertserra.com", headerWidget);
    lblEmail->setObjectName("LblEmailMenu");
    lblEmail->setStyleSheet("font-size: 13px; background: transparent;");

    menuHeaderLayout->addWidget(lblRole);
    menuHeaderLayout->addWidget(lblEmail);

    QWidgetAction *headerAction = new QWidgetAction(menuOpciones);
    headerAction->setDefaultWidget(headerWidget);

    QAction *accionPerfil = new QAction("👤  Mi Perfil", this);
    QAction *accionConfig = new QAction("⚙️  Configuración", this);
    QAction *accionCerrarSesion = new QAction("🚪  Cerrar Sesión", this);

    menuOpciones->addAction(headerAction);
    menuOpciones->addSeparator();
    menuOpciones->addAction(accionPerfil);
    menuOpciones->addAction(accionConfig);
    menuOpciones->addSeparator();
    menuOpciones->addAction(accionCerrarSesion);

    btnMenuRayas->setMenu(menuOpciones);
    connect(accionConfig, &QAction::triggered, this, &MainWindow::abrirConfiguracion);

    QLabel *userProfile = new QLabel("A  Header Subhead", topHeader);
    userProfile->setObjectName("UserProfileText");
    userProfile->setStyleSheet("border: none; background: transparent; padding-right: 15px;");

    headerLayout->addWidget(btnMenuRayas);
    headerLayout->addStretch();
    headerLayout->addWidget(userProfile);

    m_contenedorVistas = new QStackedWidget(this);
    m_contenedorVistas->setObjectName("ContenedorVistas");

    m_contenedorVistas->addWidget(new DashboardWidget(this));

    m_teacherListWidget = new TeacherListWidget(this);
    m_contenedorVistas->addWidget(m_teacherListWidget);

    m_classroomListWidget = new ClassroomListWidget(this);
    m_contenedorVistas->addWidget(m_classroomListWidget);

    m_subjectListWidget = new SubjectListWidget(this);
    m_contenedorVistas->addWidget(m_subjectListWidget);

    m_contenedorVistas->addWidget(new ViewPlaceholder("Generación de Horarios", this));
    m_contenedorVistas->addWidget(new ViewPlaceholder("Visualización de Horarios", this));

    rightLayout->addWidget(topHeader);
    rightLayout->addWidget(m_contenedorVistas);
}

void MainWindow::configurarEstilosMenu() {
    bool isDark = qApp->property("isDarkMode").toBool();
    QString colorBase = isDark ? "#b3b3b3" : "#4b5563";
    QString estiloInactivo = QString("text-align: left; padding: 10px 20px; border: none; font-size: 14px; color: %1; background-color: transparent; border-left: 4px solid transparent;").arg(colorBase);

    m_btnInicio->setStyleSheet(estiloInactivo);
    m_btnDocentes->setStyleSheet(estiloInactivo);
    m_btnAulas->setStyleSheet(estiloInactivo);
    m_btnAsignaturas->setStyleSheet(estiloInactivo);
    m_btnGeneracion->setStyleSheet(estiloInactivo);
    m_btnVisualizacion->setStyleSheet(estiloInactivo);
}

void MainWindow::activarBoton(QPushButton* btn) {
    configurarEstilosMenu();
    bool isDark = qApp->property("isDarkMode").toBool();
    QString bg = isDark ? "#333333" : "#e8eaf6";
    QString color = isDark ? "#ffffff" : "#1a237e";

    btn->setStyleSheet(QString("text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold; background-color: %1; color: %2; border: none; border-left: 4px solid %2;").arg(bg, color));
}

void MainWindow::mostrarInicio() {
    m_contenedorVistas->setCurrentIndex(0);
    activarBoton(m_btnInicio);
}

void MainWindow::mostrarDocentes() {
    m_contenedorVistas->setCurrentIndex(1);
    activarBoton(m_btnDocentes);
}

void MainWindow::mostrarAulas() {
    m_contenedorVistas->setCurrentIndex(2);
    activarBoton(m_btnAulas);
}

void MainWindow::mostrarAsignaturas() {
    m_contenedorVistas->setCurrentIndex(3);
    activarBoton(m_btnAsignaturas);
}

void MainWindow::mostrarGeneracion() {
    m_contenedorVistas->setCurrentIndex(4);
    activarBoton(m_btnGeneracion);
}

void MainWindow::mostrarVisualizacion() {
    m_contenedorVistas->setCurrentIndex(5);
    activarBoton(m_btnVisualizacion);
}

void MainWindow::aplicarTemaGlobal(bool modoOscuro) {
    qApp->setProperty("isDarkMode", modoOscuro);

    if (modoOscuro) {
        // === TEMA OSCURO (NEGRO PURO #121212 Y PANELES GRISES #1e1e1e) ===
        qApp->setStyleSheet(
            // Fondo general negro absoluto
            "QWidget { background-color: #121212; color: #ffffff; }"

            // FORZAR TODOS LOS PANELES, TARJETAS Y TABLAS A GRIS OSCURO (#1e1e1e)
            "QFrame, QDialog, QMenu, QTableWidget, QTableView, QScrollArea { background-color: #1e1e1e; border: 1px solid #333333; border-radius: 8px; }"

            // Excepciones estructurales de la ventana principal
            "#FondoPrincipal { background-color: #121212; border: none; }"
            "#Sidebar { background-color: #121212; border-right: 1px solid #333333; border-radius: 0px; }"
            "#TopHeader { background-color: #121212; border-bottom: 1px solid #333333; border-radius: 0px; }"
            "#RightContainer, #WidgetMenuCorreo, #ContenedorVistas { background: transparent; border: none; }"

            // Textos legibles en blanco y gris claro
            "QLabel { background: transparent; color: #ffffff; border: none; }"
            "#TituloAcademico, #TituloHorarios, #UserProfileText, #LblEmailMenu { color: #aaaaaa; }"
            "#LogoLiceo, #LblRoleMenu { color: #ffffff; }"

            // Elementos internos y sub-paneles dentro de las tarjetas
            "QFrame QWidget { background-color: transparent; color: #ffffff; }"

            // Iconos y Menú desplegable
            "#MenuIcono { background: transparent; color: #ffffff; border: none; }"
            "QMenu::item { background-color: transparent; padding: 8px 40px 8px 20px; color: #ffffff; }"
            "QMenu::item:selected { background-color: #333333; }"
            "QMenu::separator { background: #333333; height: 1px; margin: 4px 0px; }"

            // Pestañas (Settings Dialog)
            "QTabWidget::pane { background-color: #1e1e1e; border: 1px solid #333333; border-radius: 4px; }"
            "QTabBar::tab { background: #121212; color: #aaaaaa; border: 1px solid #333333; padding: 8px 16px; }"
            "QTabBar::tab:selected { background: #1e1e1e; color: #ffffff; font-weight: bold; border-bottom: 2px solid #ffffff; }"
            );
    } else {
        // === TEMA CLARO (BLANCO Y GRIS CLARO ORIGINAL) ===
        qApp->setStyleSheet(
            // Fondo general claro
            "QWidget { background-color: #f4f5f7; color: #334155; }"

            // PANELES, TARJETAS Y TABLAS EN BLANCO
            "QFrame, QDialog, QMenu, QTableWidget, QTableView, QScrollArea { background-color: white; border: 1px solid #e0e0e0; border-radius: 8px; }"

            // Excepciones estructurales
            "#FondoPrincipal { background-color: #f4f5f7; border: none; }"
            "#Sidebar { background-color: white; border: none; border-right: 1px solid #e0e0e0; border-radius: 0px; }"
            "#TopHeader { background-color: white; border: none; border-bottom: 1px solid #e0e0e0; border-radius: 0px; }"
            "#RightContainer, #WidgetMenuCorreo, #ContenedorVistas { background: transparent; border: none; }"

            // Textos legibles
            "QLabel { background: transparent; color: #334155; border: none; }"
            "#TituloAcademico, #TituloHorarios, #UserProfileText, #LblEmailMenu { color: #6b7280; }"
            "#LogoLiceo { color: #1a237e; }"
            "#LblRoleMenu { color: #0f172a; }"

            // Elementos internos
            "QFrame QWidget { background-color: transparent; color: #334155; }"

            // Iconos y Menú
            "#MenuIcono { background: transparent; color: #334155; border: none; }"
            "QMenu::item { background-color: transparent; padding: 8px 40px 8px 20px; color: #1e293b; }"
            "QMenu::item:selected { background-color: #f1f5f9; }"
            "QMenu::separator { background: #e2e8f0; height: 1px; margin: 4px 0px; }"

            // Pestañas
            "QTabWidget::pane { background-color: white; border-top: 1px solid #e2e8f0; border-radius: 0px; }"
            "QTabBar::tab { background: #f8fafc; color: #64748b; border: 1px solid #e2e8f0; padding: 8px 16px; }"
            "QTabBar::tab:selected { background: white; color: #0f172a; font-weight: bold; border-bottom: 2px solid #243c8a; }"
            );
    }

    // Refrescar vista actual
    int index = m_contenedorVistas->currentIndex();
    if (index == 0) mostrarInicio();
    else if (index == 1) mostrarDocentes();
    else if (index == 2) mostrarAulas();
    else if (index == 3) mostrarAsignaturas();
    else if (index == 4) mostrarGeneracion();
    else if (index == 5) mostrarVisualizacion();
}

void MainWindow::abrirConfiguracion() {
    SettingsDialog dialog(this);

    connect(&dialog, &SettingsDialog::themeChanged, this, &MainWindow::aplicarTemaGlobal);

    dialog.exec();
}