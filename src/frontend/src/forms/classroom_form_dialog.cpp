/******************************************************************************
 * Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        classroom_form_dialog.cpp
 * Autor:          Paola
 * Fecha:          19 de Julio de 2026
 * Descripción:    Implementación del formulario CRUD de aulas.
 ******************************************************************************/

#include "classroom_form_dialog.hpp"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>

namespace gestor::frontend::forms {

ClassroomFormDialog::ClassroomFormDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Registrar Aula");
    resize(400, 250);
    setStyleSheet("QDialog { background-color: white; }");

    QString inputStyle =
        /* 1. Caja principal */
        "QLineEdit, QSpinBox { border: 1px solid #d1d5db; border-radius: 6px; padding: 8px; padding-right: 24px; background: white; color: #374151; font-size: 13px; }"
        "QLineEdit::placeholder { color: #9ca3af; }"

        /* 2. Botones  */
        "QSpinBox::up-button, QSpinBox::down-button { subcontrol-origin: padding; border: none; background: white; width: 16px; margin-right: 2px; }"
        "QSpinBox::up-button { subcontrol-position: top right; margin-top: 2px; }"
        "QSpinBox::down-button { subcontrol-position: bottom right; margin-bottom: 2px; }"

        /* 3. Triángulos */
        "QSpinBox::up-arrow, QSpinBox::down-arrow { width: 0; height: 0; background: white; border-left: 3px solid white; border-right: 3px solid white; }"
        "QSpinBox::up-arrow { border-bottom: 4px solid #4b5563; }"
        "QSpinBox::down-arrow { border-top: 4px solid #4b5563; }";

    campoNombre = new QLineEdit(this);
    campoNombre->setStyleSheet(inputStyle);
    campoNombre->setPlaceholderText("Ej. Laboratorio 1");

    campoCapacidad = new QSpinBox(this);
    campoCapacidad->setStyleSheet(inputStyle);
    campoCapacidad->setRange(1, 500); // Rango exigido por el ticket
    campoCapacidad->setValue(30);
    campoCapacidad->setButtonSymbols(QAbstractSpinBox::UpDownArrows);

    campoEdificio = new QLineEdit(this);
    campoEdificio->setStyleSheet(inputStyle);
    campoEdificio->setPlaceholderText("Ej. Edificio B (Opcional)");

    campoPiso = new QLineEdit(this);
    campoPiso->setStyleSheet(inputStyle);
    campoPiso->setPlaceholderText("Ej. Planta Baja (Opcional)");

    botonCancelar = new QPushButton("Cancelar", this);
    botonCancelar->setStyleSheet("QPushButton { background-color: white; border: 1px solid #d1d5db; border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } QPushButton:hover { background-color: #f3f4f6; }");
    botonCancelar->setCursor(Qt::PointingHandCursor);

    botonGuardar = new QPushButton("Guardar", this);
    botonGuardar->setStyleSheet("QPushButton { background-color: #1a237e; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #283593; }");
    botonGuardar->setCursor(Qt::PointingHandCursor);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    QString labelStyle = "<span style='font-weight:bold; color:#111827; font-size: 13px;'>";

    // El asterisco indica que son obligatorios
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
    // Validación: nombre no vacío
    if (campoNombre->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Error de Validación", "El campo Nombre es obligatorio.");
        return;
    }

    // Emitimos la señal con los datos validados
    emit aulaGuardada(campoNombre->text().trimmed(), campoCapacidad->value(), campoEdificio->text().trimmed(), campoPiso->text().trimmed());
    accept();
}

} // namespace gestor::frontend::forms
