/**
 * @file schedule_visualization_widget.cpp
 * @brief Widget principal para la visualización de horarios
 *        Muestra dos botones: Horarios de Profesores y Horarios de Estudiantes
 */

#include "schedule_visualization_widget.hpp"
#include "teacher_schedule_dialog.hpp"
#include "student_schedule_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

ScheduleVisualizationWidget::ScheduleVisualizationWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    ensureSchedulesDirectory();
}

void ScheduleVisualizationWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(30);
    mainLayout->setContentsMargins(40, 40, 40, 40);

    // Título
    QLabel* titleLabel = new QLabel("Visualización de Horarios", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 28px;"
        "font-weight: bold;"
        "color: #1a237e;"
        "margin-bottom: 20px;"
    );
    mainLayout->addWidget(titleLabel);

    // Contenedor de botones
    QWidget* buttonsContainer = new QWidget(this);
    QVBoxLayout* buttonsLayout = new QVBoxLayout(buttonsContainer);
    buttonsLayout->setSpacing(20);
    buttonsLayout->setAlignment(Qt::AlignCenter);

    // Botón: Horarios de Profesores
    m_btnTeacherSchedules = new QPushButton("🎓  Horarios de Profesores", buttonsContainer);
    m_btnTeacherSchedules->setCursor(Qt::PointingHandCursor);
    m_btnTeacherSchedules->setFixedSize(350, 80);
    m_btnTeacherSchedules->setStyleSheet(
        "QPushButton {"
        "   background-color: #3f51b5;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 12px;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "   padding: 15px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #303f9f;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #283593;"
        "}"
    );
    connect(m_btnTeacherSchedules, &QPushButton::clicked, this, &ScheduleVisualizationWidget::onTeacherSchedulesClicked);
    buttonsLayout->addWidget(m_btnTeacherSchedules, 0, Qt::AlignCenter);

    // Botón: Horarios de Estudiantes
    m_btnStudentSchedules = new QPushButton("👥  Horarios de Estudiantes", buttonsContainer);
    m_btnStudentSchedules->setCursor(Qt::PointingHandCursor);
    m_btnStudentSchedules->setFixedSize(350, 80);
    m_btnStudentSchedules->setStyleSheet(
        "QPushButton {"
        "   background-color: #009688;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 12px;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "   padding: 15px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #00796b;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #00695c;"
        "}"
    );
    connect(m_btnStudentSchedules, &QPushButton::clicked, this, &ScheduleVisualizationWidget::onStudentSchedulesClicked);
    buttonsLayout->addWidget(m_btnStudentSchedules, 0, Qt::AlignCenter);

    // Separador
    QFrame* separator = new QFrame(buttonsContainer);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("color: #e0e0e0; margin: 10px 0;");
    buttonsLayout->addWidget(separator);

    // Botón: Cargar horarios desde archivo
    m_btnLoadTestData = new QPushButton("📁  Cargar Horarios desde Archivo", buttonsContainer);
    m_btnLoadTestData->setCursor(Qt::PointingHandCursor);
    m_btnLoadTestData->setFixedSize(350, 60);
    m_btnLoadTestData->setStyleSheet(
        "QPushButton {"
        "   background-color: #ff9800;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 10px;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   padding: 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #f57c00;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #ef6c00;"
        "}"
    );
    connect(m_btnLoadTestData, &QPushButton::clicked, this, &ScheduleVisualizationWidget::onLoadTestDataClicked);
    buttonsLayout->addWidget(m_btnLoadTestData, 0, Qt::AlignCenter);

    mainLayout->addWidget(buttonsContainer);

    // Etiqueta informativa
    QLabel* infoLabel = new QLabel(
        "Seleccione una opción para visualizar y editar los horarios.\n"
        "Use 'Cargar Horarios desde Archivo' para elegir un archivo JSON de horarios guardados.",
        this
    );
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("font-size: 13px; color: #6b7280; margin-top: 10px;");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);
}

void ScheduleVisualizationWidget::ensureSchedulesDirectory()
{
    // Crear directorio para horarios en AppData/LiceoRobertSerra/GestorHorarios/schedules
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        appDataPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/LiceoRobertSerra";
    }
    
    m_schedulesDir = appDataPath + "/schedules";
    QDir dir;
    if (!dir.exists(m_schedulesDir)) {
        dir.mkpath(m_schedulesDir);
    }
    
    // Copiar el archivo de prueba integrado si no existe ningún archivo
    QDir schedulesDir(m_schedulesDir);
    if (schedulesDir.entryList(QStringList() << "*.json", QDir::Files).isEmpty()) {
        copyEmbeddedTestSchedules();
    }
}

void ScheduleVisualizationWidget::copyEmbeddedTestSchedules()
{
    QFile sourceFile(":/test_data/test_schedules.json");
    if (sourceFile.open(QIODevice::ReadOnly)) {
        QByteArray data = sourceFile.readAll();
        sourceFile.close();
        
        QString destPath = m_schedulesDir + "/test_schedules.json";
        QFile destFile(destPath);
        if (destFile.open(QIODevice::WriteOnly)) {
            destFile.write(data);
            destFile.close();
            qDebug() << "Archivo de prueba copiado a:" << destPath;
        }
    }
}

void ScheduleVisualizationWidget::setClient(InternalClient* client)
{
    m_client = client;
    if (m_client) {
        connect(m_client, &InternalClient::respuestaRecibida, this, &ScheduleVisualizationWidget::onRespuestaRecibida);
    }
}

void ScheduleVisualizationWidget::onTeacherSchedulesClicked()
{
    if (!m_client) {
        QMessageBox::warning(this, "Error", "No hay conexión con el backend.");
        return;
    }

    m_pendingRequest = {{"op", "teacher_schedules_list"}};
    sendRequest("teacher_schedules_list", {});
}

void ScheduleVisualizationWidget::onStudentSchedulesClicked()
{
    if (!m_client) {
        QMessageBox::warning(this, "Error", "No hay conexión con el backend.");
        return;
    }

    m_pendingRequest = {{"op", "student_schedules_list"}};
    sendRequest("student_schedules_list", {});
}

void ScheduleVisualizationWidget::onLoadTestDataClicked()
{
    // Abrir diálogo para seleccionar archivo JSON de horarios
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Seleccionar Archivo de Horarios",
        m_schedulesDir,
        "Archivos JSON (*.json);;Todos los archivos (*.*)"
    );

    if (filePath.isEmpty()) {
        return; // Usuario canceló
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error",
            "No se pudo abrir el archivo seleccionado:\n" + filePath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QMessageBox::warning(this, "Error", "El archivo JSON no es válido.");
        return;
    }

    // Determinar si se cargan horarios de profesores o estudiantes
    QJsonObject obj = doc.object();
    if (obj.contains("teachers")) {
        openTeacherScheduleDialog(obj);
    }
    if (obj.contains("years")) {
        openStudentScheduleDialog(obj);
    }

    QString fileName = QFileInfo(filePath).fileName();
    QMessageBox::information(this, "Éxito", 
        QString("Horarios cargados desde:\n%1\n\nLos diálogos de horarios se han abierto.").arg(fileName));
}

void ScheduleVisualizationWidget::openTeacherScheduleDialog(const QJsonObject& data)
{
    TeacherScheduleDialog* dialog = new TeacherScheduleDialog(data, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::ApplicationModal);

    // Conectar señal de modificación
    connect(dialog, &TeacherScheduleDialog::scheduleModified, [this](const QJsonObject& updatedData) {
        // Aquí se podría enviar al backend para guardar
        qDebug() << "Horario de profesor modificado, enviar al backend:" << updatedData;
    });

    dialog->show();
}

void ScheduleVisualizationWidget::openStudentScheduleDialog(const QJsonObject& data)
{
    StudentScheduleDialog* dialog = new StudentScheduleDialog(data, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowModality(Qt::ApplicationModal);

    // Conectar señal de modificación
    connect(dialog, &StudentScheduleDialog::scheduleModified, [this](const QJsonObject& updatedData) {
        // Aquí se podría enviar al backend para guardar
        qDebug() << "Horario de estudiante modificado, enviar al backend:" << updatedData;
    });

    dialog->show();
}

void ScheduleVisualizationWidget::sendRequest(const QString& op, const QJsonObject& payload)
{
    if (!m_client) return;

    m_client->enviarSolicitud(op, payload);
}

void ScheduleVisualizationWidget::onRespuestaRecibida(const QJsonObject& respuesta)
{
    int code = respuesta["code"].toInt(Middleware::RESP_ERROR);
    QString op = respuesta["op"].toString();

    if (code != Middleware::RESP_EXITO) {
        QString errorMsg = respuesta["message"].toString("Error desconocido");
        QMessageBox::critical(this, "Error", errorMsg);
        return;
    }

    if (op == "teacher_schedules_list") {
        handleTeacherSchedulesResponse(respuesta);
    } else if (op == "student_schedules_list") {
        handleStudentSchedulesResponse(respuesta);
    } else if (op == "load_test_schedules") {
        handleLoadTestDataResponse(respuesta);
    }
}

void ScheduleVisualizationWidget::handleTeacherSchedulesResponse(const QJsonObject& respuesta)
{
    // Abrir diálogo con los horarios recibidos del backend
    if (respuesta.contains("data")) {
        openTeacherScheduleDialog(respuesta["data"].toObject());
    }
}

void ScheduleVisualizationWidget::handleStudentSchedulesResponse(const QJsonObject& respuesta)
{
    // Abrir diálogo con los horarios recibidos del backend
    if (respuesta.contains("data")) {
        openStudentScheduleDialog(respuesta["data"].toObject());
    }
}

void ScheduleVisualizationWidget::handleLoadTestDataResponse(const QJsonObject& respuesta)
{
    QMessageBox::information(this, "Éxito", "Datos de prueba cargados correctamente en el backend.");
}