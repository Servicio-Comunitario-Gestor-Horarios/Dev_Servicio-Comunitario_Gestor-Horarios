/**
 * @file test_variables_cp_sat.cpp
 * @brief Tests unitarios para la creación de variables CP-SAT (crearVariables).
 *
 * Cubre: pruning por titularidad, disponibilidad, turnos, aulas fijas/móviles,
 * múltiples profesores, y conteo exacto de BoolVar creadas.
 *
 * Framework: Google Test
 */

#include <gtest/gtest.h>
#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"

using namespace operations_research::sat;

// ─── Fixture ────────────────────────────────────────────────────────────────

/**
 * @brief Construye un SolverConfig mínimo y válido para tests.
 *
 * Config por defecto:
 *   - 1 profesor (Ana), materias: {0}, horas: 4
 *   - 1 curso (1ro A), turno: "manana", materia: {0} con 4h/semana
 *   - 1 materia (Matemática)
 *   - 1 aula (A1), capacidad: 30, fija (índice 0)
 *   - 5 días, 12 slots (6 mañana + 6 tarde)
 *   - Turno mañana: slots 0-5, Turno tarde: slots 6-11
 *   - Franja: 60 min/slot, 6 slots/turno
 */
struct VariablesTest : public ::testing::Test {
    SolverConfig makeBaseConfig() {
        SolverConfig cfg;
        cfg.version = "1.0";

        // Dimensiones
        cfg.dimensiones = {1, 1, 1, 1, 5, 12};

        // Franja horaria
        cfg.franja_horaria = {60, 6};

        // Turnos
        TurnoConfig manana{"manana", {0, 1, 2, 3, 4, 5}};
        TurnoConfig tarde{"tarde", {6, 7, 8, 9, 10, 11}};
        cfg.turnos.turnos = {manana, tarde};

        // Recesos (vacío)
        cfg.recesos = {};

        // Cursos
        CursoSolverConfig curso;
        curso.nombre = "1ro A";
        curso.turno = "manana";
        curso.aula_fija = 0;
        curso.num_estudiantes = 20;
        curso.plan = "General";
        curso.materias = {{0, 4}};  // materia_idx=0, 4h/semana
        cfg.cursos = {curso};

        // Profesores
        ProfesorSolverConfig prof;
        prof.nombre = "Ana";
        prof.horas_requeridas = 4;
        prof.horas_planificacion = 4;
        prof.materias_asignadas = {0};
        prof.materias_suplente = {};
        // Disponibilidad: lunes a viernes, slots 0-5 (mañana)
        for (int d = 0; d < 5; ++d) {
            prof.disponibilidad.append({d, {0, 1, 2, 3, 4, 5}});
        }
        cfg.profesores = {prof};

        // Materias
        cfg.materias = {{"Matemática"}};

        // Aulas
        cfg.aulas = {{"A1", "regular", 30}};

        // Planificación / Generación / Penalizaciones
        cfg.planificacion = {false};
        cfg.generacion = {{0}};
        cfg.penalizaciones = {100, 50};

        return cfg;
    }
};

// ─── Tests: Assignment variables ────────────────────────────────────────────

TEST_F(VariablesTest, EmptyConfig_NoVariablesCreated) {
    // Arrange
    SolverConfig cfg;
    cfg.version = "1.0";
    cfg.dimensiones = {0, 0, 0, 0, 5, 12};
    cfg.franja_horaria = {60, 6};
    TurnoConfig manana{"manana", {0, 1, 2, 3, 4, 5}};
    cfg.turnos.turnos = {manana};

    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert
    EXPECT_TRUE(vars.assignment.isEmpty());
    EXPECT_TRUE(vars.aulaAssignment.isEmpty());
}

TEST_F(VariablesTest, OneProfessorOneCourse_CreatesCorrectVariables) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: 1 profesor × 1 materia × 1 curso × 5 días × 6 slots = 30
    // Pero solo si el profesor está disponible Y el slot está en el turno.
    // Ana está disponible L-V en slots 0-5, turno manana = slots 0-5 → 30 variables
    EXPECT_EQ(vars.assignment.size(), 30);
}

TEST_F(VariablesTest, AssignmentKeysAreCorrect) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: verificar que una key específica existe
    // Ana(p=0) - Matemática(m=0) - 1ro A(c=0) - Lunes(d=0) - slot 0
    std::tuple<int,int,int,int,int> expectedKey{0, 0, 0, 0, 0};
    EXPECT_TRUE(vars.assignment.contains(expectedKey));

    // Verificar que un slot fuera del turno NO existe
    // slot 6 es tarde, turno es mañana
    std::tuple<int,int,int,int,int> wrongSlot{0, 0, 0, 0, 6};
    EXPECT_FALSE(vars.assignment.contains(wrongSlot));
}

TEST_F(VariablesTest, ProfessorNotTeachingSubject_NoVariables) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.profesores[0].materias_asignadas = {};  // Ana no enseña nada
    cfg.profesores[0].materias_suplente = {};
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert
    EXPECT_TRUE(vars.assignment.isEmpty());
}

TEST_F(VariablesTest, ProfessorNotAvailable_NoVariables) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.profesores[0].disponibilidad = {};  // Sin disponibilidad
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert
    EXPECT_TRUE(vars.assignment.isEmpty());
}

TEST_F(VariablesTest, ProfessorAvailableOnlySomeDays_CreatesPartialVariables) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    // Ana solo disponible lunes y miércoles
    cfg.profesores[0].disponibilidad = {
        {0, {0, 1, 2, 3, 4, 5}},  // lunes
        {2, {0, 1, 2, 3, 4, 5}}   // miércoles
    };
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: 2 días × 6 slots = 12
    EXPECT_EQ(vars.assignment.size(), 12);
}

TEST_F(VariablesTest, SlotOutsideTurno_NotCreated) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    // Ana disponible en slots 0-11 (todos), pero turno solo 0-5
    for (int d = 0; d < 5; ++d) {
        cfg.profesores[0].disponibilidad[d].slot = {0,1,2,3,4,5,6,7,8,9,10,11};
    }
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: solo slots 0-5 (turno manana), NO 6-11
    for (int d = 0; d < 5; ++d) {
        for (int s = 0; s <= 5; ++s) {
            EXPECT_TRUE(vars.assignment.contains({0, 0, 0, d, s}))
                << "Falta variable en día " << d << " slot " << s;
        }
        for (int s = 6; s <= 11; ++s) {
            EXPECT_FALSE(vars.assignment.contains({0, 0, 0, d, s}))
                << "Variable incorrecta en día " << d << " slot " << s;
        }
    }
}

TEST_F(VariablesTest, TurnoTarde_CreatesSlots6To11) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.cursos[0].turno = "tarde";
    // Profesor disponible en tarde (slots 6-11)
    cfg.profesores[0].disponibilidad.clear();
    for (int d = 0; d < 5; ++d) {
        cfg.profesores[0].disponibilidad.append({d, {6, 7, 8, 9, 10, 11}});
    }
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: solo slots 6-11
    for (int d = 0; d < 5; ++d) {
        for (int s = 6; s <= 11; ++s) {
            EXPECT_TRUE(vars.assignment.contains({0, 0, 0, d, s}))
                << "Falta variable tarde en día " << d << " slot " << s;
        }
        for (int s = 0; s <= 5; ++s) {
            EXPECT_FALSE(vars.assignment.contains({0, 0, 0, d, s}))
                << "Variable mañana incorrecta en día " << d << " slot " << s;
        }
    }
    EXPECT_EQ(vars.assignment.size(), 30);
}

// ─── Tests: Suplente ────────────────────────────────────────────────────────

TEST_F(VariablesTest, SubstituteProfessor_CreatesVariables) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    // Ana NO es titular de materia 0
    cfg.profesores[0].materias_asignadas = {};
    // Pero es suplente
    cfg.profesores[0].materias_suplente = {{0, 10}};
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: suplente genera variables igual que titular
    EXPECT_EQ(vars.assignment.size(), 30);
}

TEST_F(VariablesTest, TwoProfessorsSameSubject_BothCreateVariables) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.dimensiones.num_profesores = 2;

    ProfesorSolverConfig bob;
    bob.nombre = "Bob";
    bob.horas_requeridas = 4;
    bob.horas_planificacion = 4;
    bob.materias_asignadas = {0};
    bob.materias_suplente = {};
    for (int d = 0; d < 5; ++d) {
        bob.disponibilidad.append({d, {0, 1, 2, 3, 4, 5}});
    }
    cfg.profesores.append(bob);
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: 2 prof × 1 mat × 1 curso × 5 días × 6 slots = 60
    EXPECT_EQ(vars.assignment.size(), 60);
}

TEST_F(VariablesTest, TwoProfessorsDifferentAvailability_CorrectPruning) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.dimensiones.num_profesores = 2;

    // Bob solo disponible lunes
    ProfesorSolverConfig bob;
    bob.nombre = "Bob";
    bob.horas_requeridas = 4;
    bob.horas_planificacion = 4;
    bob.materias_asignadas = {0};
    bob.materias_suplente = {};
    bob.disponibilidad = {{0, {0, 1, 2, 3, 4, 5}}};  // solo lunes
    cfg.profesores.append(bob);
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: Ana=30 + Bob=6 (1 día × 6 slots) = 36
    EXPECT_EQ(vars.assignment.size(), 36);
}

// ─── Tests: Aula assignment ─────────────────────────────────────────────────

TEST_F(VariablesTest, FixedAula_NoAulaAssignmentCreated) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.cursos[0].aula_fija = 0;  // Aula fija
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert
    EXPECT_TRUE(vars.aulaAssignment.isEmpty());
}

TEST_F(VariablesTest, MobileAula_CreatesAulaAssignment) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.cursos[0].aula_fija = -1;  // Aula móvil
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: 1 curso × 5 días × 6 slots × 1 aula = 30
    EXPECT_EQ(vars.aulaAssignment.size(), 30);
}

TEST_F(VariablesTest, MobileAula_MultipleAulas) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.cursos[0].aula_fija = -1;
    cfg.dimensiones.num_aulas = 3;
    cfg.aulas = {{"A1", "regular", 30}, {"A2", "lab", 25}, {"A3", "regular", 40}};
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: 1 curso × 5 días × 6 slots × 3 aulas = 90
    EXPECT_EQ(vars.aulaAssignment.size(), 90);
}

TEST_F(VariablesTest, MobileAula_KeysAreCorrect) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.cursos[0].aula_fija = -1;
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: verificar key específica
    // curso=0, día=0, slot=0, aula=0
    std::tuple<int,int,int,int> expectedKey{0, 0, 0, 0};
    EXPECT_TRUE(vars.aulaAssignment.contains(expectedKey));

    // Slot 6 (tarde) no debería existir para turno mañana
    std::tuple<int,int,int,int> wrongSlot{0, 0, 6, 0};
    EXPECT_FALSE(vars.aulaAssignment.contains(wrongSlot));
}

// ─── Tests: Edge cases ──────────────────────────────────────────────────────

TEST_F(VariablesTest, MultipleCourses_DifferentTurnos) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.dimensiones.num_cursos = 2;

    CursoSolverConfig curso2;
    curso2.nombre = "2do B";
    curso2.turno = "tarde";
    curso2.aula_fija = 0;
    curso2.num_estudiantes = 25;
    curso2.plan = "General";
    curso2.materias = {{0, 4}};
    cfg.cursos.append(curso2);

    // Profesor disponible todo el día
    cfg.profesores[0].disponibilidad.clear();
    for (int d = 0; d < 5; ++d) {
        cfg.profesores[0].disponibilidad.append({d, {0,1,2,3,4,5,6,7,8,9,10,11}});
    }
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: curso1(mañana)=30 + curso2(tarde)=30 = 60
    EXPECT_EQ(vars.assignment.size(), 60);
}

TEST_F(VariablesTest, MultipleMateriasPerCourse) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.dimensiones.num_materias = 2;
    cfg.materias = {{"Matemática"}, {"Español"}};
    cfg.cursos[0].materias = {{0, 2}, {1, 2}};  // 2h cada una
    cfg.profesores[0].materias_asignadas = {0, 1};
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: 1 prof × 2 mat × 1 curso × 5 días × 6 slots = 60
    EXPECT_EQ(vars.assignment.size(), 60);
}

TEST_F(VariablesTest, ProfessorTeachesOnlyOneOfTwoMaterias) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.dimensiones.num_materias = 2;
    cfg.materias = {{"Matemática"}, {"Español"}};
    cfg.cursos[0].materias = {{0, 2}, {1, 2}};
    cfg.profesores[0].materias_asignadas = {0};  // Solo matemática
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: solo materia 0 → 30 variables
    EXPECT_EQ(vars.assignment.size(), 30);

    // Verificar que no hay keys con materia 1
    for (const auto& key : vars.assignment.keys()) {
        EXPECT_NE(std::get<1>(key), 1)
            << "No debería haber variable para materia 1";
    }
}

TEST_F(VariablesTest, MixedFixedAndMobileAulas) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.dimensiones.num_cursos = 2;
    cfg.dimensiones.num_aulas = 2;

    // Curso 1: aula fija
    // (ya creado en makeBaseConfig con aula_fija=0)

    // Curso 2: aula móvil
    CursoSolverConfig curso2;
    curso2.nombre = "2do B";
    curso2.turno = "manana";
    curso2.aula_fija = -1;
    curso2.num_estudiantes = 25;
    curso2.plan = "General";
    curso2.materias = {{0, 4}};
    cfg.cursos.append(curso2);

    cfg.aulas = {{"A1", "regular", 30}, {"A2", "lab", 25}};
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: aulaAssignment solo para curso 2 (móvil)
    // 1 curso × 5 días × 6 slots × 2 aulas = 60
    EXPECT_EQ(vars.aulaAssignment.size(), 60);

    // Verificar keys: curso 0 no debería estar en aulaAssignment
    for (const auto& key : vars.aulaAssignment.keys()) {
        EXPECT_EQ(std::get<0>(key), 1)
            << "aulaAssignment solo debería ser para curso 1 (móvil)";
    }
}

TEST_F(VariablesTest, NoAulas_NoAulaAssignmentEvenIfMobile) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    cfg.cursos[0].aula_fija = -1;
    cfg.dimensiones.num_aulas = 0;
    cfg.aulas = {};
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert
    EXPECT_TRUE(vars.aulaAssignment.isEmpty());
    // Assignment variables still created
    EXPECT_EQ(vars.assignment.size(), 30);
}

TEST_F(VariablesTest, ProfessorAvailableInSomeSlotsOnly) {
    // Arrange
    SolverConfig cfg = makeBaseConfig();
    // Ana solo disponible slots 0 y 1 (de los 6 del turno mañana)
    cfg.profesores[0].disponibilidad.clear();
    for (int d = 0; d < 5; ++d) {
        cfg.profesores[0].disponibilidad.append({d, {0, 1}});
    }
    CpModelBuilder model;

    // Act
    VariablesSolver vars = crearVariables(model, cfg);

    // Assert: 5 días × 2 slots = 10
    EXPECT_EQ(vars.assignment.size(), 10);
}
