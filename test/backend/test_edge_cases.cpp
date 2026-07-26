#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <backend/data/franja_horaria.hpp>
#include <backend/data/aula.hpp>
#include <backend/data/materia.hpp>
#include <backend/data/profesor.hpp>
#include <backend/data/horario.hpp>
#include <backend/data/plan_estudio.hpp>

class TestEdgeCases : public QObject {
    Q_OBJECT

private slots:
    // ─── FranjaHoraria ──────────────────────────────────────────
    void franjaHoraria_diaDomingo();
    void franjaHoraria_diaSabado();
    void franjaHoraria_inicioIgualFin();
    void franjaHoraria_inicioMayorQueFin();

    // ─── Aula ───────────────────────────────────────────────────
    void aula_capacidadMinima();
    void aula_capacidadMaxima();
    void aula_nombreUnChar();
    void aula_nombreLargo();
    void aula_fromJsonVacio();

    // ─── Materia ────────────────────────────────────────────────
    void materia_horasMinimas();
    void materia_horasMaximas();
    void materia_requerimientosVacios();
    void materia_fromJsonVacio();

    // ─── Profesor ───────────────────────────────────────────────
    void profesor_disponibilidadVacia();
    void profesor_materiasVacias();
    void profesor_nombreConAcentos();
    void profesor_fromJsonVacio();

    // ─── Asignacion ─────────────────────────────────────────────
    void asignacion_roundTripCompleto();
    void asignacion_toJsonFromJsonConsistencia();

    // ─── Horario ────────────────────────────────────────────────
    void horario_cienAsignaciones();
    void horario_asignacionesDuplicadas();

    // ─── PlanEstudio ────────────────────────────────────────────
    void planEstudio_nombreVacio();
    void planEstudio_nombreLargo();
    void materiaEnPlan_cursoCero();
    void materiaEnPlan_horasCero();
};

// ═══════════════════════════════════════════════════════════════
// FranjaHoraria — casos borde
// ═══════════════════════════════════════════════════════════════

void TestEdgeCases::franjaHoraria_diaDomingo()
{
    FranjaHoraria f;
    f.dia = 0;
    f.inicio = QTime(8, 0);
    f.fin = QTime(10, 0);

    QJsonObject json = f.toJson();
    FranjaHoraria result = FranjaHoraria::fromJson(json);

    QCOMPARE(result.dia, 0);
    QCOMPARE(result.inicio, QTime(8, 0));
    QCOMPARE(result.fin, QTime(10, 0));
}

void TestEdgeCases::franjaHoraria_diaSabado()
{
    FranjaHoraria f;
    f.dia = 6;
    f.inicio = QTime(8, 0);
    f.fin = QTime(12, 0);

    QJsonObject json = f.toJson();
    FranjaHoraria result = FranjaHoraria::fromJson(json);

    QCOMPARE(result.dia, 6);
    QCOMPARE(result.inicio, QTime(8, 0));
    QCOMPARE(result.fin, QTime(12, 0));
}

void TestEdgeCases::franjaHoraria_inicioIgualFin()
{
    FranjaHoraria f;
    f.dia = 1;
    f.inicio = QTime(12, 0);
    f.fin = QTime(12, 0);

    QJsonObject json = f.toJson();
    FranjaHoraria result = FranjaHoraria::fromJson(json);

    QCOMPARE(result.inicio, result.fin);
}

void TestEdgeCases::franjaHoraria_inicioMayorQueFin()
{
    FranjaHoraria f;
    f.dia = 2;
    f.inicio = QTime(18, 0);
    f.fin = QTime(8, 0);

    QJsonObject json = f.toJson();
    FranjaHoraria result = FranjaHoraria::fromJson(json);

    QVERIFY(result.inicio > result.fin);
}

// ═══════════════════════════════════════════════════════════════
// Aula — casos borde
// ═══════════════════════════════════════════════════════════════

void TestEdgeCases::aula_capacidadMinima()
{
    Aula a;
    a.nombre = "Minima";
    a.capacidad = 1;
    a.locacion = "Test";

    QJsonObject json = a.toJson();
    Aula result = Aula::fromJson(json);

    QCOMPARE(result.capacidad, 1);
}

void TestEdgeCases::aula_capacidadMaxima()
{
    Aula a;
    a.nombre = "Gimnasio";
    a.capacidad = 500;
    a.locacion = "Deportes";

    QJsonObject json = a.toJson();
    Aula result = Aula::fromJson(json);

    QCOMPARE(result.capacidad, 500);
}

void TestEdgeCases::aula_nombreUnChar()
{
    Aula a;
    a.nombre = "A";
    a.capacidad = 30;
    a.locacion = "Edificio A";

    QJsonObject json = a.toJson();
    Aula result = Aula::fromJson(json);

    QCOMPARE(result.nombre, QString("A"));
}

void TestEdgeCases::aula_nombreLargo()
{
    Aula a;
    a.nombre = QString(255, 'X');
    a.capacidad = 30;
    a.locacion = "Edificio Largo";

    QJsonObject json = a.toJson();
    Aula result = Aula::fromJson(json);

    QCOMPARE(result.nombre.length(), 255);
}

void TestEdgeCases::aula_fromJsonVacio()
{
    QJsonObject empty;
    Aula result = Aula::fromJson(empty);

    QVERIFY(result.nombre.isEmpty());
    QCOMPARE(result.capacidad, 0);
}

// ═══════════════════════════════════════════════════════════════
// Materia — casos borde
// ═══════════════════════════════════════════════════════════════

void TestEdgeCases::materia_horasMinimas()
{
    Materia m;
    m.nombre = "Seminario";
    m.horas_semanales = 1;
    m.requerimientos = {"Pre-requisito"};

    QJsonObject json = m.toJson();
    Materia result = Materia::fromJson(json);

    QCOMPARE(result.horas_semanales, 1);
}

void TestEdgeCases::materia_horasMaximas()
{
    Materia m;
    m.nombre = "Intensivo";
    m.horas_semanales = 40;
    m.requerimientos = {};

    QJsonObject json = m.toJson();
    Materia result = Materia::fromJson(json);

    QCOMPARE(result.horas_semanales, 40);
}

void TestEdgeCases::materia_requerimientosVacios()
{
    Materia m;
    m.nombre = "Intro";
    m.horas_semanales = 3;
    m.requerimientos = {};

    QJsonObject json = m.toJson();
    Materia result = Materia::fromJson(json);

    QVERIFY(result.requerimientos.isEmpty());
}

void TestEdgeCases::materia_fromJsonVacio()
{
    QJsonObject empty;
    Materia result = Materia::fromJson(empty);

    QVERIFY(result.nombre.isEmpty());
    QCOMPARE(result.horas_semanales, 0);
}

// ═══════════════════════════════════════════════════════════════
// Profesor — casos borde
// ═══════════════════════════════════════════════════════════════

void TestEdgeCases::profesor_disponibilidadVacia()
{
    Profesor p;
    p.nombre = "Sin Horario";
    p.disponibilidad = {};
    p.materias = {"Matematica"};

    QJsonObject json = p.toJson();
    Profesor result = Profesor::fromJson(json);

    QVERIFY(result.disponibilidad.isEmpty());
}

void TestEdgeCases::profesor_materiasVacias()
{
    Profesor p;
    p.nombre = "Sin Materias";
    p.disponibilidad = {{1, QTime(8, 0), QTime(10, 0)}};
    p.materias = {};

    QJsonObject json = p.toJson();
    Profesor result = Profesor::fromJson(json);

    QVERIFY(result.materias.isEmpty());
}

void TestEdgeCases::profesor_nombreConAcentos()
{
    Profesor p;
    p.nombre = "Dr. José María García-López";
    p.disponibilidad = {};
    p.materias = {};

    QJsonObject json = p.toJson();
    Profesor result = Profesor::fromJson(json);

    QCOMPARE(result.nombre, QString("Dr. José María García-López"));
}

void TestEdgeCases::profesor_fromJsonVacio()
{
    QJsonObject empty;
    Profesor result = Profesor::fromJson(empty);

    QVERIFY(result.nombre.isEmpty());
    QVERIFY(result.disponibilidad.isEmpty());
    QVERIFY(result.materias.isEmpty());
}

// ═══════════════════════════════════════════════════════════════
// Asignacion — round-trip y consistencia
// ═══════════════════════════════════════════════════════════════

void TestEdgeCases::asignacion_roundTripCompleto()
{
    Asignacion original;
    original.aula = {"A-101", 30, "Edificio A"};
    original.franja_horaria = {1, QTime(8, 0), QTime(10, 0)};
    original.materia = {"Calculo I", 4, {"Pre-algebra"}};
    original.profesor = {"Dr. Perez", {}, {"Calculo I"}};

    QJsonObject json = original.toJson();
    Asignacion result = Asignacion::fromJson(json);

    QCOMPARE(result.aula.nombre, QString("A-101"));
    QCOMPARE(result.aula.capacidad, 30);
    QCOMPARE(result.franja_horaria.dia, 1);
    QCOMPARE(result.franja_horaria.inicio, QTime(8, 0));
    QCOMPARE(result.materia.nombre, QString("Calculo I"));
    QCOMPARE(result.materia.horas_semanales, 4);
    QCOMPARE(result.profesor.nombre, QString("Dr. Perez"));
}

void TestEdgeCases::asignacion_toJsonFromJsonConsistencia()
{
    Asignacion original;
    original.aula = {"Lab-3", 20, "Ciencias"};
    original.franja_horaria = {3, QTime(14, 0), QTime(16, 30)};
    original.materia = {"Quimica", 3, {}};
    original.profesor = {"Ing. Lopez", {}, {"Quimica", "Fisica"}};

    QJsonObject json1 = original.toJson();
    Asignacion temp = Asignacion::fromJson(json1);
    QJsonObject json2 = temp.toJson();

    QCOMPARE(QJsonDocument(json1), QJsonDocument(json2));
}

// ═══════════════════════════════════════════════════════════════
// Horario — casos borde
// ═══════════════════════════════════════════════════════════════

void TestEdgeCases::horario_cienAsignaciones()
{
    Horario h;
    for (int i = 0; i < 100; i++) {
        Asignacion a;
        a.aula = {QString("Aula-%1").arg(i), 30, "Edificio"};
        a.franja_horaria = {i % 7, QTime(8, 0), QTime(10, 0)};
        a.materia = {QString("Materia-%1").arg(i), 3, {}};
        a.profesor = {QString("Prof-%1").arg(i), {}, {}};
        h.asignaciones.append(a);
    }

    QJsonObject json = h.toJson();
    Horario result = Horario::fromJson(json);

    QCOMPARE(result.asignaciones.size(), 100);
    QCOMPARE(result.asignaciones[50].aula.nombre, QString("Aula-50"));
}

void TestEdgeCases::horario_asignacionesDuplicadas()
{
    Horario h;
    Asignacion a;
    a.aula = {"A-101", 30, "Edificio A"};
    a.franja_horaria = {1, QTime(8, 0), QTime(10, 0)};
    a.materia = {"Calculo I", 4, {}};
    a.profesor = {"Dr. Perez", {}, {}};

    h.asignaciones.append(a);
    h.asignaciones.append(a);
    h.asignaciones.append(a);

    QJsonObject json = h.toJson();
    Horario result = Horario::fromJson(json);

    QCOMPARE(result.asignaciones.size(), 3);
    QCOMPARE(result.asignaciones[0].aula.nombre, result.asignaciones[2].aula.nombre);
}

// ═══════════════════════════════════════════════════════════════
// PlanEstudio — casos borde
// ═══════════════════════════════════════════════════════════════

void TestEdgeCases::planEstudio_nombreVacio()
{
    PlanEstudio plan;
    plan.nombre = "";
    plan.materias = {};

    QJsonObject json = plan.toJson();
    PlanEstudio result = PlanEstudio::fromJson(json);

    QVERIFY(result.nombre.isEmpty());
}

void TestEdgeCases::planEstudio_nombreLargo()
{
    PlanEstudio plan;
    plan.nombre = QString(500, 'P');
    plan.materias = {};

    QJsonObject json = plan.toJson();
    PlanEstudio result = PlanEstudio::fromJson(json);

    QCOMPARE(result.nombre.length(), 500);
}

void TestEdgeCases::materiaEnPlan_cursoCero()
{
    MateriaEnPlan mep;
    mep.materia = {"Matematica", 3, {"Pre-algebra"}};
    mep.curso = 0;
    mep.horas = 3;

    QJsonObject json = mep.toJson();
    MateriaEnPlan result = MateriaEnPlan::fromJson(json);

    QCOMPARE(result.curso, 0);
    QCOMPARE(result.horas, 3);
}

void TestEdgeCases::materiaEnPlan_horasCero()
{
    MateriaEnPlan mep;
    mep.materia = {"Fisica", 4, {}};
    mep.curso = 1;
    mep.horas = 0;

    QJsonObject json = mep.toJson();
    MateriaEnPlan result = MateriaEnPlan::fromJson(json);

    QCOMPARE(result.horas, 0);
}

QTEST_MAIN(TestEdgeCases)
#include "test_edge_cases.moc"
