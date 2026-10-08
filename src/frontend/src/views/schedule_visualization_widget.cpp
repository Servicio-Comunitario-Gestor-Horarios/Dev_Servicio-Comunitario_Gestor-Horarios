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

ScheduleVisualizationWidget::ScheduleVisualizationWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
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

    // Botón: Cargar datos de prueba
    m_btnLoadTestData = new QPushButton("📥  Cargar Horarios de Prueba", buttonsContainer);
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
        "Use 'Cargar Horarios de Prueba' para cargar datos de ejemplo.",
        this
    );
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("font-size: 13px; color: #6b7280; margin-top: 10px;");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);
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
    // Cargar archivo JSON de prueba desde recursos
    QFile file(":/test_data/test_schedules.json");
    QByteArray data;

    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
        file.close();
    } else {
        // Intentar cargar desde ruta relativa (para desarrollo)
        QFile file2("../test_schedules.json");
        if (file2.open(QIODevice::ReadOnly)) {
            data = file2.readAll();
            file2.close();
        } else {
            QMessageBox::warning(this, "Error",
                "No se encontró el archivo de datos de prueba.\n"
                "Asegúrese de que test_schedules.json esté en el directorio del proyecto.");
            return;
        }
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QMessageBox::warning(this, "Error", "El archivo JSON de prueba no es válido.");
        return;
    }

    // Determinar si se cargan horarios de profesores o estudiantes basado en el botón
    // Por defecto cargamos ambos tipos de datos
    QJsonObject obj = doc.object();
    if (obj.contains("teachers")) {
        openTeacherScheduleDialog(obj);
    }
    if (obj.contains("years")) {
        openStudentScheduleDialog(obj);
    }

    QMessageBox::information(this, "Éxito", "Datos de prueba cargados correctamente.\nLos diálogos de horarios se han abierto.");
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