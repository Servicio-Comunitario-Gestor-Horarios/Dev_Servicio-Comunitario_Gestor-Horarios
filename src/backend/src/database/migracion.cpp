#include "backend/database/migracion.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

bool Migracion::runAll(QSqlDatabase& db) {

    QSqlQuery query(db);

    const auto exec = [&](const QString& sql) -> bool {
        
        if(!query.exec(sql)) {
            qCritical() << "Error de migracion: " << query.lastError().text();
            return false;
        }

        return true;
    };

    // ── Aulas ──
    if (!exec(
        "CREATE TABLE IF NOT EXISTS Aulas ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL,"
        "capacidad INTEGER NOT NULL CHECK(capacidad > 0),"
        "edificio TEXT,"
        "piso TEXT,"
        "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")"
    )) return false;

    // ── Profesores ──
    if (!exec(
        "CREATE TABLE IF NOT EXISTS Profesores ("
        "id TEXT PRIMARY KEY,"
        "nombre TEXT NOT NULL UNIQUE,"
        "email TEXT NOT NULL UNIQUE,"
        "telefono TEXT,"
        "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")"
    )) return false;

    // ── Materias ──
    if (!exec(
        "CREATE TABLE IF NOT EXISTS Materias ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL,"
        "requisitos TEXT,"
        "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")"
    )) return false;

    // ── Profesor-Materia (N:M) ──
    if (!exec(
        "CREATE TABLE IF NOT EXISTS Profesor_Materia ("
        "id_Profesor TEXT NOT NULL REFERENCES Profesores(id),"
        "id_Materia INTEGER NOT NULL REFERENCES Materias(id),"
        "PRIMARY KEY (id_Profesor, id_Materia)"
        ")"
    )) return false;

    // ── Disponibilidad_Profesor ──
    if (!exec(
        "CREATE TABLE IF NOT EXISTS Disponibilidad_Profesor ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_Profesor TEXT NOT NULL REFERENCES Profesores(id),"
        "dia TEXT NOT NULL,"
        "hora_inicio TEXT NOT NULL,"
        "hora_fin TEXT NOT NULL,"
        "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")"
    )) return false;

    // ── PlanEstudio ──
    if (!exec(
        "CREATE TABLE IF NOT EXISTS PlanEstudio ("
        "codigo TEXT PRIMARY KEY,"
        "nombre TEXT NOT NULL,"
        "descripcion TEXT,"
        "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")"
    )) return false;

    // ── PlanEstudio-Materia (N:M con curso + horas) ──
    if (!exec(
        "CREATE TABLE IF NOT EXISTS PlanEstudio_Materia ("
        "codigo_PlanEstudio TEXT NOT NULL REFERENCES PlanEstudio(codigo),"
        "id_Materia INTEGER NOT NULL REFERENCES Materias(id),"
        "curso INTEGER NOT NULL CHECK(curso > 0),"
        "horas INTEGER NOT NULL CHECK(horas > 0),"
        "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (codigo_PlanEstudio, id_Materia)"
        ")"
    )) return false;

    return true;
}