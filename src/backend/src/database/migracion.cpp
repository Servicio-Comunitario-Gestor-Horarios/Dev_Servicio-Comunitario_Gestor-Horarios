#include "backend/database/migracion.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

namespace
{
    /// Ejecuta una sentencia SQL; registra el error si falla.
    bool ejecutar(QSqlQuery& query, const QString& sql)
    {
        if (!query.exec(sql))
        {
            qCritical() << "Error de migracion:" << query.lastError().text();
            return false;
        }

        return true;
    }

    bool aplicar(QSqlDatabase& db)
    {
        QSqlQuery query(db);

        // ── Aulas ──
        if (!ejecutar(query,
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
        if (!ejecutar(query,
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
        if (!ejecutar(query,
            "CREATE TABLE IF NOT EXISTS Materias ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "nombre TEXT NOT NULL,"
            "requisitos TEXT,"
            "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")"
        )) return false;

        // ── Profesor-Materia (N:M) ──
        if (!ejecutar(query,
            "CREATE TABLE IF NOT EXISTS Profesor_Materia ("
            "id_Profesor TEXT NOT NULL REFERENCES Profesores(id),"
            "id_Materia INTEGER NOT NULL REFERENCES Materias(id),"
            "PRIMARY KEY (id_Profesor, id_Materia)"
            ")"
        )) return false;

        // ── Disponibilidad_Profesor ──
        if (!ejecutar(query,
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
        if (!ejecutar(query,
            "CREATE TABLE IF NOT EXISTS PlanEstudio ("
            "codigo TEXT PRIMARY KEY,"
            "nombre TEXT NOT NULL,"
            "descripcion TEXT,"
            "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")"
        )) return false;

        // ── PlanEstudio-Materia (N:M con curso + horas) ──
        if (!ejecutar(query,
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

        // ── Turnos ──
        if (!ejecutar(query,
            "CREATE TABLE IF NOT EXISTS Turnos ("
            "nombre TEXT PRIMARY KEY,"
            "inicio TEXT NOT NULL,"
            "fin TEXT NOT NULL,"
            "slots INTEGER NOT NULL CHECK(slots > 0),"
            "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ")"
        )) return false;

        // ── Recesos (un receso pertenece a un turno) ──
        if (!ejecutar(query,
            "CREATE TABLE IF NOT EXISTS Recesos ("
            "turno TEXT NOT NULL REFERENCES Turnos(nombre),"
            "despues_de_slot INTEGER NOT NULL CHECK(despues_de_slot >= 0),"
            "duracion INTEGER NOT NULL CHECK(duracion > 0),"
            "inicio TEXT,"
            "fin TEXT,"
            "PRIMARY KEY (turno, despues_de_slot)"
            ")"
        )) return false;

        // ── Cursos ──
        // Identidad (anio, seccion) unica en todo el sistema; `aula` es una
        // referencia opcional a Aulas(id). No existe `Curso_Materia`: las materias
        // del curso se derivan de su plan (spec 005).
        if (!ejecutar(query,
            "CREATE TABLE IF NOT EXISTS Cursos ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "anio INTEGER NOT NULL CHECK(anio >= 1),"
            "seccion TEXT NOT NULL CHECK(length(seccion) = 1),"
            "turno TEXT REFERENCES Turnos(nombre),"
            "aula INTEGER REFERENCES Aulas(id),"
            "fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "UNIQUE(anio, seccion)"
            ")"
        )) return false;

        return true;
}
}

bool Migracion::aplicarPaso(QSqlDatabase& db)
{
    return aplicar(db);
}
