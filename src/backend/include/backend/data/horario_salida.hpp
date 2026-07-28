#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

/// Asignación en el horario de salida (índices enteros).
struct AsignacionOutput {
    int slot = -1;
    int materia = -1;
    int profesor = -1;
    int aula = -1;

    QJsonObject toJson() const;
    static AsignacionOutput fromJson(const QJsonObject& obj);
};

/// Un día con sus asignaciones.
struct DiaOutput {
    int dia = -1;
    QVector<AsignacionOutput> asignaciones;

    QJsonObject toJson() const;
    static DiaOutput fromJson(const QJsonObject& obj);
};

/// Horario de un curso.
struct CursoOutput {
    QString turno;
    QVector<DiaOutput> dias;

    QJsonObject toJson() const;
    static CursoOutput fromJson(const QJsonObject& obj);
};

/// Metadatos del archivo de salida del solver.
struct MetadataSalida {
    QString fecha_generacion;
    QString configuracion;
    int tiempo_ejecucion_ms = 0;
    int total_asignaciones = 0;
    int cursos_generados = 0;
    int profesores_asignados = 0;
    int conflictos = 0;

    QJsonObject toJson() const;
    static MetadataSalida fromJson(const QJsonObject& obj);
};

/// Horario completo de salida (raíz del JSON).
struct HorarioSalida {
    MetadataSalida metadata;
    QMap<QString, CursoOutput> horarios;

    QJsonObject toJson() const;
    static HorarioSalida fromJson(const QJsonObject& obj);
};

/// Estadísticas derivadas del horario.
struct EstadisticasHorario {
    int total_asignaciones = 0;
    QMap<int, int> asignaciones_por_profesor;
    QMap<int, int> asignaciones_por_aula;
    QMap<QString, int> asignaciones_por_curso;
    QMap<int, int> asignaciones_por_dia;
};
