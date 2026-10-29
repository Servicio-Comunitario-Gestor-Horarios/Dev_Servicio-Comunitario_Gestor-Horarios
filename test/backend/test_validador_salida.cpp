#include <QTest>

#include <backend/services/ValidadorSalidaSolver.hpp>
#include <backend/solver/config/contrato_solver.hpp>

/// Tests de las validaciones posteriores de la salida del solver (P2 y P4, RF-3).
///
/// Desde la 010 el solver informa el **aula real** de cada clase, también en el modo de aula
/// fija: un curso puede tener clases en aulas distintas (su aula de referencia y las aulas a
/// las que se desplaza). Estos casos fijan que P3/P4 miran `asignacion.aula` y no el aula de
/// referencia del curso (RF-4). P4 indexa la ocupación por (docente, día, slot) y (aula, día,
/// slot), no solo por (día, slot): dos cursos en paralelo con aulas y docentes distintos no son
/// un conflicto.
class TestValidadorSalida : public QObject
{
    Q_OBJECT

private:
    /// Configuración coherente con la regla de tipo de la 010: cada materia declara el tipo
    /// de aula que requiere y cada aula declara el suyo. El curso 1A usa A1 (tipo «Aula de
    /// estudio») como aula de referencia.
    SolverConfig configBase() const
    {
        SolverConfig config;
        config.aula_fija = true;
        config.materias = {
            MateriaSolverConfig{QStringLiteral("Matemática"), QStringLiteral("Aula de estudio")},
            MateriaSolverConfig{QStringLiteral("Tecnología"), QStringLiteral("Laboratorio")}};
        config.aulas = {
            AulaSolverConfig{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30},
            AulaSolverConfig{QStringLiteral("Laboratorio"), QStringLiteral("Laboratorio"), 30}};
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

    /// Configuración del curso 1A con 1 hora de Matemática (que cabe en su aula de
    /// referencia) y 1 de Tecnología (desplazada al laboratorio). El laboratorio no aloja a
    /// los 20 estudiantes del curso; el aula de referencia sí.
    SolverConfig configConMateriaDesplazada() const
    {
        SolverConfig config = configBase();
        config.planes[0].materias = {PlanMateriaConfig{0, 1, 1}, PlanMateriaConfig{1, 1, 1}};
        config.aulas[1].capacidad = 10;
        return config;
    }

    /// Curso 1A en modo de aula fija con clases en aulas distintas: Matemática en su aula de
    /// referencia (A1) y Tecnología desplazada al laboratorio. Es lo que emite el solver
    /// desde T4 (`asignacion.aula` es el aula real).
    HorarioSalida horarioConMateriaDesplazada() const
    {
        HorarioSalida h;
        CursoOutput curso;
        curso.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{0, 0, 0, 0}, AsignacionOutput{1, 1, 0, 1}};
        curso.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("A"))] = curso;
        return h;
    }

    /// Configuración con un segundo docente (Beto), para poder poner dos cursos en paralelo
    /// con docentes distintos.
    SolverConfig configConDosDocentes() const
    {
        SolverConfig config = configBase();
        ProfesorSolverConfig beto;
        beto.nombre = QStringLiteral("Beto");
        beto.horas_requeridas = -1;  // sin tope de horas
        beto.horas_aula = -1;
        config.profesores.append(beto);
        return config;
    }

    /// Dos cursos en paralelo en la misma franja, cada uno con su aula y su docente: es lo que
    /// emite el solver con más de un curso (13 en el seed). Cada curso cubre sus 2 horas.
    HorarioSalida horarioDosCursosEnParalelo() const
    {
        HorarioSalida h;

        CursoOutput a;
        a.turno = QStringLiteral("manana");
        DiaOutput diaA;
        diaA.dia = 0;
        // 1A: las dos clases en su aula de referencia (A1) con Ana.
        diaA.asignaciones = {AsignacionOutput{0, 0, 0, 0}, AsignacionOutput{1, 0, 0, 0}};
        a.dias = {diaA};
        h.horarios[claveCurso(1, QStringLiteral("A"))] = a;

        CursoOutput b;
        b.turno = QStringLiteral("manana");
        DiaOutput diaB;
        diaB.dia = 0;
        // 1B: las dos clases desplazadas al laboratorio con Beto, en las mismas franjas.
        diaB.asignaciones = {AsignacionOutput{0, 0, 1, 1}, AsignacionOutput{1, 0, 1, 1}};
        b.dias = {diaB};
        h.horarios[claveCurso(1, QStringLiteral("B"))] = b;
        return h;
    }

    /// Conflictos (P4) en un solo texto. Los avisos (P2) no cuentan como conflicto.
    static QString conflictos(const AnalisisSalida& a)
    {
        return a.conflictos.join(QLatin1Char('\n'));
    }

    /// Todos los mensajes del análisis, solo para el diagnóstico de un fallo.
    static QString texto(const AnalisisSalida& a)
    {
        return conflictos(a) + QLatin1Char('\n') + a.avisos.join(QLatin1Char('\n'));
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

    /// Mismo docente y misma franja en dos cursos distintos; el aula es distinta, así que el
    /// único conflicto posible es el del docente.
    void solapamientoDeProfesor_noPresentable()
    {
        HorarioSalida h = horarioValido();
        CursoOutput otro;
        otro.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{0, 0, 0, 1}};  // mismo profesor 0, aula 1
        otro.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("B"))] = otro;

        const AnalisisSalida a = analizarSalidaSolver(h, configBase());
        QVERIFY(!a.presentable());
        QVERIFY(a.hayConflictoSolapamiento());
        QVERIFY2(conflictos(a).contains(QStringLiteral("docente «Ana»")), qPrintable(texto(a)));
        QVERIFY2(!conflictos(a).contains(QStringLiteral("aula")), qPrintable(texto(a)));
    }

    /// Mismo curso dos veces en la misma franja (P4 se conserva tal cual). El docente y el
    /// aula de las dos clases son distintos para aislar el conflicto del propio curso.
    void mismoCursoDosVecesEnLaFranja_noPresentable()
    {
        HorarioSalida h = horarioValido();
        h.horarios[claveCurso(1, QStringLiteral("A"))].dias[0].asignaciones = {
            AsignacionOutput{0, 0, 0, 0}, AsignacionOutput{0, 1, 1, 1}};

        const AnalisisSalida a = analizarSalidaSolver(h, configConDosDocentes());
        QVERIFY(!a.presentable());
        QVERIFY2(conflictos(a).contains(QStringLiteral("dos clases")), qPrintable(texto(a)));
        QVERIFY2(!conflictos(a).contains(QStringLiteral("docente")), qPrintable(texto(a)));
        QVERIFY2(!conflictos(a).contains(QStringLiteral("aula")), qPrintable(texto(a)));
    }

    void solapamientoDeAula_noPresentable()
    {
        // Dos cursos distintos que usan la misma aula de referencia en el mismo día/slot.
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
        // El conflicto de aula nombra el aula real informada por el solver (el docente de 1B
        // no está asignado, así que no hay conflicto de docente).
        QVERIFY2(conflictos(a).contains(QStringLiteral("aula «A1»")), qPrintable(texto(a)));
        QVERIFY2(!conflictos(a).contains(QStringLiteral("docente")), qPrintable(texto(a)));
    }

    void horasNoCubiertas_noGeneraAviso()
    {
        // P1 se retiró en la 005 (I2): un curso con menos horas de las esperadas ya no avisa.
        HorarioSalida h = horarioValido();
        h.horarios[claveCurso(1, QStringLiteral("A"))].dias[0].asignaciones = {AsignacionOutput{0, 0, 0, 0}};

        const AnalisisSalida a = analizarSalidaSolver(h, configBase());
        QVERIFY(a.presentable());
        QVERIFY2(a.avisos.isEmpty(), qPrintable(texto(a)));
    }

    void excesoDeHorasDeProfesor_generaAviso()
    {
        SolverConfig config = configBase();
        config.profesores[0].horas_requeridas = 1;  // se asignan 2

        const AnalisisSalida a = analizarSalidaSolver(horarioValido(), config);
        QVERIFY(a.presentable());
        QVERIFY(!a.avisos.isEmpty());
    }

    void excesoDeCapacidadDeAula_noGeneraAviso()
    {
        // P3 se retiró en la 005 (I2): la capacidad del aula ya no genera aviso.
        SolverConfig config = configBase();
        config.aulas[0].capacidad = 10;  // el curso tiene 20 estudiantes

        const AnalisisSalida a = analizarSalidaSolver(horarioValido(), config);
        QVERIFY(a.presentable());
        QVERIFY2(a.avisos.isEmpty(), qPrintable(texto(a)));
    }

    /// RF-4: un curso en modo de aula fija con clases en aulas distintas no genera conflictos
    /// (P4 usa el aula real de cada clase) ni avisos (la capacidad se retiró con P3, 005 I2).
    void aulaFijaConClasesEnAulasDistintas_esPresentableSinAvisos()
    {
        const SolverConfig config = configConMateriaDesplazada();
        QVERIFY(config.aula_fija);
        QCOMPARE(config.cursos[0].aula, 0);  // la referencia del curso es A1

        const AnalisisSalida a = analizarSalidaSolver(horarioConMateriaDesplazada(), config);

        // El horario es coherente: una clase por franja, horas cubiertas y sin solapes.
        QVERIFY2(a.presentable(), qPrintable(texto(a)));
        QVERIFY(a.conflictos.isEmpty());
        QVERIFY2(a.avisos.isEmpty(), qPrintable(texto(a)));
    }

    /// RF-3/RF-4: la exclusividad de aula cubre las clases desplazadas; dos clases en el
    /// mismo laboratorio y la misma franja no son presentables.
    void dosClasesDesplazadasEnElMismoAulaYfranja_noPresentable()
    {
        HorarioSalida h = horarioConMateriaDesplazada();
        CursoOutput otro;
        otro.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{1, 1, -1, 1}};  // mismo slot 1 y mismo laboratorio
        otro.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("B"))] = otro;

        const AnalisisSalida a = analizarSalidaSolver(h, configConMateriaDesplazada());
        QVERIFY(!a.presentable());
        QVERIFY(a.hayConflictoSolapamiento());
        QVERIFY2(conflictos(a).contains(QStringLiteral("aula «Laboratorio»")),
                 qPrintable(texto(a)));
    }

    /// P4 no debe confundir cursos distintos: dos clases en la misma franja con aulas y
    /// docentes distintos no son un solapamiento (el defecto anterior indexaba solo por
    /// día/slot y denunciaba cualquier paralelismo).
    void dosCursosEnParalelo_conAulasYdocentesDistintos_esPresentable()
    {
        const AnalisisSalida a = analizarSalidaSolver(horarioDosCursosEnParalelo(), configConDosDocentes());
        QVERIFY2(a.conflictos.isEmpty(), qPrintable(texto(a)));
        QVERIFY2(a.presentable(), qPrintable(texto(a)));
        QVERIFY2(a.avisos.isEmpty(), qPrintable(texto(a)));
    }

    /// Dos clases sin docente ni aula asignados comparten la franja: no hay recurso que
    /// colisione, así que no son un conflicto entre sí (índices negativos = ausentes).
    void docenteYaulaSinAsignar_noSonConflictoEntreSi()
    {
        HorarioSalida h;
        for (const QString& seccion : {QStringLiteral("A"), QStringLiteral("B")})
        {
            CursoOutput curso;
            curso.turno = QStringLiteral("manana");
            DiaOutput dia;
            dia.dia = 0;
            dia.asignaciones = {AsignacionOutput{0, 0, -1, -1}};
            curso.dias = {dia};
            h.horarios[claveCurso(1, seccion)] = curso;
        }

        const AnalisisSalida a = analizarSalidaSolver(h, configBase());
        QVERIFY2(a.conflictos.isEmpty(), qPrintable(texto(a)));
        QVERIFY(a.presentable());
    }

    /// Una salida incoherente sigue rechazándose: dos clases que informan el mismo aula
    /// inexistente en la misma franja colisionan entre sí. P4 nombra el aula como desconocida.
    /// Un aula inexistente **sola** no es un conflicto (P4 solo detecta colisiones, no valida
    /// el catálogo).
    void aulaInexistenteRepetidaEnLaFranja_noPresentable()
    {
        const SolverConfig config = configConMateriaDesplazada();
        QCOMPARE(config.aulas.size(), 2);

        HorarioSalida h = horarioConMateriaDesplazada();
        // 1A informa un aula 7 inexistente (0..1) en su clase desplazada...
        h.horarios[claveCurso(1, QStringLiteral("A"))].dias[0].asignaciones[1].aula = 7;
        QVERIFY2(analizarSalidaSolver(h, config).presentable(), "un aula inexistente sola no es conflicto");

        // ...y 1B informa esa misma aula 7 en la misma franja: colisión real entre dos cursos.
        CursoOutput otro;
        otro.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{1, 1, -1, 7}};
        otro.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("B"))] = otro;

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY(!a.presentable());
        QVERIFY(a.hayConflictoSolapamiento());
        QVERIFY2(conflictos(a).contains(QStringLiteral("aula «(desconocida)»")),
                 qPrintable(texto(a)));
    }
};

QTEST_MAIN(TestValidadorSalida)
#include "test_validador_salida.moc"
