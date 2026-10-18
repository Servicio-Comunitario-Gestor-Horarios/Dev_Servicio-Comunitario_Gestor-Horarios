#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include "dialogs/schedule_model.hpp"

/// Tests de la rejilla dirigida por datos.
///
/// La visualización no supone turnos ni franjas: los lee del documento
/// (`_cuadricula`), que la generación copia de la configuración enviada al
/// solver. Aquí se comprueban las filas según el turno del curso, el receso con
/// su duración real, el cálculo de horas y la unión de una clase que ocupa
/// varias franjas seguidas.
class TestCuadriculaHorario : public QObject
{
    Q_OBJECT

    /// Documento con la rejilla real: 16 franjas de 40 min por día, mañana
    /// (07:00, slots 0..7) y tarde (12:40, slots 8..15), receso de 10 min tras
    /// la segunda franja de cada turno. Sin secciones ni docentes todavía.
    static QJsonObject documentoBase()
    {
        QJsonArray turnos;
        for (int t = 0; t < 2; ++t) {
            QJsonObject turno;
            turno.insert(QStringLiteral("nombre"),
                         t == 0 ? QStringLiteral("Mañana") : QStringLiteral("Tarde"));
            turno.insert(QStringLiteral("inicio"),
                         t == 0 ? QStringLiteral("07:00") : QStringLiteral("12:40"));
            QJsonArray franjas;
            for (int s = 0; s < 8; ++s)
                franjas.append(t * 8 + s);
            turno.insert(QStringLiteral("slot"), franjas);
            turnos.append(turno);
        }

        QJsonArray recesos;
        for (int t = 0; t < 2; ++t) {
            QJsonObject receso;
            receso.insert(QStringLiteral("turno"),
                          t == 0 ? QStringLiteral("Mañana") : QStringLiteral("Tarde"));
            receso.insert(QStringLiteral("despues_de_slot"), 2);
            receso.insert(QStringLiteral("duracion"), 10);
            receso.insert(QStringLiteral("inicio"),
                          t == 0 ? QStringLiteral("08:20") : QStringLiteral("14:00"));
            receso.insert(QStringLiteral("fin"),
                          t == 0 ? QStringLiteral("08:30") : QStringLiteral("14:10"));
            recesos.append(receso);
        }

        QJsonObject rejilla;
        rejilla.insert(QStringLiteral("duracion_minutos"), 40);
        rejilla.insert(QStringLiteral("num_dias"), 5);
        rejilla.insert(QStringLiteral("num_slots_dia"), 16);
        rejilla.insert(QStringLiteral("turnos"), turnos);
        rejilla.insert(QStringLiteral("recesos"), recesos);

        QJsonObject raiz;
        raiz.insert(QStringLiteral("_cuadricula"), rejilla);
        return raiz;
    }

    /// Añade al documento un año con una sección del `turno` indicado y su horario.
    static QJsonObject conSeccion(const QJsonObject& doc, const QString& turno,
                                  const QJsonObject& schedule)
    {
        QJsonObject seccion;
        seccion.insert(QStringLiteral("name"), QStringLiteral("1ro A"));
        seccion.insert(QStringLiteral("turno"), turno);
        seccion.insert(QStringLiteral("schedule"), schedule);

        QJsonArray secciones;
        secciones.append(seccion);

        QJsonObject anio;
        anio.insert(QStringLiteral("year"), 1);
        anio.insert(QStringLiteral("name"), QStringLiteral("1° Año"));
        anio.insert(QStringLiteral("sections"), secciones);

        QJsonArray years;
        years.append(anio);

        QJsonObject copia = doc;
        copia.insert(QStringLiteral("years"), years);
        return copia;
    }

    /// Añade al documento un docente con ese horario (para ver los turnos unidos).
    static QJsonObject conDocente(const QJsonObject& doc, const QJsonObject& schedule)
    {
        QJsonObject docente;
        docente.insert(QStringLiteral("name"), QStringLiteral("Ana"));
        docente.insert(QStringLiteral("schedule"), schedule);

        QJsonArray teachers;
        teachers.append(docente);

        QJsonObject copia = doc;
        copia.insert(QStringLiteral("teachers"), teachers);
        return copia;
    }

    static ScheduleModel::Ref refSeccion()
    {
        ScheduleModel::Ref ref;
        ref.kind = ScheduleModel::OwnerKind::Section;
        ref.yearIndex = 0;
        ref.sectionIndex = 0;
        return ref;
    }

    static ScheduleModel::Ref refDocente()
    {
        ScheduleModel::Ref ref;
        ref.kind = ScheduleModel::OwnerKind::Teacher;
        ref.teacherIndex = 0;
        return ref;
    }

    static QJsonObject bloque(const char* materia, const char* aula)
    {
        QJsonObject b;
        b.insert(QStringLiteral("materia"), QString::fromUtf8(materia));
        b.insert(QStringLiteral("profesor"), QStringLiteral("Ana"));
        b.insert(QStringLiteral("aula"), QString::fromUtf8(aula));
        return b;
    }

    /// Horario (día -> slot -> bloque) con `bloque` en cada franja indicada,
    /// donde cada franja es el par (día, slot).
    static QJsonObject horario(const QVector<QPair<int, int>>& franjas, const QJsonObject& bloque)
    {
        QJsonObject schedule;
        for (const QPair<int, int>& f : franjas) {
            const QString dia = QString::number(f.first);
            QJsonObject d = schedule.value(dia).toObject();
            d.insert(QString::number(f.second), bloque);
            schedule.insert(dia, d);
        }
        return schedule;
    }

    static int filaDelSlot(const QVector<ScheduleModel::RowDef>& rows, int slot)
    {
        for (int i = 0; i < rows.size(); ++i)
            if (!rows[i].recess && rows[i].slot == slot) return i;
        return -1;
    }

private slots:
    // ── Filas según el turno ────────────────────────────────────────────────
    void lasFilasDelCursoSiguenSuTurno()
    {
        const QJsonObject doc =
            conSeccion(documentoBase(), QStringLiteral("Tarde"), QJsonObject());
        const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowsFor(doc, refSeccion());

        // Solo las franjas de la tarde (8..15) y su receso: las de la mañana no
        // se dibujan para un curso de la tarde.
        QVector<int> franjas;
        for (const ScheduleModel::RowDef& r : rows)
            if (!r.recess) franjas.append(r.slot);
        QCOMPARE(franjas, (QVector<int>{8, 9, 10, 11, 12, 13, 14, 15}));

        // El receso va tras la segunda franja del turno, con su duración real.
        QCOMPARE(rows.size(), 9);
        const ScheduleModel::RowDef& receso = rows.at(2);
        QVERIFY(receso.recess);
        QCOMPARE(receso.minutos, 10);
        QCOMPARE(receso.label, QStringLiteral("RECESO"));
        QVERIFY(receso.detalle.contains(QStringLiteral("14:00")));
        QVERIFY(receso.detalle.contains(QStringLiteral("10 min")));

        // Horas: inicio del turno + bloque de 40 min, desplazadas por el receso.
        QCOMPARE(rows.at(0).label, QStringLiteral("12:40 - 13:20"));
        QCOMPARE(rows.at(1).label, QStringLiteral("13:20 - 14:00"));
        QCOMPARE(rows.at(3).label, QStringLiteral("14:10 - 14:50"));
        QCOMPARE(rows.at(8).label, QStringLiteral("17:30 - 18:10"));
    }

    void lasFilasDelDocenteIncluyenTodosLosTurnos()
    {
        const QJsonObject doc =
            conDocente(documentoBase(), QJsonObject());
        const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowsFor(doc, refDocente());

        // Un docente puede dar clase en cualquier turno: se dibujan los dos.
        QCOMPARE(rows.size(), 18);   // 16 franjas + 2 recesos
        QCOMPARE(rows.at(0).label, QStringLiteral("07:00 - 07:40"));
        QCOMPARE(rows.at(2).label, QStringLiteral("RECESO"));
        QCOMPARE(rows.at(3).label, QStringLiteral("08:30 - 09:10"));
        QCOMPARE(rows.at(17).label, QStringLiteral("17:30 - 18:10"));
    }

    void documentoSinRejillaUsaLaPlantillaHistorica()
    {
        const QJsonObject doc;   // sin `_cuadricula`
        const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowsFor(doc, refDocente());

        QCOMPARE(rows.size(), ScheduleModel::rowDefs().size());
        QCOMPARE(rows.at(3).recess, true);
        QCOMPARE(rows.at(3).detalle, QStringLiteral("09:15 - 09:45 (30 min)"));
        QCOMPARE(rows.at(0).label, QStringLiteral("07:00 - 07:45"));
    }

    // ── Una clase que ocupa varias franjas ──────────────────────────────────
    void uneLasFranjasSeguidasYElRecesoCortaLosTramos()
    {
        const QJsonObject doc =
            conSeccion(documentoBase(), QStringLiteral("Mañana"), QJsonObject());
        const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowsFor(doc, refSeccion());
        const int fila0 = filaDelSlot(rows, 0);

        const QJsonObject matematica = bloque("Matemática", "A1");
        // Franjas 0 y 1: misma materia, profesor y aula → un solo bloque.
        const QJsonObject seguidas = horario({{1, 0}, {1, 1}}, matematica);
        QCOMPARE(ScheduleModel::largoDeTramo(seguidas, rows, fila0, 1), 2);
        QCOMPARE(ScheduleModel::horasAsignadas(seguidas), 2);
        QCOMPARE(ScheduleModel::bloquesVisuales(seguidas, rows, 5), 1);

        // Franjas 1 y 2: el receso va en medio, así que son dos bloques.
        const QJsonObject conReceso = horario({{1, 1}, {1, 2}}, matematica);
        QCOMPARE(ScheduleModel::largoDeTramo(conReceso, rows, filaDelSlot(rows, 1), 1), 1);

        // Distinto aula (o profesor): tampoco se unen.
        QJsonObject dia;
        dia.insert(QStringLiteral("0"), matematica);
        dia.insert(QStringLiteral("1"), bloque("Matemática", "A2"));
        QJsonObject otraAula;
        otraAula.insert(QStringLiteral("1"), dia);
        QCOMPARE(ScheduleModel::largoDeTramo(otraAula, rows, fila0, 1), 1);

        // Otra materia tampoco.
        QJsonObject diaMaterias;
        diaMaterias.insert(QStringLiteral("0"), matematica);
        diaMaterias.insert(QStringLiteral("1"), bloque("Física", "A1"));
        QJsonObject dosMaterias;
        dosMaterias.insert(QStringLiteral("1"), diaMaterias);
        QCOMPARE(ScheduleModel::largoDeTramo(dosMaterias, rows, fila0, 1), 1);
    }

    void mueveElBloqueEntero()
    {
        const QJsonObject bloqueMate = bloque("Matemática", "A1");
        QJsonObject doc = conSeccion(documentoBase(), QStringLiteral("Mañana"),
                                     horario({{1, 0}, {1, 1}}, bloqueMate));
        const ScheduleModel::Ref ref = refSeccion();
        const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowsFor(doc, ref);
        const QJsonObject schedule = ScheduleModel::scheduleOf(doc, ref);

        QCOMPARE(ScheduleModel::largoDeTramoEn(schedule, rows, 1, 0), 2);
        // El destino (franjas 4 y 5) cabe; empezar en la 1 no (receso en medio).
        QVERIFY(ScheduleModel::franjasSeguidas(rows, 4, 2));
        QVERIFY(!ScheduleModel::franjasSeguidas(rows, 1, 2));

        const ScheduleModel::MovePlan plan =
            ScheduleModel::planMoveRun(doc, ref, 1, 0, 2, 2, 4);
        QVERIFY(plan.valid);
        QVERIFY(!plan.rotation);
        QCOMPARE(plan.franjas, 2);
        // Dos traslados del tramo y dos de lo que ocupaba el destino (intercambio).
        QCOMPARE(plan.moves.size(), 4);
        ScheduleModel::applyMove(doc, ref, plan, /*swapOthers=*/false);

        const QJsonObject despues = ScheduleModel::scheduleOf(doc, ref);
        QVERIFY(ScheduleModel::getBlock(despues, 1, 0).isEmpty());
        QVERIFY(ScheduleModel::getBlock(despues, 1, 1).isEmpty());
        QCOMPARE(ScheduleModel::getBlock(despues, 2, 4).materia, QStringLiteral("Matemática"));
        QCOMPARE(ScheduleModel::getBlock(despues, 2, 5).materia, QStringLiteral("Matemática"));
        QCOMPARE(ScheduleModel::horasAsignadas(despues), 2);
    }

    /// Añade (o reemplaza) un bloque en una franja del día 1 del horario.
    static void poner(QJsonObject& schedule, int slot, const QJsonObject& b)
    {
        QJsonObject dia = schedule.value(QStringLiteral("1")).toObject();
        dia.insert(QString::number(slot), b);
        schedule.insert(QStringLiteral("1"), dia);
    }

    /// Soltar el tramo encima de sí mismo (una franja más arriba) lo desplaza sin
    /// deformarlo: antes el tramo crecía a tres franjas y la clase de en medio
    /// desaparecía.
    void desplazarElTramoSobreSuPropioEspacioNoLoDeforma()
    {
        const ScheduleModel::Ref ref = refSeccion();
        QJsonObject horarioSeccion =
            horario({{1, 3}, {1, 4}}, bloque("Matemática", "A1"));
        poner(horarioSeccion, 5, bloque("Física", "A2"));
        QJsonObject doc =
            conSeccion(documentoBase(), QStringLiteral("Mañana"), horarioSeccion);

        const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowsFor(doc, ref);
        const QJsonObject antes = ScheduleModel::scheduleOf(doc, ref);
        QCOMPARE(ScheduleModel::largoDeTramoEn(antes, rows, 1, 3), 2);
        QCOMPARE(ScheduleModel::horasAsignadas(antes), 3);

        // Destino: franjas 4 y 5 (la 4 ya es del propio tramo).
        const ScheduleModel::MovePlan plan =
            ScheduleModel::planMoveRun(doc, ref, 1, 3, 2, 1, 4);
        QVERIFY(plan.valid);
        QVERIFY(plan.rotation);
        ScheduleModel::applyMove(doc, ref, plan, /*swapOthers=*/false);

        const QJsonObject despues = ScheduleModel::scheduleOf(doc, ref);
        // La clase vecina pasa a la franja que el tramo deja libre...
        QCOMPARE(ScheduleModel::getBlock(despues, 1, 3).materia, QStringLiteral("Física"));
        // ...y el tramo queda igual de largo, sin crecer ni perder franjas.
        QCOMPARE(ScheduleModel::getBlock(despues, 1, 4).materia, QStringLiteral("Matemática"));
        QCOMPARE(ScheduleModel::getBlock(despues, 1, 5).materia, QStringLiteral("Matemática"));
        QCOMPARE(ScheduleModel::horasAsignadas(despues), 3);
        QCOMPARE(ScheduleModel::largoDeTramo(despues, rows,
                                             filaDelSlot(rows, 4), 1), 2);
    }

    /// El mismo desplazamiento hacia abajo (el destino empieza una franja después).
    void desplazarElTramoHaciaAbajo()
    {
        const ScheduleModel::Ref ref = refSeccion();
        QJsonObject sched = horario({{1, 11}, {1, 12}}, bloque("Matemática", "A1"));
        poner(sched, 13, bloque("Física", "A2"));
        QJsonObject doc =
            conSeccion(documentoBase(), QStringLiteral("Tarde"), sched);

        const ScheduleModel::MovePlan plan =
            ScheduleModel::planMoveRun(doc, ref, 1, 11, 2, 1, 12);
        QVERIFY(plan.valid);
        QVERIFY(plan.rotation);
        ScheduleModel::applyMove(doc, ref, plan, /*swapOthers=*/false);

        const QJsonObject despues = ScheduleModel::scheduleOf(doc, ref);
        QCOMPARE(ScheduleModel::getBlock(despues, 1, 11).materia, QStringLiteral("Física"));
        QCOMPARE(ScheduleModel::getBlock(despues, 1, 12).materia, QStringLiteral("Matemática"));
        QCOMPARE(ScheduleModel::getBlock(despues, 1, 13).materia, QStringLiteral("Matemática"));
        QCOMPARE(ScheduleModel::horasAsignadas(despues), 3);
    }

    /// Un desplazamiento con conflicto recoloca las clases en el otro horario
    /// sin perder ninguna.
    void elIntercambioGlobalConservaTodasLasClases()
    {
        // Docente: Matemática en 3,4 y Física en 5.
        QJsonObject docente = horario({{1, 3}, {1, 4}}, bloque("Matemática", "A1"));
        poner(docente, 5, bloque("Física", "A1"));
        // Sección: clases en la misma aula, en las franjas afectadas.
        QJsonObject seccion = horario({{1, 3}}, bloque("Historia", "A1"));
        poner(seccion, 5, bloque("Química", "A1"));

        QJsonObject doc = conDocente(
            conSeccion(documentoBase(), QStringLiteral("Mañana"), seccion), docente);
        ScheduleModel::Ref ref;   // el docente
        ref.kind = ScheduleModel::OwnerKind::Teacher;
        ref.teacherIndex = 0;

        const ScheduleModel::MovePlan plan =
            ScheduleModel::planMoveRun(doc, ref, 1, 3, 2, 1, 4);
        QVERIFY(plan.valid);
        QVERIFY(plan.hasConflict);
        ScheduleModel::applyMove(doc, ref, plan, /*swapOthers=*/true);

        // El docente desplaza el tramo y la Física pasa a la franja libre.
        const QJsonObject docD = ScheduleModel::scheduleOf(doc, ref);
        QCOMPARE(ScheduleModel::horasAsignadas(docD), 3);
        QCOMPARE(ScheduleModel::getBlock(docD, 1, 3).materia, QStringLiteral("Física"));
        QCOMPARE(ScheduleModel::getBlock(docD, 1, 4).materia, QStringLiteral("Matemática"));
        QCOMPARE(ScheduleModel::getBlock(docD, 1, 5).materia, QStringLiteral("Matemática"));

        // Y la otra vista tampoco pierde ninguna de sus clases.
        const QJsonObject sec = ScheduleModel::scheduleOf(doc, refSeccion());
        QCOMPARE(ScheduleModel::horasAsignadas(sec), 2);
    }
};

QTEST_APPLESS_MAIN(TestCuadriculaHorario)

#include "test_cuadricula_horario.moc"
