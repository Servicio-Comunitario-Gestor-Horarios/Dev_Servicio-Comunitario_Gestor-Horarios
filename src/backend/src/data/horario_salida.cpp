#include "backend/data/horario_salida.hpp"
#include <QJsonArray>
#include <QJsonDocument>

// --- AsignacionOutput ---
QJsonObject AsignacionOutput::toJson() const {
    QJsonObject obj;
    obj["slot"] = slot;
    obj["materia"] = materia;
    obj["profesor"] = profesor;
    obj["aula"] = aula;
    return obj;
}

AsignacionOutput AsignacionOutput::fromJson(const QJsonObject& obj) {
    AsignacionOutput a;
    a.slot = obj["slot"].toInt(-1);
    a.materia = obj["materia"].toInt(-1);
    a.profesor = obj["profesor"].toInt(-1);
    a.aula = obj["aula"].toInt(-1);
    return a;
}

// --- DiaOutput ---
QJsonObject DiaOutput::toJson() const {
    QJsonObject obj;
    obj["dia"] = dia;
    QJsonArray arreglo;
    for (const auto& a : asignaciones) {
        arreglo.append(a.toJson());
    }
    obj["asignaciones"] = arreglo;
    return obj;
}

DiaOutput DiaOutput::fromJson(const QJsonObject& obj) {
    DiaOutput d;
    d.dia = obj["dia"].toInt(-1);
    for (const auto& v : obj["asignaciones"].toArray()) {
        d.asignaciones.append(AsignacionOutput::fromJson(v.toObject()));
    }
    return d;
}

// --- CursoOutput ---
QJsonObject CursoOutput::toJson() const {
    QJsonObject obj;
    obj["turno"] = turno;
    QJsonArray arreglo;
    for (const auto& d : dias) {
        arreglo.append(d.toJson());
    }
    obj["dias"] = arreglo;
    return obj;
}

CursoOutput CursoOutput::fromJson(const QJsonObject& obj) {
    CursoOutput c;
    c.turno = obj["turno"].toString();
    for (const auto& v : obj["dias"].toArray()) {
        c.dias.append(DiaOutput::fromJson(v.toObject()));
    }
    return c;
}

// --- MetadataSalida ---
QJsonObject MetadataSalida::toJson() const {
    QJsonObject obj;
    obj["fecha_generacion"] = fecha_generacion;
    obj["configuracion"] = configuracion;
    obj["tiempo_ejecucion_ms"] = tiempo_ejecucion_ms;
    obj["total_asignaciones"] = total_asignaciones;
    obj["cursos_generados"] = cursos_generados;
    obj["profesores_asignados"] = profesores_asignados;
    obj["conflictos"] = conflictos;
    return obj;
}

MetadataSalida MetadataSalida::fromJson(const QJsonObject& obj) {
    MetadataSalida m;
    m.fecha_generacion = obj["fecha_generacion"].toString();
    m.configuracion = obj["configuracion"].toString();
    m.tiempo_ejecucion_ms = obj["tiempo_ejecucion_ms"].toInt(0);
    m.total_asignaciones = obj["total_asignaciones"].toInt(0);
    m.cursos_generados = obj["cursos_generados"].toInt(0);
    m.profesores_asignados = obj["profesores_asignados"].toInt(0);
    m.conflictos = obj["conflictos"].toInt(0);
    return m;
}

// --- HorarioSalida ---
QJsonObject HorarioSalida::toJson() const {
    QJsonObject obj;
    obj["metadata"] = metadata.toJson();
    QJsonObject horariosObj;
    for (auto it = horarios.constBegin(); it != horarios.constEnd(); ++it) {
        horariosObj[it.key()] = it.value().toJson();
    }
    obj["horarios"] = horariosObj;
    return obj;
}

HorarioSalida HorarioSalida::fromJson(const QJsonObject& obj) {
    HorarioSalida h;
    h.metadata = MetadataSalida::fromJson(obj["metadata"].toObject());
    QJsonObject horariosObj = obj["horarios"].toObject();
    for (auto it = horariosObj.constBegin(); it != horariosObj.constEnd(); ++it) {
        h.horarios[it.key()] = CursoOutput::fromJson(it.value().toObject());
    }
    return h;
}
