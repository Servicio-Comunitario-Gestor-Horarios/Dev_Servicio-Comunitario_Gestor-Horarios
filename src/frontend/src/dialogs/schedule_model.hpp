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
 * La rejilla (franjas, duración del bloque y recesos) la describe el propio
 * documento en `_cuadricula`, que la generación copia de la configuración que se
 * envió al solver: días, franjas de cada turno y recesos (posición y duración).
 * Si el documento no la trae (horarios antiguos), se usa la plantilla histórica
 * de 12 franjas de 45 min con un receso de 30 min.
 *
 * Una clase puede ocupar varias franjas seguidas (una materia de dos horas
 * académicas): se pintan como un solo bloque y se mueven enteras.
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

/** Identidad de un bloque: dos franjas seguidas son la misma clase si coincide. */
inline QString firmaBloque(const Block& b)
{
    return b.materia + QChar(0x1F) + b.profesor + QChar(0x1F) + b.aula
         + QChar(0x1F) + b.curso;
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

/** Traslado elemental: la clase que está en (dayFrom, slotFrom) pasa a (dayTo, slotTo). */
struct SlotMove {
    int dayFrom = 0, slotFrom = 0;
    int dayTo = 0, slotTo = 0;
};

struct MovePlan {
    bool valid = false;         ///< Hay una clase para mover
    bool hasConflict = false;
    bool rotation = false;      ///< El destino solapa el propio tramo: se desplaza
    int dayFrom = 0, slotFrom = 0;
    int dayTo = 0, slotTo = 0;
    int franjas = 1;            ///< Franjas que ocupa la clase arrastrada (≥ 1)
    Block moved;                ///< Clase arrastrada (primera franja del tramo)
    Block displaced;            ///< Clase que ocupaba el destino (primera franja)
    QVector<SlotMove> moves;    ///< Traslados elementales (biyección de franjas)
    QVector<Conflict> conflicts;
};

/**
 * Traslados elementales del tramo que empieza en (dayFrom, slotFrom) y ocupa
 * `largo` franjas hacia (dayTo, slotTo).
 *
 * Si los dos tramos no se solapan (o son de días distintos) es el intercambio
 * completo de ambos, como siempre. Si el destino solapa el propio tramo —se
 * arrastra la clase una o dos franjas—, el tramo se desplaza y las clases que
 * ocupaban las franjas ganadas pasan a las que el tramo deja libres: ninguna
 * clase se pierde y el tramo conserva su tamaño.
 *
 * La lista es una biyección sobre las franjas afectadas: se leen todas las
 * franjas de origen antes de escribir (véase `applyMove`).
 */
inline QVector<SlotMove> tramoMovimientos(int dayFrom, int slotFrom, int largo,
                                          int dayTo, int slotTo)
{
    QVector<SlotMove> out;
    for (int i = 0; i < largo; ++i) {
        SlotMove mv;
        mv.dayFrom = dayFrom; mv.slotFrom = slotFrom + i;
        mv.dayTo = dayTo;     mv.slotTo = slotTo + i;
        out.append(mv);
    }

    if (dayFrom != dayTo) {
        // Otro día: lo que ocupaba el destino vuelve al origen entero.
        for (int i = 0; i < largo; ++i) {
            SlotMove mv;
            mv.dayFrom = dayTo;   mv.slotFrom = slotTo + i;
            mv.dayTo = dayFrom;   mv.slotTo = slotFrom + i;
            out.append(mv);
        }
        return out;
    }

    // Mismo día: las franjas del destino ajenas al tramo pasan a las franjas que
    // el tramo libera (el orden conserva la posición relativa de cada clase).
    QVector<int> ganadas;   // franjas del destino que no son del propio tramo
    QVector<int> libres;    // franjas del origen fuera del destino
    for (int i = 0; i < largo; ++i) {
        const int ganada = slotTo + i;
        if (ganada < slotFrom || ganada >= slotFrom + largo) ganadas.append(ganada);
    }
    for (int i = 0; i < largo; ++i) {
        const int libre = slotFrom + i;
        if (libre < slotTo || libre >= slotTo + largo) libres.append(libre);
    }
    for (int i = 0; i < ganadas.size() && i < libres.size(); ++i) {
        SlotMove mv;
        mv.dayFrom = dayFrom; mv.slotFrom = ganadas.at(i);
        mv.dayTo = dayFrom;   mv.slotTo = libres.at(i);
        out.append(mv);
    }
    return out;
}

/**
 * Planifica mover la clase que empieza en (dayFrom, slotFrom) y ocupa `largo`
 * franjas seguidas: una materia de dos horas académicas viaja entera, no por
 * mitades. Con `largo = 1` es el movimiento simple de una franja.
 */
inline MovePlan planMoveRun(const QJsonObject& root, const Ref& self,
                            int dayFrom, int slotFrom, int largo,
                            int dayTo, int slotTo)
{
    MovePlan plan;
    plan.dayFrom = dayFrom; plan.slotFrom = slotFrom;
    plan.dayTo = dayTo;     plan.slotTo = slotTo;
    plan.franjas = largo;
    if (largo < 1 || (dayFrom == dayTo && slotFrom == slotTo)) return plan;

    const QJsonObject s = scheduleOf(root, self);
    plan.moved = getBlock(s, dayFrom, slotFrom);
    if (plan.moved.isEmpty()) return plan;

    // El tramo debe ser homogéneo: sus `largo` franjas son la misma clase.
    const QString firma = firmaBloque(plan.moved);
    for (int i = 1; i < largo; ++i) {
        const Block b = getBlock(s, dayFrom, slotFrom + i);
        if (b.isEmpty() || firmaBloque(b) != firma) return MovePlan{};
    }

    const int salto = dayFrom == dayTo ? slotTo - slotFrom : largo;
    plan.rotation = dayFrom == dayTo && salto != 0
                 && (salto < 0 ? -salto : salto) < largo;
    plan.displaced = getBlock(s, dayTo, slotTo);
    plan.moves = tramoMovimientos(dayFrom, slotFrom, largo, dayTo, slotTo);
    plan.valid = !plan.moves.isEmpty();
    if (!plan.valid) return plan;

    // Un choque solo cuenta si el traslado cambia lo que hay en la franja: en un
    // desplazamiento, las franjas que la clase ya ocupaba no cambian.
    for (const SlotMove& mv : plan.moves) {
        const Block entra = getBlock(s, mv.dayFrom, mv.slotFrom);
        if (entra.isEmpty()) continue;
        if (firmaBloque(entra) == firmaBloque(getBlock(s, mv.dayTo, mv.slotTo)))
            continue;
        plan.conflicts += findConflicts(root, self, mv.dayTo, mv.slotTo, entra);
    }
    plan.hasConflict = !plan.conflicts.isEmpty();
    return plan;
}

/** Movimiento de una sola franja (compatibilidad con la vista anterior). */
inline MovePlan planMove(const QJsonObject& root, const Ref& self,
                         int dayFrom, int slotFrom, int dayTo, int slotTo)
{
    return planMoveRun(root, self, dayFrom, slotFrom, 1, dayTo, slotTo);
}

/**
 * Aplica el movimiento. Los traslados se leen todos del estado original antes de
 * escribir, de modo que un desplazamiento (destino sobre el propio tramo) no
 * pise clases a medio mover. Si `swapOthers` es true, el mismo traslado se
 * aplica a cada horario en conflicto, para que las clases que chocaban pasen a
 * las franjas que la clase arrastrada deja libres.
 */
inline void applyMove(QJsonObject& root, const Ref& self, const MovePlan& plan, bool swapOthers)
{
    if (!plan.valid || plan.moves.isEmpty()) return;

    auto aplicar = [&](const Ref& ref) {
        const QJsonObject antes = scheduleOf(root, ref);
        QJsonObject despues = antes;
        for (const SlotMove& mv : plan.moves)
            setBlock(despues, mv.dayTo, mv.slotTo, getBlock(antes, mv.dayFrom, mv.slotFrom));
        setSchedule(root, ref, despues);
    };

    aplicar(self);
    if (!swapOthers) return;

    QVector<Ref> done;
    for (const Conflict& c : plan.conflicts) {
        if (sameRef(c.other, self)) continue;
        bool already = false;
        for (const Ref& d : done) if (sameRef(d, c.other)) { already = true; break; }
        if (already) continue;
        done.append(c.other);
        aplicar(c.other);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Estructura de días / franjas / receso
// ─────────────────────────────────────────────────────────────────────────────

struct RowDef {
    bool recess = false;
    int slot = -1;          ///< -1 si la fila es el receso
    QString label;          ///< Etiqueta de hora ("07:00 - 07:40") o "RECESO"
    QString turno;          ///< "Mañana" / "Tarde"
    QString detalle;        ///< Detalle del receso ("08:20 - 08:30 · 10 min")
    QString horaInicio;     ///< "07:00" (vacío si el turno no declara inicio)
    QString horaFin;        ///< "07:40"
    int minutos = 0;        ///< Duración de la franja académica (min)
    int indiceTurno = -1;   ///< Posición dentro del turno (-1 en los recesos)
};

inline QStringList dayNames()
{
    return { "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado", "Domingo" };
}

/**
 * Plantilla histórica: 12 franjas de 45 min y un receso de 30 min.
 *
 * Solo se usa con documentos que no declaran su propia rejilla (`_cuadricula`),
 * como el archivo de ejemplo integrado o los horarios guardados por versiones
 * anteriores.
 */
inline QVector<RowDef> rowDefs()
{
    return {
        { false, 0,  "07:00 - 07:45", "Mañana", {}, "07:00", "07:45", 45, 0 },
        { false, 1,  "07:45 - 08:30", "Mañana", {}, "07:45", "08:30", 45, 1 },
        { false, 2,  "08:30 - 09:15", "Mañana", {}, "08:30", "09:15", 45, 2 },
        { true,  -1, "RECESO",        "Mañana", "09:15 - 09:45 (30 min)", {}, {}, 30, -1 },
        { false, 3,  "09:45 - 10:30", "Mañana", {}, "09:45", "10:30", 45, 3 },
        { false, 4,  "10:30 - 11:15", "Mañana", {}, "10:30", "11:15", 45, 4 },
        { false, 5,  "11:15 - 12:00", "Mañana", {}, "11:15", "12:00", 45, 5 },
        { false, 6,  "12:30 - 13:15", "Tarde",  {}, "12:30", "13:15", 45, 0 },
        { false, 7,  "13:15 - 14:00", "Tarde",  {}, "13:15", "14:00", 45, 1 },
        { false, 8,  "14:00 - 14:45", "Tarde",  {}, "14:00", "14:45", 45, 2 },
        { false, 9,  "14:45 - 15:30", "Tarde",  {}, "14:45", "15:30", 45, 3 },
        { false, 10, "15:30 - 16:15", "Tarde",  {}, "15:30", "16:15", 45, 4 },
        { false, 11, "16:15 - 17:00", "Tarde",  {}, "16:15", "17:00", 45, 5 },
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Rejilla declarada por el documento ("_cuadricula")
// ─────────────────────────────────────────────────────────────────────────────

struct RecesoGrid {
    int despuesDeSlot = 0;   ///< Se inserta tras esa cantidad de franjas del turno.
    int duracion = 0;        ///< Minutos.
};

struct TurnoGrid {
    QString nombre;
    QVector<int> franjas;    ///< Franjas por día del turno, en orden.
    QString inicio;          ///< "HH:mm" o vacío si el dominio no lo declara.
    QVector<RecesoGrid> recesos;
};

struct GridSpec {
    bool valida = false;     ///< ¿El documento describe su rejilla?
    int duracionMinutos = 0;
    int numDias = 5;
    int numSlotsDia = 0;
    QVector<TurnoGrid> turnos;
};

/** Lee la rejilla que la generación dejó en el documento (si la hay). */
inline GridSpec parseGrid(const QJsonObject& root)
{
    GridSpec g;
    const QJsonObject c = root.value("_cuadricula").toObject();
    if (c.isEmpty()) return g;

    g.duracionMinutos = c.value("duracion_minutos").toInt(0);
    g.numDias = c.value("num_dias").toInt(5);
    g.numSlotsDia = c.value("num_slots_dia").toInt(0);

    const QJsonArray turnos = c.value("turnos").toArray();
    for (const QJsonValue& v : turnos) {
        const QJsonObject o = v.toObject();
        TurnoGrid t;
        t.nombre = o.value("nombre").toString();
        const QJsonArray slotsJson = o.value("slot").toArray();
        for (const QJsonValue& s : slotsJson)
            t.franjas.append(s.toInt());
        t.inicio = o.value("inicio").toString();
        if (t.nombre.isEmpty() || t.franjas.isEmpty()) continue;
        g.turnos.append(t);
    }

    const QJsonArray recesos = c.value("recesos").toArray();
    for (const QJsonValue& v : recesos) {
        const QJsonObject o = v.toObject();
        const QString turno = o.value("turno").toString();
        RecesoGrid r;
        r.despuesDeSlot = o.value("despues_de_slot").toInt(0);
        r.duracion = o.value("duracion").toInt(0);
        for (TurnoGrid& t : g.turnos)
            if (t.nombre == turno) t.recesos.append(r);
    }

    g.valida = g.duracionMinutos > 0 && !g.turnos.isEmpty();
    return g;
}

/** Turno declarado por el curso/sección del horario `ref`, si lo hay. */
inline QString turnoDe(const QJsonObject& root, const Ref& ref)
{
    if (ref.kind != OwnerKind::Section) return QString();
    const QJsonArray ys = root.value("years").toArray();
    if (ref.yearIndex < 0 || ref.yearIndex >= ys.size()) return QString();
    const QJsonArray ss = ys.at(ref.yearIndex).toObject().value("sections").toArray();
    if (ref.sectionIndex < 0 || ref.sectionIndex >= ss.size()) return QString();
    return ss.at(ref.sectionIndex).toObject().value("turno").toString();
}

/**
 * Turnos que se dibujan para `ref`: para un curso, el suyo (sus clases se
 * planifican dentro de su turno); para un docente, todos, porque puede dar
 * clase en cualquiera de ellos.
 */
inline QVector<TurnoGrid> turnosVisibles(const GridSpec& g, const QJsonObject& root,
                                         const Ref& ref)
{
    const QString nombre = turnoDe(root, ref);
    if (ref.kind == OwnerKind::Section && !nombre.isEmpty()) {
        for (const TurnoGrid& t : g.turnos)
            if (t.nombre == nombre) return { t };
    }
    return g.turnos;
}

/** Hora "HH:mm" resultante de sumar `minutos` a `inicio`; vacío si no hay inicio. */
inline QString horaDesde(const QString& inicio, int minutos)
{
    if (inicio.isEmpty()) return QString();
    const QStringList partes = inicio.split(QLatin1Char(':'));
    if (partes.size() < 2) return QString();
    int h = partes.at(0).toInt();
    int m = partes.at(1).toInt() + minutos;
    h += m / 60;
    m %= 60;
    return QString("%1:%2").arg(h % 24, 2, 10, QLatin1Char('0'))
                           .arg(m, 2, 10, QLatin1Char('0'));
}

/**
 * Filas de la rejilla del horario `ref`: franjas y recesos de sus turnos, con
 * las horas calculadas desde el inicio del turno, la duración del bloque y los
 * recesos que van por delante (el receso desplaza las franjas siguientes).
 */
inline QVector<RowDef> rowsFor(const QJsonObject& root, const Ref& ref)
{
    const GridSpec g = parseGrid(root);
    if (!g.valida) return rowDefs();

    QVector<RowDef> rows;
    for (const TurnoGrid& t : turnosVisibles(g, root, ref)) {
        int reloj = 0;   // minutos desde el inicio del turno (recesos incluidos)
        for (int k = 0; k <= t.franjas.size(); ++k) {
            for (const RecesoGrid& r : t.recesos) {
                if (r.despuesDeSlot != k) continue;
                RowDef fila;
                fila.recess = true;
                fila.turno = t.nombre;
                fila.minutos = r.duracion;
                fila.horaInicio = horaDesde(t.inicio, reloj);
                fila.horaFin = horaDesde(t.inicio, reloj + r.duracion);
                fila.label = QStringLiteral("RECESO");
                fila.detalle = fila.horaInicio.isEmpty()
                                   ? QString("%1 min").arg(r.duracion)
                                   : QString("%1 - %2  ·  %3 min")
                                         .arg(fila.horaInicio, fila.horaFin)
                                         .arg(r.duracion);
                rows.append(fila);
                reloj += r.duracion;
            }
            if (k == t.franjas.size()) break;

            RowDef fila;
            fila.slot = t.franjas.at(k);
            fila.turno = t.nombre;
            fila.indiceTurno = k;
            fila.minutos = g.duracionMinutos;
            fila.horaInicio = horaDesde(t.inicio, reloj);
            fila.horaFin = horaDesde(t.inicio, reloj + g.duracionMinutos);
            fila.label = fila.horaInicio.isEmpty()
                             ? QString("Franja %1").arg(k + 1)
                             : QString("%1 - %2").arg(fila.horaInicio, fila.horaFin);
            rows.append(fila);
            reloj += g.duracionMinutos;
        }
    }
    return rows.isEmpty() ? rowDefs() : rows;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tramos: materias que ocupan más de una franja académica
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Franjas que abarca la clase que empieza en la fila `r0` del día `day`: filas
 * seguidas del mismo turno (sin receso en medio) con la misma clase. La fila de
 * un receso corta el tramo aunque los slots sean consecutivos.
 */
inline int largoDeTramo(const QJsonObject& schedule, const QVector<RowDef>& rows,
                        int r0, int day)
{
    if (r0 < 0 || r0 >= rows.size() || rows[r0].recess) return 1;
    const Block b = getBlock(schedule, day, rows[r0].slot);
    if (b.isEmpty()) return 1;

    const QString firma = firmaBloque(b);
    int largo = 1;
    while (r0 + largo < rows.size()) {
        const RowDef& fila = rows[r0 + largo];
        // Franjas seguidas del mismo turno, sin receso en medio y consecutivas
        // también por número (el movimiento del tramo opera con `slot + k`).
        if (fila.recess || fila.turno != rows[r0].turno
            || fila.indiceTurno != rows[r0].indiceTurno + largo
            || fila.slot != rows[r0].slot + largo)
            break;
        const Block siguiente = getBlock(schedule, day, fila.slot);
        if (siguiente.isEmpty() || firmaBloque(siguiente) != firma) break;
        ++largo;
    }
    return largo;
}

/** ¿Caben `largo` franjas seguidas a partir del slot `slot`? */
inline bool franjasSeguidas(const QVector<RowDef>& rows, int slot, int largo)
{
    int r0 = -1;
    for (int i = 0; i < rows.size(); ++i)
        if (!rows[i].recess && rows[i].slot == slot) { r0 = i; break; }
    if (r0 < 0) return false;

    for (int i = 1; i < largo; ++i) {
        const int r = r0 + i;
        if (r >= rows.size() || rows[r].recess || rows[r].turno != rows[r0].turno
            || rows[r].indiceTurno != rows[r0].indiceTurno + i
            || rows[r].slot != slot + i)
            return false;
    }
    return true;
}

/** Largo del tramo que contiene la franja (day, slot). */
inline int largoDeTramoEn(const QJsonObject& schedule, const QVector<RowDef>& rows,
                          int day, int slot)
{
    for (int i = 0; i < rows.size(); ++i)
        if (!rows[i].recess && rows[i].slot == slot)
            return largoDeTramo(schedule, rows, i, day);
    return 1;
}

/** Nº de clases que se pintan (las franjas seguidas de una misma clase cuentan una). */
inline int bloquesVisuales(const QJsonObject& schedule, const QVector<RowDef>& rows,
                           int numDias)
{
    int total = 0;
    for (int day = 1; day <= numDias; ++day) {
        int r = 0;
        while (r < rows.size()) {
            if (rows[r].recess) { ++r; continue; }
            const int largo = largoDeTramo(schedule, rows, r, day);
            if (!getBlock(schedule, day, rows[r].slot).isEmpty()) ++total;
            r += largo;
        }
    }
    return total;
}

/** Nº de franjas académicas ocupadas (una por clase/hora asignada). */
inline int horasAsignadas(const QJsonObject& schedule)
{
    int total = 0;
    for (auto it = schedule.constBegin(); it != schedule.constEnd(); ++it) {
        const QJsonObject dia = it.value().toObject();
        for (auto jt = dia.constBegin(); jt != dia.constEnd(); ++jt)
            if (!parseBlock(jt.value()).isEmpty()) ++total;
    }
    return total;
}

} // namespace ScheduleModel
