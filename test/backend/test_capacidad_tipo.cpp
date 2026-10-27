#include <gtest/gtest.h>

#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Tests de las funciones puras de demanda y capacidad por tipo de aula (RF-6, T11).
///
/// Fijan la unidad de la cuenta: una franja es un par (día, slot) —`FranjaSemana`— y no un slot
/// repetido por día. La capacidad de un tipo es `aulas × |unión de las franjas de los turnos de los
/// cursos que lo requieren|`; la demanda, la suma de bloques de esas materias.
namespace {

/// Configuración mínima de 2 días con dos turnos que comparten la franja 5:
/// TB = {4, 5} y TC = {5, 6}. Dos aulas del tipo general (una con otra caja) y una de laboratorio.
SolverConfig configBase() {
    SolverConfig config;
    config.aula_fija = false;
    config.dimensiones.num_dias = 2;
    config.dimensiones.num_slots_dia = 7;
    config.franja_horaria.duracion_minutos = 60;

    TurnoConfig tb;
    tb.nombre = QStringLiteral("TB");
    tb.slot = {4, 5};
    TurnoConfig tc;
    tc.nombre = QStringLiteral("TC");
    tc.slot = {5, 6};
    config.turnos.turnos = {tb, tc};

    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("Aula de estudio")},
                       {QStringLiteral("Química"), QStringLiteral("Laboratorio")}};

    config.aulas = {{QStringLiteral("A1"), QStringLiteral("Aula de estudio"), 30},
                    {QStringLiteral("A2"), QStringLiteral("AULA DE ESTUDIO"), 30},
                    {QStringLiteral("L1"), QStringLiteral("Laboratorio"), 20}};

    // Curso 1A (turno TB): 2 bloques del tipo general y 1 de laboratorio.
    PlanSolverConfig planA;
    planA.codigo = QStringLiteral("A");
    planA.materias = {{0, 1, 2}, {1, 1, 1}};

    // Curso 2B (turno TC): 1 bloque del tipo general.
    PlanSolverConfig planB;
    planB.codigo = QStringLiteral("B");
    planB.materias = {{0, 2, 1}};

    config.planes = {planA, planB};
    config.cursos = {{1, QStringLiteral("A"), QStringLiteral("TB"), -1, QStringLiteral("A"), -1},
                     {2, QStringLiteral("B"), QStringLiteral("TC"), -1, QStringLiteral("B"), -1}};
    return config;
}

QString claveGeneral() { return claveTipo(QStringLiteral("Aula de estudio")); }
QString claveLaboratorio() { return claveTipo(QStringLiteral("Laboratorio")); }

}  // namespace

TEST(TestCapacidadTipo, franjaSemana_ordenIgualdadYqHash) {
    const FranjaSemana lunesCuarta{0, 4};
    const FranjaSemana lunesCuartaOtra{0, 4};
    const FranjaSemana lunesQuinta{0, 5};
    const FranjaSemana martesPrimera{1, 0};

    EXPECT_TRUE(lunesCuarta == lunesCuartaOtra);
    EXPECT_FALSE(lunesCuarta == lunesQuinta);
    EXPECT_TRUE(lunesCuarta < lunesQuinta);       // mismo día: ordena por franja
    EXPECT_TRUE(lunesQuinta < martesPrimera);     // primero el día, después la franja
    EXPECT_FALSE(martesPrimera < lunesCuarta);
    EXPECT_EQ(qHash(lunesCuarta), qHash(lunesCuartaOtra));

    QSet<FranjaSemana> conjunto{lunesCuarta, lunesCuartaOtra, lunesQuinta, martesPrimera};
    EXPECT_EQ(conjunto.size(), 3);                // las dos primeras son la misma franja
    EXPECT_TRUE(conjunto.contains(FranjaSemana{0, 4}));

    QHash<FranjaSemana, int> mapa;
    mapa[lunesCuarta] = 1;
    mapa[lunesCuartaOtra] = 2;                    // misma clave: sobrescribe
    EXPECT_EQ(mapa.size(), 1);
    EXPECT_EQ(mapa.value(lunesCuarta), 2);
    EXPECT_EQ(mapa.value(martesPrimera, -1), -1);
}

TEST(TestCapacidadTipo, slotsTurno_devuelveLosSlotsDeclarados) {
    const SolverConfig config = configBase();

    EXPECT_TRUE(slotsTurno(config, QStringLiteral("TB")) == QVector<int>({4, 5}));
    EXPECT_TRUE(slotsTurno(config, QStringLiteral("TC")) == QVector<int>({5, 6}));
    EXPECT_TRUE(slotsTurno(config, QStringLiteral("TX")).isEmpty());
}

TEST(TestCapacidadTipo, franjasDelCurso_cuentaParesDiaFranjaYnoSlotsSueltos) {
    const SolverConfig config = configBase();

    // 2 días × 2 franjas del turno TB = 4 pares distintos; no 2 slots repetidos por día.
    const QSet<FranjaSemana> franjas = franjasDelCurso(config, config.cursos[0]);
    EXPECT_EQ(franjas.size(), 4);
    EXPECT_TRUE(franjas.contains(FranjaSemana{0, 4}));
    EXPECT_TRUE(franjas.contains(FranjaSemana{0, 5}));
    EXPECT_TRUE(franjas.contains(FranjaSemana{1, 4}));
    EXPECT_TRUE(franjas.contains(FranjaSemana{1, 5}));

    EXPECT_EQ(franjasSemanales(config, config.cursos[0]), 4);

    // Turno inexistente: sin franjas y sin error.
    CursoSolverConfig huerfano = config.cursos[0];
    huerfano.turno = QStringLiteral("TX");
    EXPECT_TRUE(franjasDelCurso(config, huerfano).isEmpty());
    EXPECT_EQ(franjasSemanales(config, huerfano), 0);
}

TEST(TestCapacidadTipo, franjasDelTipo_uneLosTurnosDeLosCursosQueLoTienen) {
    const SolverConfig config = configBase();

    // El tipo general lo requieren 1A (TB) y 2B (TC): la unión por día es {4,5,6} → 2 × 3.
    EXPECT_EQ(franjasDelTipo(config, claveGeneral()).size(), 6);

    // El laboratorio solo lo requiere 1A (TB): 2 días × 2 franjas.
    EXPECT_EQ(franjasDelTipo(config, claveLaboratorio()).size(), 4);

    // Un tipo que ninguna materia requiere no aporta franjas.
    EXPECT_TRUE(franjasDelTipo(config, claveTipo(QStringLiteral("Cancha"))).isEmpty());
}

TEST(TestCapacidadTipo, demandaDelTipo_sumaLosBloquesDeTodosLosCursosYNoLosDeOtroTipo) {
    const SolverConfig config = configBase();

    EXPECT_EQ(demandaDelTipo(config, claveGeneral()), 3);      // 2 bloques de 1A + 1 de 2B
    EXPECT_EQ(demandaDelTipo(config, claveLaboratorio()), 1);  // 1 bloque de 1A
    EXPECT_EQ(demandaDelTipo(config, claveTipo(QStringLiteral("Cancha"))), 0);
}

TEST(TestCapacidadTipo, aulasYcapacidadDelTipo_seCuentanPorClave) {
    const SolverConfig config = configBase();

    // A1 y A2 declaran el mismo tipo con distinta caja: la comparación es por clave.
    EXPECT_EQ(aulasDelTipo(config, claveGeneral()), 2);
    EXPECT_EQ(aulasDelTipo(config, claveLaboratorio()), 1);
    EXPECT_EQ(aulasDelTipo(config, claveTipo(QStringLiteral("Cancha"))), 0);

    // capacidad = aulas × |franjasDelTipo|.
    EXPECT_EQ(capacidadDelTipo(config, claveGeneral()), 2 * 6);
    EXPECT_EQ(capacidadDelTipo(config, claveLaboratorio()), 1 * 4);
    EXPECT_EQ(capacidadDelTipo(config, claveTipo(QStringLiteral("Cancha"))), 0);
}

TEST(TestCapacidadTipo, tipoGeneralAula_esAulaDeEstudioPorClave) {
    EXPECT_EQ(tipoGeneralAula(), QStringLiteral("Aula de estudio"));

    // La convención se reconoce con la misma normalización del contrato.
    EXPECT_EQ(claveTipo(tipoGeneralAula()), claveTipo(QStringLiteral("  AULA DE ESTUDIO ")));
    EXPECT_EQ(claveTipo(tipoGeneralAula()), claveTipo(QStringLiteral("\tAula de Estudio\t")));
    EXPECT_NE(claveTipo(tipoGeneralAula()), claveTipo(QStringLiteral("Laboratorio")));
}
