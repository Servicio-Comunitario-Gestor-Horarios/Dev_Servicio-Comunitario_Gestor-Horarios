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
        config.turnos.turnos = {TurnoConfig{QStringLiteral("manana"), {0, 1, 2, 3}}};
        config.eventos = {EventoConfig{QStringLiteral("Receso"), QStringLiteral("manana"), 2, 1, 10,
                                       QStringLiteral("08:20"), QStringLiteral("08:30")}};
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
        cursoA.turno = QStringLiteral("manana");
        CursoSolverConfig cursoB;
        cursoB.anio = 1; cursoB.seccion = QStringLiteral("B");
        cursoB.num_estudiantes = 20; cursoB.plan = QStringLiteral("Ciencias"); cursoB.aula = 0;
        cursoB.turno = QStringLiteral("manana");
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

    /// 018: configuración con una cancha (aforo 2) añadida al catálogo, en el índice 2.
    /// Conserva el turno de 4 franjas y el receso tras la segunda (segmentos [0,1] y [2,3]).
    static constexpr int kCancha = 2;

    SolverConfig configConCancha() const
    {
        SolverConfig config = configConDosDocentes();
        config.aulas.append(
            AulaSolverConfig{QStringLiteral("Cancha"), QStringLiteral("Cancha"), 88});
        return config;
    }

    /// Curso del plan «Ciencias» con aula de referencia A1.
    static CursoSolverConfig cursoDelPlan(const QString& seccion)
    {
        CursoSolverConfig c;
        c.anio = 1;
        c.seccion = seccion;
        c.turno = QStringLiteral("manana");
        c.num_estudiantes = 20;
        c.plan = QStringLiteral("Ciencias");
        c.aula = 0;
        return c;
    }

    /// Dos clases (franjas `slotA` y `slotB` del día 0) del mismo curso, materia 0, con el
    /// docente y el aula indicados.
    static CursoOutput cursoEnFranjas(const QString& /*seccion*/, int slotA, int slotB, int aula,
                                      int profesor)
    {
        CursoOutput curso;
        curso.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{slotA, 0, profesor, aula},
                            AsignacionOutput{slotB, 0, profesor, aula}};
        curso.dias = {dia};
        return curso;
    }

    /// Dos clases en el par 0–1 del día 0 (posición de par completa).
    static CursoOutput cursoEnPar(int aula, int profesor)
    {
        return cursoEnFranjas(QString(), 0, 1, aula, profesor);
    }

    /// Docente adicional sin tope de horas, para que dos cursos no choquen por docente.
    static ProfesorSolverConfig docente(const QString& nombre)
    {
        ProfesorSolverConfig p;
        p.nombre = nombre;
        p.horas_requeridas = -1;
        p.horas_aula = -1;
        return p;
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

    void dosCursosMismoDiaYSlot_distintoDocenteYAula_esPresentable()
    {
        // Regresión: dos cursos en la misma franja con docentes y aulas
        // distintos NO son un solapamiento. La clave de ocupación debe incluir
        // la entidad (docente/aula), no solo (día, slot).
        SolverConfig config = configBase();
        ProfesorSolverConfig luis;
        luis.nombre = QStringLiteral("Luis");
        luis.horas_requeridas = 4;
        luis.horas_aula = 4;
        config.profesores.append(luis);
        config.aulas.append(AulaSolverConfig{QStringLiteral("A2"), QStringLiteral("Aula de estudio"), 30});

        HorarioSalida h = horarioValido();  // curso 1A: docente 0 en aula 0
        CursoOutput otro;
        otro.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        // Curso 1B en el mismo día y slots, con otro docente y otra aula.
        dia.asignaciones = {AsignacionOutput{0, 0, 1, 1}, AsignacionOutput{1, 0, 1, 1}};
        otro.dias = {dia};
        h.horarios[claveCurso(1, QStringLiteral("B"))] = otro;

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY(a.conflictos.isEmpty());
        QVERIFY(a.presentable());
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

    // ── 018 T3: comprobación de la salida con aforo y pares cruzados ──────────────────────

    /// RF-7: dos cursos distintos con docentes distintos en la cancha, en la misma posición de
    /// par, son válidos; el aforo es un máximo, no una exclusividad de una clase por aula.
    void dosCursosEnLaCanchaEnElMismoPar_esPresentable()
    {
        const SolverConfig config = configConCancha();

        HorarioSalida h;
        h.horarios[claveCurso(1, QStringLiteral("A"))] = cursoEnPar(kCancha, 0);
        h.horarios[claveCurso(1, QStringLiteral("B"))] = cursoEnPar(kCancha, 1);

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY2(a.conflictos.isEmpty(), qPrintable(texto(a)));
        QVERIFY(a.presentable());
    }

    /// RF-7: tres clases en la cancha en la misma franja superan su aforo 2 y se marcan con el
    /// aula, el día, la franja, el número de clases y el aforo.
    void tresCursosEnLaCancha_esConflictoDeAforo()
    {
        SolverConfig config = configConCancha();
        config.cursos.append(cursoDelPlan(QStringLiteral("C")));
        config.profesores.append(docente(QStringLiteral("Carla")));

        HorarioSalida h;
        h.horarios[claveCurso(1, QStringLiteral("A"))] = cursoEnPar(kCancha, 0);
        h.horarios[claveCurso(1, QStringLiteral("B"))] = cursoEnPar(kCancha, 1);
        h.horarios[claveCurso(1, QStringLiteral("C"))] = cursoEnPar(kCancha, 2);

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY(!a.presentable());
        QVERIFY2(conflictos(a).contains(QStringLiteral("aula «Cancha»")), qPrintable(texto(a)));
        QVERIFY2(conflictos(a).contains(QStringLiteral("3 clases")), qPrintable(texto(a)));
        QVERIFY2(conflictos(a).contains(QStringLiteral("aforo es 2")), qPrintable(texto(a)));
    }

    /// RF-7: un aula de aforo 1 sigue sin admitir dos clases en la misma franja, y el mensaje
    /// informa del número de clases y del aforo.
    void dosCursosEnAulaDeAforoUno_esConflicto()
    {
        const SolverConfig config = configConCancha();

        HorarioSalida h;
        h.horarios[claveCurso(1, QStringLiteral("A"))] = cursoEnPar(0, 0);
        h.horarios[claveCurso(1, QStringLiteral("B"))] = cursoEnPar(0, 1);

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY(!a.presentable());
        QVERIFY2(conflictos(a).contains(QStringLiteral("aula «A1»")), qPrintable(texto(a)));
        QVERIFY2(conflictos(a).contains(QStringLiteral("2 clases")), qPrintable(texto(a)));
        QVERIFY2(conflictos(a).contains(QStringLiteral("aforo es 1")), qPrintable(texto(a)));
    }

    /// RF-7: un aula cuyo índice no existe tiene aforo 1; dos clases en ella son conflicto y se
    /// nombra como desconocida.
    void aulaInexistenteConAforoUno_dosClasesEsConflicto()
    {
        const SolverConfig config = configConCancha();
        QCOMPARE(config.aulas.size(), 3);   // A1, Laboratorio y Cancha

        HorarioSalida h;
        h.horarios[claveCurso(1, QStringLiteral("A"))] = cursoEnPar(9, 0);
        h.horarios[claveCurso(1, QStringLiteral("B"))] = cursoEnPar(9, 1);

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY(!a.presentable());
        QVERIFY2(conflictos(a).contains(QStringLiteral("aula «(desconocida)»")),
                 qPrintable(texto(a)));
        QVERIFY2(conflictos(a).contains(QStringLiteral("aforo es 1")), qPrintable(texto(a)));
    }

    /// RF-7: una clase sin aula declarada (índice negativo) no cuenta como ocupación: no crea
    /// ni alivia el conflicto de aforo de la cancha.
    void claseSinAula_noCuentaParaElAforo()
    {
        SolverConfig config = configConCancha();
        config.cursos.append(cursoDelPlan(QStringLiteral("C")));

        HorarioSalida h;
        // Dos cursos en la cancha (aforo 2, justo en el límite)...
        h.horarios[claveCurso(1, QStringLiteral("A"))] = cursoEnPar(kCancha, 0);
        h.horarios[claveCurso(1, QStringLiteral("B"))] = cursoEnPar(kCancha, 1);
        // ...y un tercero sin aula en las mismas franjas: no ocupa la cancha.
        h.horarios[claveCurso(1, QStringLiteral("C"))] = cursoEnPar(-1, -1);

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY2(a.conflictos.isEmpty(), qPrintable(texto(a)));
        QVERIFY(a.presentable());
    }

    /// RF-5/RF-7: un par cruzado (un curso en 0–1 y otro en 1–2 de la cancha) comparte una sola
    /// franja de cada bloque y se marca como conflicto.
    void parCruzadoEnLaCancha_esConflicto()
    {
        const SolverConfig config = configConCancha();

        HorarioSalida h;
        h.horarios[claveCurso(1, QStringLiteral("A"))] = cursoEnFranjas(QString(), 0, 1, kCancha, 0);
        h.horarios[claveCurso(1, QStringLiteral("B"))] = cursoEnFranjas(QString(), 1, 2, kCancha, 1);

        const AnalisisSalida a = analizarSalidaSolver(h, config);
        QVERIFY(!a.presentable());
        QVERIFY2(conflictos(a).contains(
                     QStringLiteral("ocupa la cancha en una sola franja del bloque")),
                 qPrintable(texto(a)));
    }
};

QTEST_MAIN(TestValidadorSalida)
#include "test_validador_salida.moc"
