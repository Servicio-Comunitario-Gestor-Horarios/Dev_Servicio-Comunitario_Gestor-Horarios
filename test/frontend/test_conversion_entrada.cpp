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
        const QJsonObject scheduleCurso = seccion.value(QStringLiteral("schedule")).toObject();
        QCOMPARE(bloque(scheduleCurso, QStringLiteral("1"), QStringLiteral("0"))
                     .value(QStringLiteral("materia")).toString(),
                 QStringLiteral("Matemática"));
    }

    void omiteDiasYSlotsFueraDeRango()
    {
        HorarioSalida horario;
        CursoOutput curso;
        curso.dias = {
            // Día del solver 5 → "6": fuera de rango, se omite.
            DiaOutput{5, {asignacion(0, 0, 0, 0)}},
            // Slot 15: fuera de rango, se omite; slot 2 es válido.
            DiaOutput{0, {asignacion(15, 1, 1, 0), asignacion(2, 0, 0, 0)}},
        };
        horario.horarios.insert(claveCurso(1, QStringLiteral("A")), curso);

        const QJsonObject root = GeneracionSolver::aFormatoVisualizacion(horario, contextoBase());
        const QJsonArray teachers = root.value(QStringLiteral("teachers")).toArray();
        const QJsonObject scheduleAna = teachers.at(0).toObject()
                                            .value(QStringLiteral("schedule")).toObject();

        QVERIFY(!scheduleAna.contains(QStringLiteral("6")));
        QVERIFY(!scheduleAna.contains(QStringLiteral("1"))
                || !scheduleAna.value(QStringLiteral("1")).toObject().contains(QStringLiteral("15")));
        QVERIFY(!bloque(scheduleAna, QStringLiteral("1"), QStringLiteral("2")).isEmpty());
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
