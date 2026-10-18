#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "services/entrada_solver_builder.hpp"

/// Tests de la traducción `HorarioSalida` → formato de visualización (RF-1).
///
/// El conversor es puro: recibe la salida del solver (índices enteros) y un
/// contexto de nombres, y produce el JSON con `teachers`/`years` que consume la
/// sección de visualización. Aquí se comprueban la traducción de índices, las
/// claves de día/slot y el filtrado de valores fuera de rango.
class TestConversionEntrada : public QObject
{
    Q_OBJECT

private:
    GeneracionSolver::ContextoHorario contextoBase() const
    {
        GeneracionSolver::ContextoHorario ctx;
        ctx.materias   = {QStringLiteral("Matemática"), QStringLiteral("Física")};
        ctx.profesores = {QStringLiteral("Ana"), QStringLiteral("Luis")};
        ctx.aulas      = {QStringLiteral("A1")};
        ctx.cursos.insert(claveCurso(1, QStringLiteral("A")), qMakePair(1, QStringLiteral("A")));
        ctx.cursos.insert(claveCurso(2, QStringLiteral("B")), qMakePair(2, QStringLiteral("B")));
        return ctx;
    }

    /// Contexto con la rejilla real: 16 franjas por día, mañana 0..7 y tarde
    /// 8..15, con un receso de 10 min tras la segunda franja de cada turno.
    GeneracionSolver::ContextoHorario contextoConRejilla() const
    {
        GeneracionSolver::ContextoHorario ctx = contextoBase();
        ctx.cuadricula.duracionMinutos = 40;
        ctx.cuadricula.numDias = 5;
        ctx.cuadricula.numSlotsDia = 16;

        GeneracionSolver::TurnoVisual manana;
        manana.nombre = QStringLiteral("Mañana");
        manana.inicio = QStringLiteral("07:00");
        GeneracionSolver::TurnoVisual tarde;
        tarde.nombre = QStringLiteral("Tarde");
        tarde.inicio = QStringLiteral("12:40");
        for (int s = 0; s < 8; ++s) {
            manana.franjas.append(s);
            tarde.franjas.append(s + 8);
        }
        ctx.cuadricula.turnos = {manana, tarde};

        GeneracionSolver::RecesoVisual recesoManana;
        recesoManana.turno = manana.nombre;
        recesoManana.despuesDeSlot = 2;
        recesoManana.duracion = 10;
        recesoManana.inicio = QStringLiteral("08:20");
        recesoManana.fin = QStringLiteral("08:30");
        GeneracionSolver::RecesoVisual recesoTarde = recesoManana;
        recesoTarde.turno = tarde.nombre;
        recesoTarde.inicio = QStringLiteral("14:00");
        recesoTarde.fin = QStringLiteral("14:10");
        ctx.cuadricula.recesos = {recesoManana, recesoTarde};
        return ctx;
    }

    static AsignacionOutput asignacion(int slot, int materia, int profesor, int aula)
    {
        AsignacionOutput a;
        a.slot = slot;
        a.materia = materia;
        a.profesor = profesor;
        a.aula = aula;
        return a;
    }

    static QJsonObject bloque(const QJsonObject& schedule, const QString& dia, const QString& slot)
    {
        return schedule.value(dia).toObject().value(slot).toObject();
    }

private slots:
    void traduceIndicesANombresYClavesDeDia()
    {
        HorarioSalida horario;
        horario.metadata.fecha_generacion = QStringLiteral("2026-10-17 10:00:00");
        horario.metadata.total_asignaciones = 3;
        horario.metadata.cursos_generados = 2;

        CursoOutput cursoA;
        cursoA.turno = QStringLiteral("manana");
        cursoA.dias = {
            DiaOutput{0, {asignacion(0, 0, 0, 0), asignacion(1, 1, 1, 0)}},
        };
        horario.horarios.insert(claveCurso(1, QStringLiteral("A")), cursoA);

        CursoOutput cursoB;
        cursoB.turno = QStringLiteral("tarde");
        cursoB.dias = {
            DiaOutput{2, {asignacion(3, 0, 0, 0)}},
        };
        horario.horarios.insert(claveCurso(2, QStringLiteral("B")), cursoB);

        const QJsonObject root = GeneracionSolver::aFormatoVisualizacion(horario, contextoBase());

        // ── Docentes ──────────────────────────────────────────────────────
        const QJsonArray teachers = root.value(QStringLiteral("teachers")).toArray();
        QCOMPARE(int(teachers.size()), 2);
        QCOMPARE(teachers.at(0).toObject().value(QStringLiteral("name")).toString(),
                 QStringLiteral("Ana"));

        const QJsonObject scheduleAna = teachers.at(0).toObject()
                                            .value(QStringLiteral("schedule")).toObject();
        // Día del solver 0 → clave "1"; slot 0.
        const QJsonObject bloqueAna0 = bloque(scheduleAna, QStringLiteral("1"), QStringLiteral("0"));
        QCOMPARE(bloqueAna0.value(QStringLiteral("materia")).toString(),
                 QStringLiteral("Matemática"));
        QCOMPARE(bloqueAna0.value(QStringLiteral("profesor")).toString(), QStringLiteral("Ana"));
        QCOMPARE(bloqueAna0.value(QStringLiteral("aula")).toString(), QStringLiteral("A1"));
        QCOMPARE(bloqueAna0.value(QStringLiteral("curso")).toString(), QStringLiteral("1ro A"));

        // Luis en día 0/slot 1.
        const QJsonObject scheduleLuis = teachers.at(1).toObject()
                                             .value(QStringLiteral("schedule")).toObject();
        const QJsonObject bloqueLuis = bloque(scheduleLuis, QStringLiteral("1"), QStringLiteral("1"));
        QCOMPARE(bloqueLuis.value(QStringLiteral("materia")).toString(), QStringLiteral("Física"));
        QCOMPARE(bloqueLuis.value(QStringLiteral("curso")).toString(), QStringLiteral("1ro A"));

        // Día del solver 2 → clave "3". La asignación de "2do B" es de Ana
        // (profesor 0), así que aparece en su horario.
        const QJsonObject bloqueAnaB = bloque(scheduleAna, QStringLiteral("3"), QStringLiteral("3"));
        QCOMPARE(bloqueAnaB.value(QStringLiteral("materia")).toString(),
                 QStringLiteral("Matemática"));
        QCOMPARE(bloqueAnaB.value(QStringLiteral("curso")).toString(), QStringLiteral("2do B"));

        // ── Años y secciones ──────────────────────────────────────────────
        const QJsonArray years = root.value(QStringLiteral("years")).toArray();
        QCOMPARE(int(years.size()), 2);
        QCOMPARE(years.at(0).toObject().value(QStringLiteral("year")).toInt(), 1);
        QCOMPARE(years.at(0).toObject().value(QStringLiteral("name")).toString(),
                 QStringLiteral("1° Año"));

        const QJsonArray secciones = years.at(0).toObject()
                                         .value(QStringLiteral("sections")).toArray();
        QCOMPARE(int(secciones.size()), 1);
        const QJsonObject seccion = secciones.at(0).toObject();
        QCOMPARE(seccion.value(QStringLiteral("name")).toString(), QStringLiteral("1ro A"));
        // El turno del curso viaja con la sección: decide qué franjas se dibujan.
        QCOMPARE(seccion.value(QStringLiteral("turno")).toString(), QStringLiteral("manana"));
        const QJsonObject scheduleCurso = seccion.value(QStringLiteral("schedule")).toObject();
        QCOMPARE(bloque(scheduleCurso, QStringLiteral("1"), QStringLiteral("0"))
                     .value(QStringLiteral("materia")).toString(),
                 QStringLiteral("Matemática"));
    }

    void omiteDiasFueraDeRangoYConservaLaTarde()
    {
        HorarioSalida horario;
        CursoOutput curso;
        curso.dias = {
            // Día del solver 5 → "6": fuera de rango, se omite.
            DiaOutput{5, {asignacion(0, 0, 0, 0)}},
            // Franjas 2 y 15 (esta última de la tarde): ambas válidas.
            DiaOutput{0, {asignacion(15, 0, 0, 0), asignacion(2, 1, 1, 0)}},
        };
        horario.horarios.insert(claveCurso(1, QStringLiteral("A")), curso);

        const QJsonObject root =
            GeneracionSolver::aFormatoVisualizacion(horario, contextoConRejilla());
        const QJsonArray teachers = root.value(QStringLiteral("teachers")).toArray();
        const QJsonObject scheduleAna = teachers.at(0).toObject()
                                            .value(QStringLiteral("schedule")).toObject();

        QVERIFY(!scheduleAna.contains(QStringLiteral("6")));
        // La franja 15 es de la tarde y antes se descartaba en silencio.
        QVERIFY(!bloque(scheduleAna, QStringLiteral("1"), QStringLiteral("15")).isEmpty());
        // La franja 2 la imparte Luis, así que no aparece en el horario de Ana.
        QVERIFY(bloque(scheduleAna, QStringLiteral("1"), QStringLiteral("2")).isEmpty());
    }

    void declaraLaRejillaConLaQueSeGenero()
    {
        HorarioSalida horario;
        CursoOutput curso;
        curso.turno = QStringLiteral("Tarde");
        curso.dias = {DiaOutput{0, {asignacion(8, 0, 0, 0)}}};
        horario.horarios.insert(claveCurso(1, QStringLiteral("A")), curso);

        const QJsonObject root =
            GeneracionSolver::aFormatoVisualizacion(horario, contextoConRejilla());

        // La rejilla viaja en el documento: la visualización no adivina la
        // duración del bloque, las franjas de cada turno ni los recesos.
        const QJsonObject rejilla = root.value(QStringLiteral("_cuadricula")).toObject();
        QCOMPARE(rejilla.value(QStringLiteral("duracion_minutos")).toInt(), 40);
        QCOMPARE(rejilla.value(QStringLiteral("num_dias")).toInt(), 5);
        QCOMPARE(rejilla.value(QStringLiteral("num_slots_dia")).toInt(), 16);
        const QJsonArray turnos = rejilla.value(QStringLiteral("turnos")).toArray();
        QCOMPARE(int(turnos.size()), 2);
        QCOMPARE(turnos.at(1).toObject().value(QStringLiteral("nombre")).toString(),
                 QStringLiteral("Tarde"));
        QCOMPARE(turnos.at(1).toObject().value(QStringLiteral("inicio")).toString(),
                 QStringLiteral("12:40"));
        QCOMPARE(int(turnos.at(1).toObject().value(QStringLiteral("slot")).toArray().size()), 8);

        const QJsonArray recesos = rejilla.value(QStringLiteral("recesos")).toArray();
        QCOMPARE(int(recesos.size()), 2);
        QCOMPARE(recesos.at(0).toObject().value(QStringLiteral("duracion")).toInt(), 10);
        QCOMPARE(recesos.at(0).toObject().value(QStringLiteral("despues_de_slot")).toInt(), 2);
    }

    void incluyeMeta()
    {
        HorarioSalida horario;
        horario.metadata.fecha_generacion = QStringLiteral("2026-10-17 12:34:56");
        horario.metadata.total_asignaciones = 7;
        horario.metadata.cursos_generados = 3;

        const QJsonObject root = GeneracionSolver::aFormatoVisualizacion(horario, contextoBase());
        const QJsonObject meta = root.value(QStringLiteral("_meta")).toObject();
        QCOMPARE(meta.value(QStringLiteral("fecha_generacion")).toString(),
                 QStringLiteral("2026-10-17 12:34:56"));
        QCOMPARE(meta.value(QStringLiteral("total_asignaciones")).toInt(), 7);
        QCOMPARE(meta.value(QStringLiteral("cursos_generados")).toInt(), 3);
    }
};

QTEST_APPLESS_MAIN(TestConversionEntrada)

#include "test_conversion_entrada.moc"
