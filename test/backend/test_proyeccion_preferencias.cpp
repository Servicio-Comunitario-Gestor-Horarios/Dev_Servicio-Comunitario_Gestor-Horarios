#include <gtest/gtest.h>

#include <QTime>
#include <QVector>

#include "backend/data/materia.hpp"
#include "backend/data/plan_estudio.hpp"
#include "backend/services/ConstructorEntradaSolver.hpp"
#include "backend/services/proyeccion_preferencias.hpp"

/// Tests de la rejilla compartida y de la proyección reloj → franja de las preferencias
/// (RF-3 de la 013).
///
/// Fijan la unidad de la preferencia proyectada —día de la GENERACIÓN (0 = lunes) y slots
/// absolutos del día—, la regla de la franja (bloque ENTERO dentro del rango, todos los turnos,
/// los eventos no cortan) y los tres motivos de no aplicada con precedencia (c) > (a) > (b).
namespace {

/// Escenario de reloj de la rejilla: mañana 07:00–11:00 (4 franjas de 60 min, slots 0–3) y
/// tarde 13:00–16:00 (3 franjas, slots 4–6). Los turnos llegan DESORDENADOS a propósito.
DatosDominio datosBase()
{
    DatosDominio d;

    MateriaDTO materia;
    materia.id = 1;
    materia.nombre = QStringLiteral("Matemática");
    materia.idTipoAula = 1;
    materia.tipoAula = QStringLiteral("Aula de estudio");
    d.materias = {materia};

    AulaDTO aula;
    aula.id = 10;
    aula.nombre = QStringLiteral("A1");
    aula.capacidad = 30;
    aula.idTipoAula = 1;
    aula.tipoAula = QStringLiteral("Aula de estudio");
    d.aulas = {aula};

    TurnoDTO tarde;
    tarde.nombre = QStringLiteral("tarde");
    tarde.inicio = QTime(13, 0);
    tarde.fin = QTime(16, 0);
    tarde.numSlots = 3;

    TurnoDTO manana;
    manana.nombre = QStringLiteral("manana");
    manana.inicio = QTime(7, 0);
    manana.fin = QTime(11, 0);
    manana.numSlots = 4;

    d.turnos = {tarde, manana};

    CursoDTO curso;
    curso.id = 100;
    curso.anio = 1;
    curso.seccion = QStringLiteral("A");
    curso.turno = QStringLiteral("manana");
    curso.aula = 10;
    d.cursos = {curso};

    Materia materiaPlan;
    materiaPlan.nombre = QStringLiteral("Matemática");
    MateriaEnPlan enPlan;
    enPlan.curso = 1;
    enPlan.horas = 2;
    enPlan.materia = materiaPlan;
    PlanEstudio plan;
    plan.codigo = QStringLiteral("Ciencias");
    plan.nombre = QStringLiteral("Ciencias");
    plan.materias = {enPlan};
    d.planes = {plan};

    return d;
}

/// Configuración PARCIAL equivalente a la que construye la entrada: turnos con sus slots
/// absolutos (los de la tarde continúan donde terminan los de la mañana), un curso de mañana
/// y su plan embebido. Sin docentes: los añaden los tests.
SolverConfig configuracionBase()
{
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = true;
    config.franja_horaria.duracion_minutos = 60;
    config.franja_horaria.slots_por_turno = 4;
    config.dimensiones.num_dias = 5;
    config.dimensiones.num_slots_dia = 7;
    config.dimensiones.num_materias = 1;
    config.dimensiones.num_aulas = 1;
    config.dimensiones.num_cursos = 1;

    TurnoConfig manana;
    manana.nombre = QStringLiteral("manana");
    manana.slot = {0, 1, 2, 3};
    manana.inicio = QStringLiteral("07:00");
    manana.fin = QStringLiteral("11:00");

    TurnoConfig tarde;
    tarde.nombre = QStringLiteral("tarde");
    tarde.slot = {4, 5, 6};
    tarde.inicio = QStringLiteral("13:00");
    tarde.fin = QStringLiteral("16:00");

    config.turnos.turnos = {manana, tarde};

    CursoSolverConfig curso;
    curso.anio = 1;
    curso.seccion = QStringLiteral("A");
    curso.turno = QStringLiteral("manana");
    curso.plan = QStringLiteral("Ciencias");
    curso.num_estudiantes = 20;
    curso.aula = 0;
    config.cursos = {curso};

    PlanSolverConfig plan;
    plan.codigo = QStringLiteral("Ciencias");
    plan.materias = {PlanMateriaConfig{0, 1, 2}};   // materia 0, año 1, 2 h
    config.planes = {plan};

    config.materias = {MateriaSolverConfig{QStringLiteral("Matemática"),
                                           QStringLiteral("Aula de estudio")}};
    config.aulas = {AulaSolverConfig{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30}};

    return config;
}

FranjaHoraria franja(int dia, int horaInicio, int horaFin)
{
    return FranjaHoraria{dia, QTime(horaInicio, 0), QTime(horaFin, 0)};
}

PreferenciaDTO preferencia(int dia, int horaInicio, int horaFin, SentidoPreferencia sentido,
                           int id = 0)
{
    PreferenciaDTO p;
    p.id = id;
    p.franja = franja(dia, horaInicio, horaFin);
    p.sentido = sentido;
    return p;
}

/// Añade un docente al escenario, con `datos.docentes` y `config.profesores` en el MISMO orden
/// (el índice es el que viaja en la preferencia proyectada). `candidato` decide si opta por la
/// materia del curso; en los dos casos declara disponibilidad el lunes por la mañana.
int agregarDocente(DatosDominio& datos, SolverConfig& config, const QString& nombre,
                   bool candidato)
{
    ProfesorDTO docente;
    docente.id = nombre;
    docente.nombre = nombre;
    DisponibilidadDTO disp;
    disp.franja = franja(1, 7, 11);
    docente.disponibilidad = {disp};
    if (candidato) {
        MateriaAsignadaDTO materia;
        materia.id = 1;
        materia.nombre = QStringLiteral("Matemática");
        docente.materias = {materia};
    }
    datos.docentes.append(docente);

    ProfesorSolverConfig prof;
    prof.nombre = nombre;
    prof.horas_aula = 10;
    prof.disponibilidad = {Disponibilidad{0, {0, 1, 2, 3}}};
    if (candidato)
        prof.materias_asignadas = {0};
    config.profesores.append(prof);
    config.dimensiones.num_profesores = config.profesores.size();

    return config.profesores.size() - 1;
}

void agregarPreferencia(DatosDominio& datos, int docente, const PreferenciaDTO& preferencia)
{
    datos.docentes[docente].preferencias.append(preferencia);
}

} // namespace

// ─── Rejilla compartida: ordenarTurnos y slotsDeFranja ─────────────────────────

TEST(TestProyeccionPreferencias, rejilla_ordenaTurnosPorInicioYAcumulaOffsets)
{
    const QVector<TurnoProyectado> turnos = ordenarTurnos(datosBase().turnos);

    ASSERT_EQ(turnos.size(), 2);
    EXPECT_EQ(turnos[0].nombre, QStringLiteral("manana"));
    EXPECT_EQ(turnos[0].offset, 0);
    EXPECT_EQ(turnos[0].numSlots, 4);
    EXPECT_EQ(turnos[1].nombre, QStringLiteral("tarde"));
    EXPECT_EQ(turnos[1].offset, 4);   // continúa donde terminan los slots de la mañana
    EXPECT_EQ(turnos[1].numSlots, 3);
}

TEST(TestProyeccionPreferencias, rejilla_soloAlcanzaBloquesEnterosDentroDelRango)
{
    const QVector<TurnoProyectado> turnos = ordenarTurnos(datosBase().turnos);

    // 07:00–09:00 contiene enteros los bloques 0 (07:00–08:00) y 1 (08:00–09:00).
    EXPECT_TRUE(slotsDeFranja(franja(1, 7, 9), turnos, 60) == QVector<int>({0, 1}));
    // 07:30–08:30 no contiene ningún bloque completo: no hay crédito parcial.
    EXPECT_TRUE(slotsDeFranja(FranjaHoraria{1, QTime(7, 30), QTime(8, 30)}, turnos, 60).isEmpty());
    // 07:30–09:30 solo contiene entero el bloque 1 (08:00–09:00).
    EXPECT_TRUE(slotsDeFranja(FranjaHoraria{1, QTime(7, 30), QTime(9, 30)}, turnos, 60) ==
                QVector<int>({1}));
}

TEST(TestProyeccionPreferencias, rejilla_bordesDeFranjaSinCreditoParcial)
{
    const QVector<TurnoProyectado> turnos = ordenarTurnos(datosBase().turnos);

    // Un bloque exacto entra.
    EXPECT_TRUE(slotsDeFranja(franja(1, 7, 8), turnos, 60) == QVector<int>({0}));
    EXPECT_TRUE(slotsDeFranja(franja(1, 10, 11), turnos, 60) == QVector<int>({3}));
    // Un rango que empieza antes del turno solo alcanza los bloques enteros.
    EXPECT_TRUE(slotsDeFranja(franja(1, 6, 7), turnos, 60).isEmpty());
    // Un rango que acaba después del turno también.
    EXPECT_TRUE(slotsDeFranja(franja(1, 8, 12), turnos, 60) == QVector<int>({1, 2, 3}));
    // Empieza antes del turno y acaba pasado el último bloque: los cuatro caben enteros.
    EXPECT_TRUE(slotsDeFranja(franja(1, 6, 12), turnos, 60) == QVector<int>({0, 1, 2, 3}));
    // Rango en el hueco entre turnos: no alcanza ninguna franja.
    EXPECT_TRUE(slotsDeFranja(franja(1, 11, 13), turnos, 60).isEmpty());
}

TEST(TestProyeccionPreferencias, rejilla_recorreTodosLosTurnosDelDia)
{
    const QVector<TurnoProyectado> turnos = ordenarTurnos(datosBase().turnos);

    // 13:00–16:00 es el turno de la tarde completo.
    EXPECT_TRUE(slotsDeFranja(franja(1, 13, 16), turnos, 60) == QVector<int>({4, 5, 6}));
    // 11:00–14:00 solo contiene el primer bloque de la tarde (13:00–14:00).
    EXPECT_TRUE(slotsDeFranja(franja(1, 11, 14), turnos, 60) == QVector<int>({4}));
}

TEST(TestProyeccionPreferencias, rejilla_cruceEntreTurnosAcumulaMananaYTarde)
{
    const QVector<TurnoProyectado> turnos = ordenarTurnos(datosBase().turnos);

    // 10:00–14:00 cruza el final de la mañana y el inicio de la tarde.
    EXPECT_TRUE(slotsDeFranja(franja(1, 10, 14), turnos, 60) == QVector<int>({3, 4}));
    // El día completo acumula todos los slots, ordenados.
    EXPECT_TRUE(slotsDeFranja(franja(1, 7, 16), turnos, 60) == QVector<int>({0, 1, 2, 3, 4, 5, 6}));
}

TEST(TestProyeccionPreferencias, rejilla_elEventoNoCortaNiDesplazaLaRejilla)
{
    DatosDominio datos = datosBase();
    // Receso tras la franja 2 del turno de la mañana: 09:00–09:15 (no corta el rango).
    EventoDTO receso;
    receso.nombre = QStringLiteral("Receso");
    receso.despuesDeSlot = 2;
    receso.orden = 1;
    receso.duracion = 15;
    receso.inicio = QTime(9, 0);
    receso.fin = QTime(9, 15);
    for (TurnoDTO& turno : datos.turnos)
        if (turno.nombre == QStringLiteral("manana"))
            turno.eventos = {receso};

    const QVector<TurnoProyectado> turnos = ordenarTurnos(datos.turnos);

    // El rango que abarca el evento alcanza todas las franjas del turno.
    EXPECT_TRUE(slotsDeFranja(franja(1, 7, 11), turnos, 60) == QVector<int>({0, 1, 2, 3}));
    // Y la rejilla no se desplaza: la franja 2 sigue siendo 09:00–10:00 y la 3, 10:00–11:00.
    EXPECT_TRUE(slotsDeFranja(franja(1, 9, 10), turnos, 60) == QVector<int>({2}));
    EXPECT_TRUE(slotsDeFranja(FranjaHoraria{1, QTime(9, 15), QTime(10, 15)}, turnos, 60)
                    .isEmpty());
}

// ─── Filtro de día ────────────────────────────────────────────────────────────

TEST(TestProyeccionPreferencias, domingo_noAplicaPorDiaFueraDeSemana)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    agregarPreferencia(datos, ana, preferencia(0, 7, 9, SentidoPreferencia::Preferida));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.intervinientes.isEmpty());
    ASSERT_EQ(proyeccion.noAplicadas.size(), 1);
    EXPECT_EQ(proyeccion.noAplicadas[0].motivo, MotivoPreferencia::DiaFueraDeSemana);
    EXPECT_EQ(proyeccion.noAplicadas[0].docente, ana);
    EXPECT_EQ(proyeccion.noAplicadas[0].franja.dia, 0);   // día de DOMINIO
    EXPECT_EQ(proyeccion.noAplicadas[0].sentido, SentidoPreferencia::Preferida);
}

TEST(TestProyeccionPreferencias, sabadoNoDeclarado_noAplica)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();   // num_dias = 5 (lunes a viernes)
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    agregarPreferencia(datos, ana, preferencia(6, 7, 9, SentidoPreferencia::Evitada));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.intervinientes.isEmpty());
    ASSERT_EQ(proyeccion.noAplicadas.size(), 1);
    EXPECT_EQ(proyeccion.noAplicadas[0].motivo, MotivoPreferencia::DiaFueraDeSemana);
}

TEST(TestProyeccionPreferencias, sabadoDeclarado_aplicaYConvierteElDiaDeLaGeneracion)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    config.dimensiones.num_dias = 6;   // lunes a sábado
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    agregarPreferencia(datos, ana, preferencia(6, 7, 9, SentidoPreferencia::Evitada));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.noAplicadas.isEmpty());
    ASSERT_EQ(proyeccion.intervinientes.size(), 1);
    EXPECT_EQ(proyeccion.intervinientes[0].docente, ana);
    EXPECT_EQ(proyeccion.intervinientes[0].dia, 5);   // sábado (dominio 6) → generación 5
    EXPECT_EQ(proyeccion.intervinientes[0].sentido, SentidoPreferencia::Evitada);
    EXPECT_TRUE(proyeccion.intervinientes[0].slot == QVector<int>({0, 1}));
}

// ─── «Participa» y motivos (c) > (a) > (b) ───────────────────────────────────

TEST(TestProyeccionPreferencias, docenteQueNoParticipa_motivoDocenteNoParticipa)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    const int luis = agregarDocente(datos, config, QStringLiteral("Luis"), false);
    agregarPreferencia(datos, luis, preferencia(1, 7, 9, SentidoPreferencia::Evitada));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.intervinientes.isEmpty());
    ASSERT_EQ(proyeccion.noAplicadas.size(), 1);
    EXPECT_EQ(proyeccion.noAplicadas[0].motivo, MotivoPreferencia::DocenteNoParticipa);
    EXPECT_EQ(proyeccion.noAplicadas[0].docente, luis);
}

TEST(TestProyeccionPreferencias, preferenciaDeTardeConDocenteSoloDeManana_motivoSinFranjaDelTurno)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();   // único curso de mañana
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    agregarPreferencia(datos, ana, preferencia(1, 13, 14, SentidoPreferencia::Preferida));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.intervinientes.isEmpty());
    ASSERT_EQ(proyeccion.noAplicadas.size(), 1);
    EXPECT_EQ(proyeccion.noAplicadas[0].motivo, MotivoPreferencia::SinFranjaDelTurno);
    EXPECT_EQ(proyeccion.noAplicadas[0].franja.dia, 1);
}

TEST(TestProyeccionPreferencias, rangoSinNingunaFranjaDeLaRejilla_motivoSinFranjaDelTurno)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    // 11:00–13:00 es el hueco entre turnos: no alcanza ninguna franja.
    agregarPreferencia(datos, ana, preferencia(1, 11, 13, SentidoPreferencia::Evitada));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.intervinientes.isEmpty());
    ASSERT_EQ(proyeccion.noAplicadas.size(), 1);
    EXPECT_EQ(proyeccion.noAplicadas[0].motivo, MotivoPreferencia::SinFranjaDelTurno);
}

TEST(TestProyeccionPreferencias, motivos_precedenciaDocenteNoParticipaGanaADiaYFranja)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    const int luis = agregarDocente(datos, config, QStringLiteral("Luis"), false);

    // Luis no participa y su preferencia además es de domingo y por la tarde: el motivo es (c).
    agregarPreferencia(datos, luis, preferencia(0, 13, 14, SentidoPreferencia::Preferida));
    // Ana participa: el domingo es (a) aunque tampoco alcance franjas del turno.
    agregarPreferencia(datos, ana, preferencia(0, 13, 14, SentidoPreferencia::Evitada));
    // Ana en un día declarado pero sin franjas de su turno: (b).
    agregarPreferencia(datos, ana, preferencia(1, 13, 14, SentidoPreferencia::Preferida));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.intervinientes.isEmpty());
    // Orden de recorrido: docentes en el orden de los datos y, dentro, el de la 012.
    ASSERT_EQ(proyeccion.noAplicadas.size(), 3);
    EXPECT_EQ(proyeccion.noAplicadas[0].docente, ana);
    EXPECT_EQ(proyeccion.noAplicadas[0].motivo, MotivoPreferencia::DiaFueraDeSemana);
    EXPECT_EQ(proyeccion.noAplicadas[1].docente, ana);
    EXPECT_EQ(proyeccion.noAplicadas[1].motivo, MotivoPreferencia::SinFranjaDelTurno);
    EXPECT_EQ(proyeccion.noAplicadas[2].docente, luis);
    EXPECT_EQ(proyeccion.noAplicadas[2].motivo, MotivoPreferencia::DocenteNoParticipa);
}

// ─── Intervinientes: franjas, cruce de turnos y orden canónico ────────────────

TEST(TestProyeccionPreferencias, interveniente_llevaDiaDeLaGeneracionYFranjasAlcanzadas)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    // Martes (dominio 2) de 07:00 a 09:00 → día de generación 1 y franjas {0, 1}.
    agregarPreferencia(datos, ana, preferencia(2, 7, 9, SentidoPreferencia::Preferida));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.noAplicadas.isEmpty());
    ASSERT_EQ(proyeccion.intervinientes.size(), 1);
    EXPECT_EQ(proyeccion.intervinientes[0].docente, ana);
    EXPECT_EQ(proyeccion.intervinientes[0].dia, 1);
    EXPECT_EQ(proyeccion.intervinientes[0].sentido, SentidoPreferencia::Preferida);
    EXPECT_TRUE(proyeccion.intervinientes[0].slot == QVector<int>({0, 1}));
}

TEST(TestProyeccionPreferencias, interveniente_queCruzaTurnosEsUnaEntradaConSusFranjas)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    // Segundo curso de tarde: la misma materia y plan, turno de la tarde.
    CursoSolverConfig cursoTarde;
    cursoTarde.anio = 1;
    cursoTarde.seccion = QStringLiteral("B");
    cursoTarde.turno = QStringLiteral("tarde");
    cursoTarde.plan = QStringLiteral("Ciencias");
    cursoTarde.num_estudiantes = 20;
    cursoTarde.aula = 0;
    config.cursos.append(cursoTarde);
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);
    // Ana pasa a estar disponible en toda la jornada, así que también es candidata de tarde.
    config.profesores[ana].disponibilidad[0].slot = {0, 1, 2, 3, 4, 5, 6};

    // 10:00–14:00 cruza el final de la mañana y el inicio de la tarde sin fusionarse.
    agregarPreferencia(datos, ana, preferencia(1, 10, 14, SentidoPreferencia::Evitada));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    EXPECT_TRUE(proyeccion.noAplicadas.isEmpty());
    ASSERT_EQ(proyeccion.intervinientes.size(), 1);   // una entrada, no una por turno
    EXPECT_TRUE(proyeccion.intervinientes[0].slot == QVector<int>({3, 4}));
}

TEST(TestProyeccionPreferencias, intervinientes_salenEnOrdenCanonico)
{
    DatosDominio datos = datosBase();
    SolverConfig config = configuracionBase();
    const int ana = agregarDocente(datos, config, QStringLiteral("Ana"), true);

    // Declaradas fuera de orden a propósito: el resultado se canoniza por día.
    agregarPreferencia(datos, ana, preferencia(4, 7, 9, SentidoPreferencia::Preferida, 4));
    agregarPreferencia(datos, ana, preferencia(2, 7, 9, SentidoPreferencia::Preferida, 2));
    agregarPreferencia(datos, ana, preferencia(1, 7, 9, SentidoPreferencia::Preferida, 1));

    const ProyeccionPreferencias proyeccion = proyectarPreferencias(datos, config, 60);

    ASSERT_EQ(proyeccion.intervinientes.size(), 3);
    EXPECT_EQ(proyeccion.intervinientes[0].dia, 0);
    EXPECT_EQ(proyeccion.intervinientes[1].dia, 1);
    EXPECT_EQ(proyeccion.intervinientes[2].dia, 3);
}

TEST(TestProyeccionPreferencias, textoDeMotivo_esEspanolYdistingueLosTresMotivos)
{
    EXPECT_TRUE(textoDeMotivo(MotivoPreferencia::Ninguno).isEmpty());

    const QString noParticipa = textoDeMotivo(MotivoPreferencia::DocenteNoParticipa);
    const QString dia = textoDeMotivo(MotivoPreferencia::DiaFueraDeSemana);
    const QString franja = textoDeMotivo(MotivoPreferencia::SinFranjaDelTurno);

    EXPECT_FALSE(noParticipa.isEmpty());
    EXPECT_FALSE(dia.isEmpty());
    EXPECT_FALSE(franja.isEmpty());
    EXPECT_NE(noParticipa, dia);
    EXPECT_NE(noParticipa, franja);
    EXPECT_NE(dia, franja);

    // RNF-1: el texto del motivo explica la causa.
    EXPECT_TRUE(noParticipa.contains(QStringLiteral("no participa")));
    EXPECT_TRUE(dia.contains(QStringLiteral("semana")));
    EXPECT_TRUE(franja.contains(QStringLiteral("franja")));
}
