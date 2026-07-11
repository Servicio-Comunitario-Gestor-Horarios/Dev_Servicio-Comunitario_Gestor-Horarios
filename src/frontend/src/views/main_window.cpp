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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Liceo Nacional Robert Serra - Gestión");
    resize(1280, 720);

    QWidget *centralWidget = new QWidget(this);
    // SOLUCIÓN: Asignar un ID para que el gris no ensucie los demás elementos
    centralWidget->setObjectName("FondoPrincipal");
    centralWidget->setStyleSheet("#FondoPrincipal { background-color: #f4f5f7; }");

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ==========================================
    // 1. BARRA LATERAL (Sidebar)
    // ==========================================
    QFrame *sidebar = new QFrame(this);
    sidebar->setObjectName("Sidebar");
    sidebar->setFixedWidth(250);
    sidebar->setStyleSheet("#Sidebar { background-color: white; border-right: 1px solid #e0e0e0; }");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setAlignment(Qt::AlignTop);
    sidebarLayout->setContentsMargins(0, 20, 0, 20);

    // Añadimos background: transparent a los textos para que se vean limpios
    QLabel *logoLabel = new QLabel("Liceo Nacional\nRobert Serra", sidebar);
    logoLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #1a237e; border: none; background: transparent; margin-left: 20px;");

    btnInicio = new QPushButton("🏠 Inicio", sidebar);
    btnDocentes = new QPushButton("🎓 Docentes", sidebar);

    btnInicio->setCursor(Qt::PointingHandCursor);
    btnDocentes->setCursor(Qt::PointingHandCursor);

    QLabel *tituloAcademico = new QLabel("ACADÉMICO", sidebar);
    tituloAcademico->setStyleSheet("color: #6b7280; font-size: 11px; font-weight: bold; border: none; background: transparent; margin-left: 20px; margin-top: 15px;");
    QLabel *tituloHorarios = new QLabel("HORARIOS", sidebar);
    tituloHorarios->setStyleSheet("color: #6b7280; font-size: 11px; font-weight: bold; border: none; background: transparent; margin-left: 20px; margin-top: 15px;");

    sidebarLayout->addWidget(logoLabel);
    sidebarLayout->addSpacing(20);
    sidebarLayout->addWidget(btnInicio);
    sidebarLayout->addWidget(tituloAcademico);
    sidebarLayout->addWidget(btnDocentes);
    sidebarLayout->addWidget(tituloHorarios);

    // ==========================================
    // 2. ÁREA DERECHA (Header + QStackedWidget)
    // ==========================================
    QWidget *rightContainer = new QWidget(this);
    rightContainer->setObjectName("RightContainer");
    rightContainer->setStyleSheet("#RightContainer { background: transparent; }");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Header superior
    QFrame *topHeader = new QFrame(this);
    topHeader->setObjectName("TopHeader");
    topHeader->setFixedHeight(60);
    topHeader->setStyleSheet("#TopHeader { background-color: white; border-bottom: 1px solid #e0e0e0; }");

    QHBoxLayout *headerLayout = new QHBoxLayout(topHeader);

    // Icono del menú limpio
    QLabel *menuIcon = new QLabel("☰", topHeader);
    menuIcon->setStyleSheet("font-size: 20px; border: none; background: transparent; padding-left: 15px;");

    QLabel *userProfile = new QLabel("A  Header Subhead", topHeader);
    userProfile->setStyleSheet("border: none; background: transparent; color: #555; padding-right: 15px;");

    headerLayout->addWidget(menuIcon);
    headerLayout->addStretch();
    headerLayout->addWidget(userProfile);

    // Contenedor de vistas
    contenedorVistas = new QStackedWidget(this);

    QWidget *vistaInicio = new QWidget();
    vistaInicio->setObjectName("VistaInicio");
    vistaInicio->setStyleSheet("#VistaInicio { background-color: white; border-radius: 10px; margin: 20px; border: 1px solid #e0e0e0; }");

    TeacherListWidget *vistaProfesores = new TeacherListWidget(this);

    contenedorVistas->addWidget(vistaInicio);
    contenedorVistas->addWidget(vistaProfesores);

    rightLayout->addWidget(topHeader);
    rightLayout->addWidget(contenedorVistas);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(rightContainer);

    setCentralWidget(centralWidget);

    connect(btnInicio, &QPushButton::clicked, this, &MainWindow::mostrarInicio);
    connect(btnDocentes, &QPushButton::clicked, this, &MainWindow::mostrarDocentes);

    mostrarInicio();
}

void MainWindow::mostrarInicio() {
    contenedorVistas->setCurrentIndex(0);
    configurarEstilosMenu();

    btnInicio->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::mostrarDocentes() {
    contenedorVistas->setCurrentIndex(1);
    configurarEstilosMenu();

    btnDocentes->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::configurarEstilosMenu() {
    QString estiloInactivo = "text-align: left; padding: 10px 20px; border: none; font-size: 14px; color: #4b5563; background-color: transparent; border-left: 4px solid transparent;";
    btnInicio->setStyleSheet(estiloInactivo);
    btnDocentes->setStyleSheet(estiloInactivo);
}