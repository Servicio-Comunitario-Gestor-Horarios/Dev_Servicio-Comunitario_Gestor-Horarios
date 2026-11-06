#include <gtest/gtest.h>

#include <QMap>
#include <QPair>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <algorithm>

#include <backend/data/horario_salida.hpp>
#include <backend/solver/config/contrato_solver.hpp>
#include <backend/solver/config/solver_config.hpp>
#include <backend/solver/solver.hpp>

/// Tests del objetivo con preferencias del docente (spec 013, RF-1, RF-4, RF-5).
///
/// Laserarquía es ESTRUCTURAL: la fase 1 es el modelo de hoy (referencia) y la fase 2 fija por
/// igualdad la asignación de docente por materia-curso (z*) y el exceso por docente (e*), de modo
/// que la preferencia solo decide DÓNDE caen las clases. El exceso se calcula en el test desde las
/// clases y `horas_aula` (P14): no se amplía la API del motor.
namespace {

QVector<Disponibilidad> dispCompleta(int dias, int franjas)
{
    QVector<Disponibilidad> disp;
    for (int d = 0; d < dias; ++d) {
        QVector<int> franjasDelDia;
        for (int s = 0; s < franjas; ++s)
            franjasDelDia.append(s);
        disp.append(Disponibilidad{d, franjasDelDia});
    }
    return disp;
}

/// Un curso de 1|A con una materia de 2 h en 2 días × 4 franjas. La fase 1 coloca las dos
/// clases en un grupo contiguo de 2 (S3); hay sitio de sobra para moverlas.
SolverConfig configDosDias()
{
    SolverConfig config;
    config.version = "2.0";
    config.aula_fija = true;
    config.dimensiones = {1, 1, 1, 1, 2, 4};
    config.franja_horaria = {60, 4};
    config.turnos.turnos = {{QStringLiteral("manana"), {0, 1, 2, 3}}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("Aula de estudio")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30}};
    config.planes = {{QStringLiteral("Ciencias"), {{0, 1, 2}}}};
    config.cursos = {{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                      QStringLiteral("Ciencias"), 0}};
    config.profesores = {{QStringLiteral("Ana"), 2, 2, QStringLiteral("manana"),
                          QStringLiteral("Ciencias"), {1}, {0}, {}, dispCompleta(2, 4)}};
    return config;
}

/// Dos cursos de 1|A y 2|B, una materia de 2 h cada uno, con dos docentes candidatos y un solo
/// aula: sirve para comprobar que las invariantes duras siguen intactas con preferencias.
SolverConfig configInvariantes()
{
    SolverConfig config;
    config.version = "2.0";
    config.aula_fija = true;
    config.dimensiones = {2, 1, 1, 2, 2, 4};
    config.franja_horaria = {60, 4};
    config.turnos.turnos = {{QStringLiteral("manana"), {0, 1, 2, 3}}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("Aula de estudio")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30}};
    config.planes = {{QStringLiteral("Plan A"), {{0, 1, 2}}},
                     {QStringLiteral("Plan B"), {{0, 2, 2}}}};
    config.cursos = {{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                      QStringLiteral("Plan A"), 0},
                     {2, QStringLiteral("B"), QStringLiteral("manana"), 20,
                      QStringLiteral("Plan B"), 0}};
    config.profesores = {
        {QStringLiteral("Ana"), 2, 2, QStringLiteral("manana"), {}, {}, {0}, {},
         dispCompleta(2, 4)},
        {QStringLiteral("Beto"), 2, 2, QStringLiteral("manana"), {}, {}, {0}, {},
         dispCompleta(2, 4)}};
    return config;
}

/// Un curso con una materia de 5 h, un titular con capacidad justa (5 h de aula) y un suplente
/// elegible: la referencia asigna al titular sin exceso y la preferencia no puede cambiarlo.
SolverConfig configSuplente()
{
    SolverConfig config;
    config.version = "2.0";
    config.aula_fija = true;
    config.dimensiones = {2, 1, 1, 1, 5, 1};
    config.franja_horaria = {60, 1};
    config.turnos.turnos = {{QStringLiteral("manana"), {0}}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("Aula de estudio")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30}};
    config.planes = {{QStringLiteral("Ciencias"), {{0, 1, 5}}}};
    config.cursos = {{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                      QStringLiteral("Ciencias"), 0}};
    config.profesores = {
        // Titular con capacidad justa (5 bloques).
        {QStringLiteral("Titular"), 5, 5, QStringLiteral("manana"), QStringLiteral("Ciencias"),
         {1}, {0}, {}, dispCompleta(5, 1)},
        // Suplente elegible para la misma materia.
        {QStringLiteral("Suplente"), 5, 5, QStringLiteral("manana"), QStringLiteral("Ciencias"),
         {1}, {}, {{0, 1}}, dispCompleta(5, 1)}};
    return config;
}

/// Preferencia ya proyectada para la configuración del solver (día de la generación).
PreferenciaSolverConfig preferencia(int docente, int dia, const QVector<int>& franjas,
                                     const QString& sentido)
{
    PreferenciaSolverConfig p;
    p.docente = docente;
    p.dia = dia;
    p.slot = franjas;
    p.sentido = sentido;
    return p;
}

/// Resumen determinista de una salida: curso|día|slot|materia|profesor|aula, ordenado.
QString resumenDeSalida(const Solver::ResultadoSolver& resultado)
{
    QStringList clases;
    for (auto it = resultado.resultado.horarios.constBegin();
         it != resultado.resultado.horarios.constEnd(); ++it) {
        for (const DiaOutput& dia : it.value().dias) {
            for (const AsignacionOutput& asignacion : dia.asignaciones) {
                clases.append(QStringLiteral("%1|%2|%3|%4|%5|%6")
                                  .arg(it.key())
                                  .arg(dia.dia)
                                  .arg(asignacion.slot)
                                  .arg(asignacion.materia)
                                  .arg(asignacion.profesor)
                                  .arg(asignacion.aula));
            }
        }
    }
    clases.sort();
    return clases.join(QLatin1Char('\n'));
}

/// Franjas (día, slot) de una materia de un curso.
QSet<QPair<int, int>> clasesDeMateria(const HorarioSalida& horario, const QString& curso,
                                      int materia)
{
    QSet<QPair<int, int>> franjas;
    for (const DiaOutput& dia : horario.horarios.value(curso).dias)
        for (const AsignacionOutput& asignacion : dia.asignaciones)
            if (asignacion.materia == materia)
                franjas.insert(qMakePair(dia.dia, asignacion.slot));
    return franjas;
}

int contarClases(const HorarioSalida& horario)
{
    int total = 0;
    for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd(); ++it)
        for (const DiaOutput& dia : it.value().dias)
            total += dia.asignaciones.size();
    return total;
}

/// Nº de clases AISLADAS (la penalización que aplica hoy S3): una clase de una materia en un día
/// sin otra clase de la misma materia en la franja anterior ni en la siguiente. Los fixtures no
/// declaran eventos, así que el día es un solo segmento.
int clasesAisladas(const HorarioSalida& horario)
{
    int aisladas = 0;
    for (auto itCurso = horario.horarios.constBegin(); itCurso != horario.horarios.constEnd();
         ++itCurso) {
        // (materia, día) → franjas ocupadas.
        QMap<QPair<int, int>, QSet<int>> franjas;
        for (const DiaOutput& dia : itCurso.value().dias)
            for (const AsignacionOutput& asignacion : dia.asignaciones)
                franjas[qMakePair(asignacion.materia, dia.dia)].insert(asignacion.slot);

        for (auto it = franjas.constBegin(); it != franjas.constEnd(); ++it) {
            for (int slot : it.value()) {
                const bool tieneVecina = it.value().contains(slot - 1) ||
                                         it.value().contains(slot + 1);
                if (!tieneVecina)
                    ++aisladas;
            }
        }
    }
    return aisladas;
}

/// Asignación observada de docente por materia-curso: `curso|materia` → conjunto de docentes
/// que dan clase de esa materia (debe ser uno solo; se guarda el conjunto para detectar cambios).
QMap<QString, QSet<int>> asignacionDocente(const HorarioSalida& horario)
{
    QMap<QString, QSet<int>> asignacion;
    for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd(); ++it)
        for (const DiaOutput& dia : it.value().dias)
            for (const AsignacionOutput& asignacionClase : dia.asignaciones)
                asignacion[QStringLiteral("%1|%2").arg(it.key()).arg(asignacionClase.materia)]
                    .insert(asignacionClase.profesor);
    return asignacion;
}

/// Exceso de horas de aula por docente, calculado desde las clases y `horas_aula` (P14).
QMap<int, int> excesoPorDocente(const HorarioSalida& horario, const SolverConfig& config)
{
    QMap<int, int> clasesPorDocente;
    for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd(); ++it)
        for (const DiaOutput& dia : it.value().dias)
            for (const AsignacionOutput& asignacion : dia.asignaciones)
                if (asignacion.profesor >= 0)
                    ++clasesPorDocente[asignacion.profesor];

    QMap<int, int> exceso;
    for (int p = 0; p < config.profesores.size(); ++p)
        exceso[p] = qMax(0, clasesPorDocente.value(p, 0) - config.profesores[p].horas_aula);
    return exceso;
}

} // namespace

// ─── Sin preferencias: la referencia no cambia (RNF-4) ───────────────────────

TEST(TestSolverPreferencias, sinPreferencias_salidaIdenticaALaActual)
{
    Solver solver;
    const auto resultado = solver.resolver(configDosDias());

    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();
    EXPECT_EQ(resumenDeSalida(resultado),
              QStringLiteral("1|A|1|2|0|0|0\n1|A|1|3|0|0|0"));
}

// ─── Garantía estructural (RF-1) ────────────────────────────────────────────

TEST(TestSolverPreferencias, mismasPreferencias_mismaAsignacionDocenteYExceso)
{
    SolverConfig conPreferencias = configInvariantes();

    Solver solver;
    const auto sinPreferencias = solver.resolver(configInvariantes());
    ASSERT_TRUE(sinPreferencias.exito) << sinPreferencias.errores.join("\n").toStdString();

    // Se prefiere el día en el que la referencia NO tiene todas las clases: el horario debe
    // moverse (control) sin tocar el reparto de docentes ni el exceso.
    bool todasEnDia1 = true;
    for (auto it = sinPreferencias.resultado.horarios.constBegin();
         it != sinPreferencias.resultado.horarios.constEnd(); ++it)
        for (const DiaOutput& dia : it.value().dias)
            if (!dia.asignaciones.isEmpty() && dia.dia != 1)
                todasEnDia1 = false;
    const int diaPreferido = todasEnDia1 ? 0 : 1;
    for (int docente = 0; docente < conPreferencias.profesores.size(); ++docente)
        conPreferencias.preferencias.append(
            preferencia(docente, diaPreferido, {0, 1, 2, 3}, QStringLiteral("preferida")));

    const auto con = solver.resolver(conPreferencias);

    ASSERT_TRUE(sinPreferencias.exito) << sinPreferencias.errores.join("\n").toStdString();
    ASSERT_TRUE(con.exito) << con.errores.join("\n").toStdString();

    EXPECT_EQ(asignacionDocente(con.resultado),
              asignacionDocente(sinPreferencias.resultado));
    EXPECT_EQ(excesoPorDocente(con.resultado, conPreferencias),
              excesoPorDocente(sinPreferencias.resultado, configInvariantes()));

    // Control: las preferencias sí cambian DÓNDE caen las clases.
    EXPECT_NE(resumenDeSalida(con), resumenDeSalida(sinPreferencias));
}

// ─── Efecto del sentido (RF-4) ──────────────────────────────────────────────

TEST(TestSolverPreferencias, evitada_mueveLaClaseFueraDeLaFranja)
{
    Solver solver;
    const auto referencia = solver.resolver(configDosDias());
    ASSERT_TRUE(referencia.exito) << referencia.errores.join("\n").toStdString();
    const QSet<QPair<int, int>> franjasReferencia =
        clasesDeMateria(referencia.resultado, QStringLiteral("1|A"), 0);
    ASSERT_EQ(franjasReferencia.size(), 2);

    // Se evitan exactamente las dos franjas que ocupa la referencia (día y slot).
    SolverConfig config = configDosDias();
    for (const QPair<int, int>& franja : franjasReferencia)
        config.preferencias.append(
            preferencia(0, franja.first, {franja.second}, QStringLiteral("evitada")));

    const auto conPreferencia = solver.resolver(config);
    ASSERT_TRUE(conPreferencia.exito) << conPreferencia.errores.join("\n").toStdString();

    const QSet<QPair<int, int>> franjas =
        clasesDeMateria(conPreferencia.resultado, QStringLiteral("1|A"), 0);
    EXPECT_EQ(franjas.size(), 2);
    for (const QPair<int, int>& franja : franjas)
        EXPECT_FALSE(franjasReferencia.contains(franja)) << "la clase sigue en una franja evitada";
}

TEST(TestSolverPreferencias, preferida_atraeLaClase)
{
    Solver solver;
    const auto referencia = solver.resolver(configDosDias());
    ASSERT_TRUE(referencia.exito) << referencia.errores.join("\n").toStdString();
    const QSet<QPair<int, int>> franjasReferencia =
        clasesDeMateria(referencia.resultado, QStringLiteral("1|A"), 0);

    // Se prefiere el día que la referencia deja libre, en dos franjas contiguas.
    const bool referenciaEnDia0 = std::any_of(franjasReferencia.begin(), franjasReferencia.end(),
                                              [](const QPair<int, int>& f) { return f.first == 0; });
    const int diaPreferido = referenciaEnDia0 ? 1 : 0;

    SolverConfig config = configDosDias();
    config.preferencias = {preferencia(0, diaPreferido, {2, 3}, QStringLiteral("preferida"))};

    const auto conPreferencia = solver.resolver(config);
    ASSERT_TRUE(conPreferencia.exito) << conPreferencia.errores.join("\n").toStdString();

    const QSet<QPair<int, int>> franjas =
        clasesDeMateria(conPreferencia.resultado, QStringLiteral("1|A"), 0);
    EXPECT_EQ(franjas, (QSet<QPair<int, int>>{qMakePair(diaPreferido, 2),
                                             qMakePair(diaPreferido, 3)}));
}

TEST(TestSolverPreferencias, dosPreferenciasSolapadasMismoSentido_cuentanUnaVez)
{
    // Dos evitadas que se solapan en la franja 1 sobre {0,1} y {1,2} equivalen a una sola
    // evitada sobre la unión {0,1,2}: la deduplicación por (docente, día, slot) hace que los dos
    // modelos sean idénticos y, con el mismo seed, la misma salida.
    SolverConfig dos = configDosDias();
    dos.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("evitada")),
                        preferencia(0, 0, {1, 2}, QStringLiteral("evitada"))};

    SolverConfig una = configDosDias();
    una.preferencias = {preferencia(0, 0, {0, 1, 2}, QStringLiteral("evitada"))};

    Solver solver;
    const auto conDos = solver.resolver(dos);
    const auto conUna = solver.resolver(una);

    ASSERT_TRUE(conDos.exito) << conDos.errores.join("\n").toStdString();
    ASSERT_TRUE(conUna.exito) << conUna.errores.join("\n").toStdString();
    EXPECT_EQ(resumenDeSalida(conDos), resumenDeSalida(conUna));
}

TEST(TestSolverPreferencias, preferenciaCedeSiNoHayAlternativa)
{
    // Todas las franjas del curso están evitadas: el horario es factible igualmente y las clases
    // caen dentro. La preferencia nunca declara infactibilidad (RF-4).
    SolverConfig config = configDosDias();
    config.dimensiones = {1, 1, 1, 1, 1, 2};
    config.franja_horaria = {60, 2};
    config.turnos.turnos = {{QStringLiteral("manana"), {0, 1}}};
    config.profesores[0].disponibilidad = dispCompleta(1, 2);
    config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("evitada"))};

    Solver solver;
    const auto resultado = solver.resolver(config);

    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();
    EXPECT_EQ(contarClases(resultado.resultado), 2);
}

// ─── Preferencia vs estructura (RF-1, RF-5) ─────────────────────────────────

TEST(TestSolverPreferencias, evitadaAcumulada_noProvocaSuplente)
{
    // 5 h de la materia en franjas evitadas: el docente no cambia (S2 intacto).
    SolverConfig config = configSuplente();
    for (int dia = 0; dia < 5; ++dia)
        config.preferencias.append(preferencia(0, dia, {0}, QStringLiteral("evitada")));

    Solver solver;
    const auto resultado = solver.resolver(config);

    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();
    EXPECT_EQ(contarClases(resultado.resultado), 5);
    for (const DiaOutput& dia : resultado.resultado.horarios.value(QStringLiteral("1|A")).dias)
        for (const AsignacionOutput& asignacion : dia.asignaciones)
            EXPECT_EQ(asignacion.profesor, 0) << "la preferencia no puede pasar al suplente";
}

TEST(TestSolverPreferencias, evitadaAcumulada_noProvocaExceso)
{
    // La misma acumulación no crea exceso de horas (S1 intacto): el titular tiene 5 h de aula.
    SolverConfig config = configSuplente();
    for (int dia = 0; dia < 5; ++dia)
        config.preferencias.append(preferencia(0, dia, {0}, QStringLiteral("evitada")));

    Solver solver;
    const auto resultado = solver.resolver(config);

    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();
    EXPECT_EQ(excesoPorDocente(resultado.resultado, config).value(0), 0);
}

// ─── Invariantes duras y determinismo ───────────────────────────────────────

TEST(TestSolverPreferencias, preferenciaNoRompeInvariantesDuras)
{
    SolverConfig config = configInvariantes();
    for (int docente = 0; docente < config.profesores.size(); ++docente) {
        config.preferencias.append(
            preferencia(docente, 0, {0, 1, 2, 3}, QStringLiteral("evitada")));
        config.preferencias.append(
            preferencia(docente, 1, {0, 1, 2, 3}, QStringLiteral("preferida")));
    }

    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    QSet<QString> cursoPorFranja;            // exclusividad de curso (curso|día|slot)
    QSet<QPair<int, int>> docentePorFranja;  // exclusividad de docente
    QSet<QPair<int, int>> aulaPorFranja;     // exclusividad de aula
    QMap<QString, int> bloquesPorMateria;

    for (auto it = resultado.resultado.horarios.constBegin();
         it != resultado.resultado.horarios.constEnd(); ++it) {
        for (const DiaOutput& dia : it.value().dias) {
            for (const AsignacionOutput& asignacion : dia.asignaciones) {
                // Turno del curso: el único declarado es {0, 1, 2, 3}.
                EXPECT_GE(asignacion.slot, 0);
                EXPECT_LE(asignacion.slot, 3);

                const QPair<int, int> franja = qMakePair(dia.dia, asignacion.slot);
                const QString cursoFranja =
                    QStringLiteral("%1|%2|%3").arg(it.key()).arg(dia.dia).arg(asignacion.slot);
                EXPECT_FALSE(cursoPorFranja.contains(cursoFranja))
                    << "un curso no puede tener dos clases en la misma franja";
                EXPECT_FALSE(docentePorFranja.contains(franja))
                    << "un docente no puede dar dos clases en la misma franja";
                EXPECT_FALSE(aulaPorFranja.contains(franja))
                    << "un aula no puede alojar dos clases en la misma franja";
                cursoPorFranja.insert(cursoFranja);
                docentePorFranja.insert(franja);
                aulaPorFranja.insert(franja);

                // El docente asignado opta por la materia (titular o suplente declarado).
                const int materia = asignacion.materia;
                const bool opta =
                    config.profesores[asignacion.profesor].materias_asignadas.contains(materia) ||
                    std::any_of(config.profesores[asignacion.profesor].materias_suplente
                                    .constBegin(),
                                config.profesores[asignacion.profesor].materias_suplente
                                    .constEnd(),
                                [materia](const MateriaSuplente& s) {
                                    return s.materiaIDx == materia;
                                });
                EXPECT_TRUE(opta);

                bloquesPorMateria[it.key()] += 1;
            }
        }
    }

    // D3: cada materia-curso cubre exactamente sus horas (2 + 2).
    EXPECT_EQ(bloquesPorMateria.value(QStringLiteral("1|A")), 2);
    EXPECT_EQ(bloquesPorMateria.value(QStringLiteral("2|B")), 2);
}

TEST(TestSolverPreferencias, ordenDeDeclaracionNoInfluye)
{
    SolverConfig unOrden = configDosDias();
    unOrden.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("evitada")),
                            preferencia(0, 1, {2, 3}, QStringLiteral("preferida"))};

    SolverConfig otroOrden = configDosDias();
    otroOrden.preferencias = {preferencia(0, 1, {2, 3}, QStringLiteral("preferida")),
                              preferencia(0, 0, {0, 1}, QStringLiteral("evitada"))};

    Solver solver;
    const auto a = solver.resolver(unOrden);
    const auto b = solver.resolver(otroOrden);

    ASSERT_TRUE(a.exito) << a.errores.join("\n").toStdString();
    ASSERT_TRUE(b.exito) << b.errores.join("\n").toStdString();
    EXPECT_EQ(resumenDeSalida(a), resumenDeSalida(b));
}

// ─── Jerarquía preferencia > S3 (fase 2b) ───────────────────────────────────

TEST(TestSolverPreferencias, preferenciaGanaALaAgrupacion)
{
    // Materia de 2 h en un día de 4 franjas. La preferencia exige las franjas 0 y 3, que dejan
    // las dos clases AISLADAS (S3 las penaliza); la referencia las agrupa en una sola tirada.
    SolverConfig config = configDosDias();
    config.dimensiones = {1, 1, 1, 1, 1, 4};
    config.franja_horaria = {60, 4};
    config.turnos.turnos = {{QStringLiteral("manana"), {0, 1, 2, 3}}};
    config.profesores[0].disponibilidad = dispCompleta(1, 4);
    config.preferencias = {preferencia(0, 0, {0, 3}, QStringLiteral("preferida"))};

    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    // La preferencia manda sobre S3: las dos clases acaban en las franjas preferidas aunque el
    // horario quede peor agrupado (ninguna otra asignación vale −50 en preferencia).
    EXPECT_EQ(clasesDeMateria(resultado.resultado, QStringLiteral("1|A"), 0),
              (QSet<QPair<int, int>>{qMakePair(0, 0), qMakePair(0, 3)}));
}

TEST(TestSolverPreferencias, fase2b_mejoraLaAgrupacionSinCambiarLaPreferencia)
{
    // La preferencia deja varias soluciones con el mismo valor. La 2b fija ese valor y
    // desempata por S3: nunca pierde cumplimiento y no empeora la agrupación de la 2a.
    SolverConfig config = configDosDias();
    config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("evitada"))};

    Solver solver;
    const auto conFase2b = solver.resolver(config);

    Solver::PresupuestoFases sinFase2b;
    sinFase2b.agrupacion = 1e-9;   // agota la 2b: se queda la solución de la 2a
    const auto soloFase2a = solver.resolver(config, sinFase2b);

    ASSERT_TRUE(conFase2b.exito) << conFase2b.errores.join("\n").toStdString();
    ASSERT_TRUE(soloFase2a.exito) << soloFase2a.errores.join("\n").toStdString();

    // Mismo cumplimiento: ninguna clase en las franjas evitadas en ninguno de los dos.
    const QSet<QPair<int, int>> evitadas{qMakePair(0, 0), qMakePair(0, 1)};
    const auto clasesCon2b = clasesDeMateria(conFase2b.resultado, QStringLiteral("1|A"), 0);
    const auto clasesSolo2a = clasesDeMateria(soloFase2a.resultado, QStringLiteral("1|A"), 0);
    for (const QPair<int, int>& franja : clasesCon2b)
        EXPECT_FALSE(evitadas.contains(franja));
    for (const QPair<int, int>& franja : clasesSolo2a)
        EXPECT_FALSE(evitadas.contains(franja));

    // 2b no puede empeorar la agrupación (S3 = nº de clases aisladas en su materia y día).
    EXPECT_LE(clasesAisladas(conFase2b.resultado), clasesAisladas(soloFase2a.resultado));
}

TEST(TestSolverPreferencias, fase2bAgotada_devuelveLaFase2a)
{
    // Con la 2b fuera de presupuesto se devuelve la 2a (preferencia cumplida), no la referencia
    // ni un error: la escalera 2b→2a conserva el cumplimiento.
    SolverConfig config = configDosDias();
    config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("preferida"))};

    Solver::PresupuestoFases presupuesto;
    presupuesto.agrupacion = 1e-9;
    Solver solver;
    const auto resultado = solver.resolver(config, presupuesto);

    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();
    EXPECT_EQ(clasesDeMateria(resultado.resultado, QStringLiteral("1|A"), 0),
              (QSet<QPair<int, int>>{qMakePair(0, 0), qMakePair(0, 1)}));
}

// ─── Fallback a la referencia (P6/P7) ───────────────────────────────────────

TEST(TestSolverPreferencias, fase2SinSolucion_devuelveLaReferencia)
{
    // Con el anclaje por igualdad la fase 2 es factible por construcción (la solución de la
    // referencia la satisface); el caso alcanzable es que el presupuesto de la fase 2 se agote
    // (UNKNOWN). Entonces se devuelve la referencia, no un error.
    Solver solver;
    const auto referencia = solver.resolver(configDosDias());
    ASSERT_TRUE(referencia.exito) << referencia.errores.join("\n").toStdString();

    SolverConfig config = configDosDias();
    config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("preferida"))};

    Solver::PresupuestoFases presupuesto;
    presupuesto.referencia = 6.0;
    presupuesto.preferencias = 1e-9;   // agota la fase 2 antes de encontrar solución
    const auto conFase2Agotada = solver.resolver(config, presupuesto);

    ASSERT_TRUE(conFase2Agotada.exito) << conFase2Agotada.errores.join("\n").toStdString();
    EXPECT_EQ(resumenDeSalida(conFase2Agotada), resumenDeSalida(referencia));
}
