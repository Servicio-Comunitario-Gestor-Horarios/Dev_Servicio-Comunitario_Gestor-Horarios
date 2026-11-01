#pragma once

/**
 * @file schedule_model.hpp
 * @brief Modelo compartido de horarios para los diálogos de profesores y
 *        estudiantes (header-only).
 *
 * Un bloque de horario contiene: materia, profesor, aula y curso (año-sección).
 * El JSON admite compatibilidad con el formato antiguo, en el que el valor era
 * únicamente el nombre de la materia como texto:
 *
 *   Formato antiguo:  { "1": { "0": "Matemáticas" } }
 *   Formato nuevo:    { "1": { "0": { "materia": "Matemáticas",
 *                                     "profesor": "Prof. Juan Pérez",
 *                                     "aula": "Aula 1",
 *                                     "curso": "1°A" } } }
 *
 * Se utiliza "day" 1..5 (Lunes..Viernes) y "slot" 0..11, con una fila visual de
 * receso entre las franjas de la mañana.
 */

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QVector>

namespace ScheduleModel {

// ─────────────────────────────────────────────────────────────────────────────
// Bloques
// ─────────────────────────────────────────────────────────────────────────────

struct Block {
    QString materia;
    QString profesor;
    QString aula;
    QString curso;

    bool isEmpty() const { return materia.trimmed().isEmpty(); }
};

inline QString pickString(const QJsonObject& o, const QStringList& keys)
{
    for (const QString& k : keys) {
        if (o.contains(k)) {
            const QString v = o.value(k).toString();
            if (!v.isEmpty()) return v;
        }
    }
    return QString();
}

/** Interpreta el valor de un bloque, aceptando el formato antiguo (texto). */
inline Block parseBlock(const QJsonValue& v)
{
    Block b;
    if (v.isString()) {
        b.materia = v.toString();
        return b;
    }
    if (v.isObject()) {
        const QJsonObject o = v.toObject();
        b.materia  = pickString(o, {"materia", "subject", "asignatura"});
        b.profesor = pickString(o, {"profesor", "teacher", "docente"});
        b.aula     = pickString(o, {"aula", "classroom", "salon", "salón"});
        b.curso    = pickString(o, {"curso", "seccion", "sección", "section",
                                    "year", "año", "anio", "cursoNombre"});
    }
    return b;
}

inline bool isTrulyEmpty(const Block& b)
{
    return b.materia.trimmed().isEmpty() && b.profesor.isEmpty()
        && b.aula.isEmpty() && b.curso.isEmpty();
}

inline QJsonValue serializeBlock(const Block& b)
{
    if (isTrulyEmpty(b)) return QJsonValue();
    QJsonObject o;
    o.insert("materia", b.materia);
    if (!b.profesor.isEmpty()) o.insert("profesor", b.profesor);
    if (!b.aula.isEmpty())     o.insert("aula", b.aula);
    if (!b.curso.isEmpty())    o.insert("curso", b.curso);
    return o;
}

/** Texto corto para mostrar el bloque en una celda (depuración). */
inline QString blockSummary(const Block& b)
{
    if (b.isEmpty()) return QString();
    return QString("%1 · %2 · %3").arg(b.materia, b.profesor, b.aula);
}

// ─────────────────────────────────────────────────────────────────────────────
// Acceso a una rejilla (schedule = day -> slot -> block)
// ─────────────────────────────────────────────────────────────────────────────

inline Block getBlock(const QJsonObject& schedule, int day, int slot)
{
    const QJsonObject d = schedule.value(QString::number(day)).toObject();
    return parseBlock(d.value(QString::number(slot)));
}

inline void setBlock(QJsonObject& schedule, int day, int slot, const Block& b)
{
    const QString dk = QString::number(day);
    const QString sk = QString::number(slot);
    QJsonObject d = schedule.value(dk).toObject();
    if (isTrulyEmpty(b)) d.remove(sk);
    else                 d.insert(sk, serializeBlock(b));
    if (d.isEmpty()) schedule.remove(dk);
    else             schedule.insert(dk, d);
}

// ─────────────────────────────────────────────────────────────────────────────
// Referencias a un horario dentro del documento completo
// ─────────────────────────────────────────────────────────────────────────────

enum class OwnerKind { Teacher, Section };

struct Ref {
    OwnerKind kind = OwnerKind::Teacher;
    int teacherIndex = -1;
    int yearIndex = -1;
    int sectionIndex = -1;
    QString name;
};

inline bool sameRef(const Ref& a, const Ref& b)
{
    return a.kind == b.kind && a.teacherIndex == b.teacherIndex
        && a.yearIndex == b.yearIndex && a.sectionIndex == b.sectionIndex;
}

inline QJsonObject scheduleOf(const QJsonObject& root, const Ref& r)
{
    if (r.kind == OwnerKind::Teacher) {
        const QJsonArray a = root.value("teachers").toArray();
        if (r.teacherIndex < 0 || r.teacherIndex >= a.size()) return {};
        return a.at(r.teacherIndex).toObject().value("schedule").toObject();
    }
    const QJsonArray ys = root.value("years").toArray();
    if (r.yearIndex < 0 || r.yearIndex >= ys.size()) return {};
    const QJsonArray ss = ys.at(r.yearIndex).toObject().value("sections").toArray();
    if (r.sectionIndex < 0 || r.sectionIndex >= ss.size()) return {};
    return ss.at(r.sectionIndex).toObject().value("schedule").toObject();
}

inline void setSchedule(QJsonObject& root, const Ref& r, const QJsonObject& sched)
{
    if (r.kind == OwnerKind::Teacher) {
        QJsonArray a = root.value("teachers").toArray();
        if (r.teacherIndex < 0 || r.teacherIndex >= a.size()) return;
        QJsonObject t = a.at(r.teacherIndex).toObject();
        t.insert("schedule", sched);
        a[r.teacherIndex] = t;
        root.insert("teachers", a);
        return;
    }
    QJsonArray ys = root.value("years").toArray();
    if (r.yearIndex < 0 || r.yearIndex >= ys.size()) return;
    QJsonObject y = ys.at(r.yearIndex).toObject();
    QJsonArray ss = y.value("sections").toArray();
    if (r.sectionIndex < 0 || r.sectionIndex >= ss.size()) return;
    QJsonObject s = ss.at(r.sectionIndex).toObject();
    s.insert("schedule", sched);
    ss[r.sectionIndex] = s;
    y.insert("sections", ss);
    ys[r.yearIndex] = y;
    root.insert("years", ys);
}

inline QVector<Ref> allRefs(const QJsonObject& root)
{
    QVector<Ref> refs;
    const QJsonArray teachers = root.value("teachers").toArray();
    for (int i = 0; i < teachers.size(); ++i) {
        Ref r; r.kind = OwnerKind::Teacher; r.teacherIndex = i;
        r.name = teachers.at(i).toObject().value("name").toString();
        refs.append(r);
    }
    const QJsonArray years = root.value("years").toArray();
    for (int yi = 0; yi < years.size(); ++yi) {
        const QJsonArray sections = years.at(yi).toObject().value("sections").toArray();
        for (int si = 0; si < sections.size(); ++si) {
            Ref r; r.kind = OwnerKind::Section; r.yearIndex = yi; r.sectionIndex = si;
            r.name = sections.at(si).toObject().value("name").toString();
            refs.append(r);
        }
    }
    return refs;
}

// ─────────────────────────────────────────────────────────────────────────────
// Detección de conflictos
// ─────────────────────────────────────────────────────────────────────────────

struct Conflict {
    Ref other;       ///< Horario donde está el bloque que choca
    int day = 0;
    int slot = 0;
    Block block;     ///< Bloque existente que provoca el conflicto
    QString reason;  ///< "aula" | "profesor" | "aula y profesor"
};

/**
 * Busca bloqueos al colocar `incoming` en (day, slot) del horario `self`.
 * Un conflicto ocurre si en CUALQUIER otro horario (otros profesores, años o
 * secciones) la misma franja día-hora está ocupada por una materia que comparte
 * el mismo aula o el mismo profesor.
 */
inline QVector<Conflict> findConflicts(const QJsonObject& root, const Ref& self,
                                       int day, int slot, const Block& incoming)
{
    QVector<Conflict> out;
    if (incoming.isEmpty()) return out;

    for (const Ref& r : allRefs(root)) {
        if (sameRef(r, self)) continue;
        const Block b = getBlock(scheduleOf(root, r), day, slot);
        if (b.isEmpty()) continue;

        const bool sameAula = !incoming.aula.isEmpty() && !b.aula.isEmpty()
                           && b.aula == incoming.aula;
        const bool sameProf = !incoming.profesor.isEmpty() && !b.profesor.isEmpty()
                           && b.profesor == incoming.profesor;
        if (!sameAula && !sameProf) continue;

        QString reason = "aula";
        if (sameAula && sameProf)      reason = "aula y profesor";
        else if (sameProf)             reason = "profesor";

        Conflict c;
        c.other = r; c.day = day; c.slot = slot; c.block = b; c.reason = reason;
        out.append(c);
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Planificación y aplicación de un movimiento
// ─────────────────────────────────────────────────────────────────────────────

struct MovePlan {
    bool valid = false;         ///< Hay una materia para mover
    bool hasConflict = false;
    int dayFrom = 0, slotFrom = 0;
    int dayTo = 0, slotTo = 0;
    Block moved;                ///< Bloque arrastrado (origen)
    Block displaced;            ///< Bloque que ocupaba el destino (puede estar vacío)
    QVector<Conflict> conflicts;
};

inline MovePlan planMove(const QJsonObject& root, const Ref& self,
                         int dayFrom, int slotFrom, int dayTo, int slotTo)
{
    MovePlan plan;
    plan.dayFrom = dayFrom; plan.slotFrom = slotFrom;
    plan.dayTo = dayTo;     plan.slotTo = slotTo;
    if (dayFrom == dayTo && slotFrom == slotTo) return plan;

    const QJsonObject s = scheduleOf(root, self);
    plan.moved = getBlock(s, dayFrom, slotFrom);
    if (plan.moved.isEmpty()) return plan;
    plan.valid = true;
    plan.displaced = getBlock(s, dayTo, slotTo);

    plan.conflicts = findConflicts(root, self, dayTo, slotTo, plan.moved);
    if (!plan.displaced.isEmpty()) {
        plan.conflicts += findConflicts(root, self, dayFrom, slotFrom, plan.displaced);
    }
    plan.hasConflict = !plan.conflicts.isEmpty();
    return plan;
}

/**
 * Aplica el movimiento dentro del horario `self`. Si `swapOthers` es true,
 * también intercambia las dos franjas en cada horario en conflicto, de modo que
 * la clase que chocaba pasa a la franja de origen (actualización global).
 */
inline void applyMove(QJsonObject& root, const Ref& self, const MovePlan& plan, bool swapOthers)
{
    if (!plan.valid) return;

    QJsonObject s = scheduleOf(root, self);
    setBlock(s, plan.dayTo, plan.slotTo, plan.moved);
    setBlock(s, plan.dayFrom, plan.slotFrom, plan.displaced);
    setSchedule(root, self, s);

    if (!swapOthers) return;

    QVector<Ref> done;
    for (const Conflict& c : plan.conflicts) {
        if (sameRef(c.other, self)) continue;
        bool already = false;
        for (const Ref& d : done) { if (sameRef(d, c.other)) { already = true; break; } }
        if (already) continue;
        done.append(c.other);

        QJsonObject t = scheduleOf(root, c.other);
        const Block atTo   = getBlock(t, plan.dayTo, plan.slotTo);
        const Block atFrom = getBlock(t, plan.dayFrom, plan.slotFrom);
        setBlock(t, plan.dayTo, plan.slotTo, atFrom);
        setBlock(t, plan.dayFrom, plan.slotFrom, atTo);
        setSchedule(root, c.other, t);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Estructura de días / franjas / receso
// ─────────────────────────────────────────────────────────────────────────────

struct RowDef {
    bool recess;
    int slot;        ///< -1 si la fila es el receso
    QString label;   ///< Etiqueta de hora (o texto del receso)
    QString turno;   ///< "Mañana" / "Tarde" / subtítulo del receso
};

inline QStringList dayNames()
{
    return { "Lunes", "Martes", "Miércoles", "Jueves", "Viernes" };
}

/** Filas de la rejilla: 12 franjas (0..11) y una fila de receso de 30 min. */
inline QVector<RowDef> rowDefs()
{
    return {
        { false, 0,  "07:00 - 07:45", "Mañana" },
        { false, 1,  "07:45 - 08:30", "Mañana" },
        { false, 2,  "08:30 - 09:15", "Mañana" },
        { true, -1,  "RECESO",        "09:15 - 09:45 (30 min)" },
        { false, 3,  "09:45 - 10:30", "Mañana" },
        { false, 4,  "10:30 - 11:15", "Mañana" },
        { false, 5,  "11:15 - 12:00", "Mañana" },
        { false, 6,  "12:30 - 13:15", "Tarde"  },
        { false, 7,  "13:15 - 14:00", "Tarde"  },
        { false, 8,  "14:00 - 14:45", "Tarde"  },
        { false, 9,  "14:45 - 15:30", "Tarde"  },
        { false, 10, "15:30 - 16:15", "Tarde"  },
        { false, 11, "16:15 - 17:00", "Tarde"  },
    };
}

} // namespace ScheduleModel
