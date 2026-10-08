/******************************************************************************
 * Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        classroom_form_dialog.cpp
 * Autor:          Paola
 * Fecha:          19 de Julio de 2026
 * Descripción:    Implementación del formulario CRUD de aulas con soporte para modo oscuro.
 ******************************************************************************/

#include "classroom_form_dialog.hpp"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QApplication>

namespace gestor::frontend::forms {

ClassroomFormDialog::ClassroomFormDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Registrar Aula");
    resize(400, 250);

    // Detectar si el modo oscuro está activo
    bool isDark = qApp->property("isDarkMode").toBool();

    QString bgColor = isDark ? "#1e1e1e" : "white";
    QString textColor = isDark ? "#ffffff" : "#374151";
    QString inputBg = isDark ? "#2a2a2a" : "white";
    QString inputBorder = isDark ? "#444444" : "#d1d5db";
    QString placeholderColor = isDark ? "#888888" : "#9ca3af";
    QString labelColor = isDark ? "#ffffff" : "#111827";
    QString arrowColor = isDark ? "#ffffff" : "#4b5563";

    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(bgColor, textColor));

    QString inputStyle = QString(
                             "QLineEdit, QSpinBox { border: 1px solid %1; border-radius: 6px; padding: 8px; padding-right: 24px; background: %2; color: %3; font-size: 13px; }"
                             "QLineEdit::placeholder { color: %4; }"
                             "QSpinBox::up-button, QSpinBox::down-button { subcontrol-origin: padding; border: none; background: %2; width: 16px; margin-right: 2px; }"
                             "QSpinBox::up-button { subcontrol-position: top right; margin-top: 2px; }"
                             "QSpinBox::down-button { subcontrol-position: bottom right; margin-bottom: 2px; }"
                             "QSpinBox::up-arrow, QSpinBox::down-arrow { width: 0; height: 0; background: %2; border-left: 3px solid %2; border-right: 3px solid %2; }"
                             "QSpinBox::up-arrow { border-bottom: 4px solid %5; }"
                             "QSpinBox::down-arrow { border-top: 4px solid %5; }"
                             ).arg(inputBorder, inputBg, textColor, placeholderColor, arrowColor);

    campoNombre = new QLineEdit(this);
    campoNombre->setStyleSheet(inputStyle);
    campoNombre->setPlaceholderText("Ej. Laboratorio 1");

    campoCapacidad = new QSpinBox(this);
    campoCapacidad->setStyleSheet(inputStyle);
    campoCapacidad->setRange(1, 500);
    campoCapacidad->setValue(30);
    campoCapacidad->setButtonSymbols(QAbstractSpinBox::UpDownArrows);

    campoEdificio = new QLineEdit(this);
    campoEdificio->setStyleSheet(inputStyle);
    campoEdificio->setPlaceholderText("Ej. Edificio B (Opcional)");

    campoPiso = new QLineEdit(this);
    campoPiso->setStyleSheet(inputStyle);
    campoPiso->setPlaceholderText("Ej. Planta Baja (Opcional)");

    botonCancelar = new QPushButton("Cancelar", this);
    botonCancelar->setStyleSheet(isDark ?
                                     "QPushButton { background-color: #2a2a2a; border: 1px solid #444444; border-radius: 6px; padding: 10px 20px; color: #ffffff; font-weight: bold; } QPushButton:hover { background-color: #333333; }" :
                                     "QPushButton { background-color: white; border: 1px solid #d1d5db; border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } QPushButton:hover { background-color: #f3f4f6; }"
                                 );
    botonCancelar->setCursor(Qt::PointingHandCursor);

    botonGuardar = new QPushButton("Guardar", this);
    botonGuardar->setStyleSheet(isDark ?
                                    "QPushButton { background-color: #3b82f6; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #2563eb; }" :
                                    "QPushButton { background-color: #1a237e; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #283593; }"
                                );
    botonGuardar->setCursor(Qt::PointingHandCursor);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    QString labelStyle = QString("<span style='font-weight:bold; color:%1; font-size: 13px;'>").arg(labelColor);

    formLayout->addRow(labelStyle + "Nombre:*</span>", campoNombre);
    formLayout->addRow(labelStyle + "Capacidad:*</span>", campoCapacidad);
    formLayout->addRow(labelStyle + "Edificio:</span>", campoEdificio);
    formLayout->addRow(labelStyle + "Piso:</span>", campoPiso);

    QHBoxLayout *botonesLayout = new QHBoxLayout();
    botonesLayout->addStretch();
    botonesLayout->addWidget(botonCancelar);
    botonesLayout->addWidget(botonGuardar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(botonesLayout);

    connect(botonGuardar, &QPushButton::clicked, this, &ClassroomFormDialog::guardarAula);
    connect(botonCancelar, &QPushButton::clicked, this, &QDialog::reject);
}

void ClassroomFormDialog::cargarDatos(const QString& nombre, int capacidad, const QString& edificio, const QString& piso) {
    campoNombre->setText(nombre);
    campoCapacidad->setValue(capacidad);
    campoEdificio->setText(edificio != "N/A" ? edificio : "");
    campoPiso->setText(piso != "N/A" ? piso : "");
}

void ClassroomFormDialog::guardarAula() {
    if (campoNombre->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Error de Validación", "El campo Nombre es obligatorio.");
        return;
    }

    emit aulaGuardada(campoNombre->text().trimmed(), campoCapacidad->value(), campoEdificio->text().trimmed(), campoPiso->text().trimmed());
    accept();
}

} // namespace gestor::frontend::forms