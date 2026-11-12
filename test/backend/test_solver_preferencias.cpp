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

/// Un curso de 1|A con una materia de 2 h en 2 días × 4 franjas. La fase 1 reparte las clases
/// con S1+S2; hay sitio de sobra para moverlas.
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
    config.dimensiones = {2, 1, 2, 2, 2, 4};
    config.franja_horaria = {60, 4};
    config.turnos.turnos = {{QStringLiteral("manana"), {0, 1, 2, 3}}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("Aula de estudio")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30},
                    {QStringLiteral("A2"), QStringLiteral("Aula de estudio"), 30}};
    config.planes = {{QStringLiteral("Plan A"), {{0, 1, 2}}},
                     {QStringLiteral("Plan B"), {{0, 2, 2}}}};
    config.cursos = {{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                      QStringLiteral("Plan A"), 0},
                     {2, QStringLiteral("B"), QStringLiteral("manana"), 20,
                      QStringLiteral("Plan B"), 1}};
    config.profesores = {
        {QStringLiteral("Ana"), 2, 2, QStringLiteral("manana"), {}, {}, {0}, {},
         dispCompleta(2, 4)},
        {QStringLiteral("Beto"), 2, 2, QStringLiteral("manana"), {}, {}, {0}, {},
         dispCompleta(2, 4)}};
    return config;
}

/// Un curso con una materia de 4 h, un titular con capacidad justa (4 h de aula) y un suplente
/// elegible: la referencia asigna al titular sin exceso y la preferencia no puede cambiarlo.
SolverConfig configSuplente()
{
    SolverConfig config;
    config.version = "2.0";
    config.aula_fija = true;
    config.dimensiones = {2, 1, 1, 1, 5, 2};
    config.franja_horaria = {60, 2};
    config.turnos.turnos = {{QStringLiteral("manana"), {0, 1}}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("Aula de estudio")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30}};
    config.planes = {{QStringLiteral("Ciencias"), {{0, 1, 4}}}};
    config.cursos = {{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                      QStringLiteral("Ciencias"), 0}};
    config.profesores = {
        // Titular con capacidad justa (4 bloques).
        {QStringLiteral("Titular"), 4, 4, QStringLiteral("manana"), QStringLiteral("Ciencias"),
         {1}, {0}, {}, dispCompleta(5, 2)},
        // Suplente elegible para la misma materia.
        {QStringLiteral("Suplente"), 4, 4, QStringLiteral("manana"), QStringLiteral("Ciencias"),
         {1}, {}, {{0, 1}}, dispCompleta(5, 2)}};
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

/// Presupuesto de 1 worker para pruebas que comparan horarios exactos (014b). El producto usa 8.
Solver::PresupuestoFases unSoloWorker()
{
    Solver::PresupuestoFases p;
    p.workers = 1;
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

TEST(TestSolverPreferencias, sinPreferencias_daUnParAdyacenteValido)
{
    Solver solver;
    const auto resultado = solver.resolver(configDosDias());

    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();
    // 014b: con 8 workers no se fija el horario exacto; sí que sea un par adyacente del día.
    const QSet<QPair<int, int>> franjas =
        clasesDeMateria(resultado.resultado, QStringLiteral("1|A"), 0);
    ASSERT_EQ(franjas.size(), 2);
    auto it = franjas.constBegin();
    const QPair<int, int> a = *it;
    ++it;
    const QPair<int, int> b = *it;
    EXPECT_EQ(a.first, b.first) << "las dos clases deben caer el mismo día";
    EXPECT_EQ(qAbs(a.second - b.second), 1) << "deben ser franjas adyacentes";
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
    const auto referencia = solver.resolver(configDosDias(), unSoloWorker());
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
    const auto referencia = solver.resolver(configDosDias(), unSoloWorker());
    ASSERT_TRUE(referencia.exito) << referencia.errores.join("\n").toStdString();
    const QSet<QPair<int, int>> franjasReferencia =
        clasesDeMateria(referencia.resultado, QStringLiteral("1|A"), 0);

    // Se prefiere el día que la referencia deja libre, en dos franjas al inicio (el hueco queda
    // al final y el valor de la 2a no cambia).
    const bool referenciaEnDia0 = std::any_of(franjasReferencia.begin(), franjasReferencia.end(),
                                              [](const QPair<int, int>& f) { return f.first == 0; });
    const int diaPreferido = referenciaEnDia0 ? 1 : 0;

    SolverConfig config = configDosDias();
    config.preferencias = {preferencia(0, diaPreferido, {0, 1}, QStringLiteral("preferida"))};

    const auto conPreferencia = solver.resolver(config, unSoloWorker());
    ASSERT_TRUE(conPreferencia.exito) << conPreferencia.errores.join("\n").toStdString();

    const QSet<QPair<int, int>> franjas =
        clasesDeMateria(conPreferencia.resultado, QStringLiteral("1|A"), 0);
    EXPECT_EQ(franjas, (QSet<QPair<int, int>>{qMakePair(diaPreferido, 0),
                                             qMakePair(diaPreferido, 1)}));
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
    const auto conDos = solver.resolver(dos, unSoloWorker());
    const auto conUna = solver.resolver(una, unSoloWorker());

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
    // 4 h de la materia en franjas evitadas: el docente no cambia (S2 intacto).
    SolverConfig config = configSuplente();
    for (int dia = 0; dia < 5; ++dia)
        config.preferencias.append(preferencia(0, dia, {0}, QStringLiteral("evitada")));

    Solver solver;
    const auto resultado = solver.resolver(config);

    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();
    EXPECT_EQ(contarClases(resultado.resultado), 4);
    for (const DiaOutput& dia : resultado.resultado.horarios.value(QStringLiteral("1|A")).dias)
        for (const AsignacionOutput& asignacion : dia.asignaciones)
            EXPECT_EQ(asignacion.profesor, 0) << "la preferencia no puede pasar al suplente";
}

TEST(TestSolverPreferencias, evitadaAcumulada_noProvocaExceso)
{
    // La misma acumulación no crea exceso de horas (S1 intacto): el titular tiene 4 h de aula.
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
    QSet<QString> docentePorFranja;          // docente|día|slot
    QSet<QString> aulaPorFranja;             // aula|día|slot
    QMap<QString, int> bloquesPorMateria;

    for (auto it = resultado.resultado.horarios.constBegin();
         it != resultado.resultado.horarios.constEnd(); ++it) {
        for (const DiaOutput& dia : it.value().dias) {
            for (const AsignacionOutput& asignacion : dia.asignaciones) {
                // Turno del curso: el único declarado es {0, 1, 2, 3}.
                EXPECT_GE(asignacion.slot, 0);
                EXPECT_LE(asignacion.slot, 3);

                const QString franjaDocente = QStringLiteral("%1|%2|%3")
                                                  .arg(asignacion.profesor)
                                                  .arg(dia.dia)
                                                  .arg(asignacion.slot);
                const QString franjaAula = QStringLiteral("%1|%2|%3")
                                               .arg(asignacion.aula)
                                               .arg(dia.dia)
                                               .arg(asignacion.slot);
                const QString cursoFranja =
                    QStringLiteral("%1|%2|%3").arg(it.key()).arg(dia.dia).arg(asignacion.slot);
                EXPECT_FALSE(cursoPorFranja.contains(cursoFranja))
                    << "un curso no puede tener dos clases en la misma franja";
                EXPECT_FALSE(docentePorFranja.contains(franjaDocente))
                    << "un docente no puede dar dos clases en la misma franja";
                EXPECT_FALSE(aulaPorFranja.contains(franjaAula))
                    << "un aula no puede alojar dos clases en la misma franja";
                cursoPorFranja.insert(cursoFranja);
                docentePorFranja.insert(franjaDocente);
                aulaPorFranja.insert(franjaAula);

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
    const auto a = solver.resolver(unOrden, unSoloWorker());
    const auto b = solver.resolver(otroOrden, unSoloWorker());

    ASSERT_TRUE(a.exito) << a.errores.join("\n").toStdString();
    ASSERT_TRUE(b.exito) << b.errores.join("\n").toStdString();
    EXPECT_EQ(resumenDeSalida(a), resumenDeSalida(b));
}

// ─── Preferencia como fase 2 ────────────────────────────────────────────────

TEST(TestSolverPreferencias, preferenciaEligeLasFranjasPreferidas)
{
    // Materia de 2 h en un día de 4 franjas. Con el emparejamiento duro las dos clases deben
    // formar un par adyacente; la preferencia premia el par (0,1) frente a los demás.
    SolverConfig config = configDosDias();
    config.dimensiones = {1, 1, 1, 1, 1, 4};
    config.franja_horaria = {60, 4};
    config.turnos.turnos = {{QStringLiteral("manana"), {0, 1, 2, 3}}};
    config.profesores[0].disponibilidad = dispCompleta(1, 4);
    config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("preferida"))};

    Solver solver;
    const auto resultado = solver.resolver(config, unSoloWorker());
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    // La preferencia manda dentro de la fase: las dos clases acaban en el par preferido.
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
    const auto referencia = solver.resolver(configDosDias(), unSoloWorker());
    ASSERT_TRUE(referencia.exito) << referencia.errores.join("\n").toStdString();

    SolverConfig config = configDosDias();
    config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("preferida"))};

    Solver::PresupuestoFases presupuesto;
    presupuesto.referencia = 6.0;
    presupuesto.preferencias = 1e-9;   // agota la fase 2 antes de encontrar solución
    presupuesto.workers = 1;           // comparación exacta de horario (test)
    const auto conFase2Agotada = solver.resolver(config, presupuesto);

    ASSERT_TRUE(conFase2Agotada.exito) << conFase2Agotada.errores.join("\n").toStdString();
    EXPECT_EQ(resumenDeSalida(conFase2Agotada), resumenDeSalida(referencia));
}
