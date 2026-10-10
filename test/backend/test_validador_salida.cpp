#include <QTest>

#include <backend/services/ValidadorSalidaSolver.hpp>

/// Tests de las validaciones posteriores de la salida del solver (P1–P4, RF-3).
class TestValidadorSalida : public QObject
{
    Q_OBJECT

private:
    SolverConfig configBase() const
    {
        SolverConfig config;
        config.materias = {MateriaSolverConfig{QStringLiteral("Matemática")}};
        config.aulas = {AulaSolverConfig{QStringLiteral("A1"), QStringLiteral("regular"), 30}};

        CursoSolverConfig curso;
        curso.nombre = QStringLiteral("1ro A");
        curso.num_estudiantes = 20;
        MateriaCurso mc; mc.materiaIDx = 0; mc.horasSemanales = 2;
        curso.materias = {mc};
        config.cursos = {curso};

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
        h.horarios[QStringLiteral("1ro A")] = curso;
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
        h.horarios[QStringLiteral("1ro B")] = otro;

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
        h.horarios[QStringLiteral("1ro B")] = otro;

        const AnalisisSalida a = analizarSalidaSolver(h, configBase());
        QVERIFY(!a.presentable());
    }

    void horasNoCubiertas_generaAviso()
    {
        // El curso debería tener 2 horas; solo se asigna 1.
        HorarioSalida h = horarioValido();
        h.horarios[QStringLiteral("1ro A")].dias[0].asignaciones = {AsignacionOutput{0, 0, 0, 0}};

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
