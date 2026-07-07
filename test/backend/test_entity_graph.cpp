#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <backend/data/aula.hpp>
#include <backend/data/franja_horaria.hpp>
#include <backend/data/materia.hpp>
#include <backend/data/profesor.hpp>
#include <backend/data/horario.hpp>
#include <backend/data/plan_estudio.hpp>

class TestEntityGraph : public QObject {
    Q_OBJECT

private slots:
    void fullGraphRoundTrip() {
        // ── FranjaHoraria ──
        FranjaHoraria f1, f2;
        f1.dia = 1;
        f1.inicio = QTime(8, 0);
        f1.fin = QTime(10, 0);
        f2.dia = 3;
        f2.inicio = QTime(14, 0);
        f2.fin = QTime(16, 0);

        // ── Aula ──
        Aula aula;
        aula.nombre = "A-101";
        aula.capacidad = 30;
        aula.locacion = "Edificio A, Piso 1";

        // ── Materia ──
        Materia materia;
        materia.nombre = "Programacion";
        materia.horas_semanales = 6;
        materia.requerimientos = {"Logica"};

        // ── Profesor ──
        Profesor profesor;
        profesor.nombre = "Luis";
        profesor.disponibilidad = {f1, f2};
        profesor.materias = {"Programacion", "Algoritmos"};

        // ── MateriaEnPlan + PlanEstudio ──
        MateriaEnPlan mep;
        mep.curso = 1;
        mep.horas = 6;
        mep.materia = materia;

        PlanEstudio plan;
        plan.nombre = "Ingenieria en Sistemas";
        plan.materias = {mep};

        // ── Asignacion + Horario ──
        Asignacion asignacion;
        asignacion.aula = aula;
        asignacion.franja_horaria = f1;
        asignacion.materia = materia;
        asignacion.profesor = profesor;

        Horario horario;
        horario.asignaciones = {asignacion};

        // ── Serializar todo ──
        QJsonObject aulaJson = aula.toJson();
        QJsonObject franjaJson = f1.toJson();
        QJsonObject materiaJson = materia.toJson();
        QJsonObject profesorJson = profesor.toJson();
        QJsonObject mepJson = mep.toJson();
        QJsonObject planJson = plan.toJson();
        QJsonObject asigJson = asignacion.toJson();
        QJsonObject horarioJson = horario.toJson();

        // ── Deserializar todo ──
        Aula aulaOut = Aula::fromJson(aulaJson);
        FranjaHoraria franjaOut = FranjaHoraria::fromJson(franjaJson);
        Materia materiaOut = Materia::fromJson(materiaJson);
        Profesor profesorOut = Profesor::fromJson(profesorJson);
        MateriaEnPlan mepOut = MateriaEnPlan::fromJson(mepJson);
        PlanEstudio planOut = PlanEstudio::fromJson(planJson);
        Asignacion asigOut = Asignacion::fromJson(asigJson);
        Horario horarioOut = Horario::fromJson(horarioJson);

        // ── Verificaciones ──
        QCOMPARE(aulaOut.nombre, aula.nombre);
        QCOMPARE(aulaOut.capacidad, aula.capacidad);
        QCOMPARE(aulaOut.locacion, aula.locacion);

        QCOMPARE(franjaOut.dia, f1.dia);
        QCOMPARE(franjaOut.inicio, f1.inicio);
        QCOMPARE(franjaOut.fin, f1.fin);

        QCOMPARE(materiaOut.nombre, materia.nombre);
        QCOMPARE(materiaOut.horas_semanales, materia.horas_semanales);
        QCOMPARE(materiaOut.requerimientos, materia.requerimientos);

        QCOMPARE(profesorOut.nombre, profesor.nombre);
        QCOMPARE(profesorOut.disponibilidad.size(), 2);
        QCOMPARE(profesorOut.disponibilidad[0].dia, f1.dia);
        QCOMPARE(profesorOut.materias, profesor.materias);

        QCOMPARE(mepOut.curso, mep.curso);
        QCOMPARE(mepOut.horas, mep.horas);
        QCOMPARE(mepOut.materia.nombre, mep.materia.nombre);

        QCOMPARE(planOut.nombre, plan.nombre);
        QCOMPARE(planOut.materias.size(), 1);
        QCOMPARE(planOut.materias[0].curso, mep.curso);

        QCOMPARE(asigOut.aula.nombre, aula.nombre);
        QCOMPARE(asigOut.franja_horaria.dia, f1.dia);
        QCOMPARE(asigOut.materia.nombre, materia.nombre);
        QCOMPARE(asigOut.profesor.nombre, profesor.nombre);

        QCOMPARE(horarioOut.asignaciones.size(), 1);
        QCOMPARE(horarioOut.asignaciones[0].materia.nombre, materia.nombre);
    }
};

QTEST_MAIN(TestEntityGraph)
#include "test_entity_graph.moc"
