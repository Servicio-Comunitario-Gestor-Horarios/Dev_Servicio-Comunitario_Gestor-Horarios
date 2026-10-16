#include <QTest>

#include <backend/services/ValidadorSalidaSolver.hpp>
#include <backend/solver/config/contrato_solver.hpp>

/// Tests de las validaciones posteriores de la salida del solver (P1–P4, RF-3).
class TestValidadorSalida : public QObject
{
    Q_OBJECT

private:
    SolverConfig configBase() const
    {
        SolverConfig config;
        config.aula_fija = true;
        config.materias = {MateriaSolverConfig{QStringLiteral("Matemática")}};
        config.aulas = {AulaSolverConfig{QStringLiteral("A1"), QStringLiteral("regular"), 30}};
        config.planes = {PlanSolverConfig{QStringLiteral("Ciencias"),
                                          {PlanMateriaConfig{0, 1, 2}}}};

        CursoSolverConfig cursoA;
        cursoA.anio = 1; cursoA.seccion = QStringLiteral("A");
        cursoA.num_estudiantes = 20; cursoA.plan = QStringLiteral("Ciencias"); cursoA.aula = 0;
        CursoSolverConfig cursoB;
        cursoB.anio = 1; cursoB.seccion = QStringLiteral("B");
        cursoB.num_estudiantes = 20; cursoB.plan = QStringLiteral("Ciencias"); cursoB.aula = 0;
        config.cursos = {cursoA, cursoB};

        ProfesorSolverConfig prof;
        prof.nombre = QStringLiteral("Ana");
        prof.horas_requeridas = 4;
        prof.horas_aula = 4;
        config.profesores = {prof};
        return config;
    }

    /// Horario válido: cubre las 2 horas de la materia, sin solapamientos.
    HorarioSalida horarioValido() const
    {
        HorarioSalida h;
        CursoOutput curso;
        curso.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{0, 0, 0, 0}, AsignacionOutput{1, 0, 0, 0}};
        curso.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("A"))] = curso;
        return h;
    }

private slots:

    void horarioValido_esPresentableSinAvisos()
    {
        const AnalisisSalida a = analizarSalidaSolver(horarioValido(), configBase());
        QVERIFY(a.presentable());
        QVERIFY(!a.hayConflictoSolapamiento());
        QVERIFY(a.conflictos.isEmpty());
        QVERIFY(a.avisos.isEmpty());
    }

    void solapamientoDeProfesor_noPresentable()
    {
        // Segundo curso que pone al mismo profesor en el mismo día/slot.
        HorarioSalida h = horarioValido();
        CursoOutput otro;
        otro.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{0, 0, 0, 0}};
        otro.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("B"))] = otro;

        const AnalisisSalida a = analizarSalidaSolver(h, configBase());
        QVERIFY(!a.presentable());
        QVERIFY(a.hayConflictoSolapamiento());
        QVERIFY(!a.conflictos.isEmpty());
    }

    void solapamientoDeAula_noPresentable()
    {
        // Dos cursos distintos que usan la misma aula en el mismo día/slot.
        HorarioSalida h = horarioValido();
        CursoOutput otro;
        otro.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{0, 0, -1, 0}};  // profesor -1, misma aula 0
        otro.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("B"))] = otro;

        const AnalisisSalida a = analizarSalidaSolver(h, configBase());
        QVERIFY(!a.presentable());
    }

    void horasNoCubiertas_generaAviso()
    {
        // El curso debería tener 2 horas; solo se asigna 1.
        HorarioSalida h = horarioValido();
        h.horarios[claveCurso(1, QStringLiteral("A"))].dias[0].asignaciones = {AsignacionOutput{0, 0, 0, 0}};

        const AnalisisSalida a = analizarSalidaSolver(h, configBase());
        QVERIFY(a.presentable());
        QVERIFY(!a.avisos.isEmpty());
    }

    void excesoDeHorasDeProfesor_generaAviso()
    {
        SolverConfig config = configBase();
        config.profesores[0].horas_requeridas = 1;  // se asignan 2

        const AnalisisSalida a = analizarSalidaSolver(horarioValido(), config);
        QVERIFY(a.presentable());
        QVERIFY(!a.avisos.isEmpty());
    }

    void excesoDeCapacidadDeAula_generaAviso()
    {
        SolverConfig config = configBase();
        config.aulas[0].capacidad = 10;  // el curso tiene 20 estudiantes

        const AnalisisSalida a = analizarSalidaSolver(horarioValido(), config);
        QVERIFY(a.presentable());
        QVERIFY(!a.avisos.isEmpty());
    }
};

QTEST_MAIN(TestValidadorSalida)
#include "test_validador_salida.moc"
