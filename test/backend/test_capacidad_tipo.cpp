#include <gtest/gtest.h>

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>

#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "common/aforo_aula.hpp"

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

/// Configuración **aparte** con una cancha (aforo 2) y un turno de 8 franjas cortado tras la
/// segunda (1 + 3 = 4 posiciones de par por día). No se toca `configBase()`: sus aserciones
/// exigen `franjasDelTipo(«Cancha»)` vacía.
SolverConfig configConCancha() {
    SolverConfig config;
    config.aula_fija = false;
    config.dimensiones.num_dias = 1;
    config.dimensiones.num_slots_dia = 8;
    config.franja_horaria.duracion_minutos = 40;

    TurnoConfig manana;
    manana.nombre = QStringLiteral("Mañana");
    manana.slot = {0, 1, 2, 3, 4, 5, 6, 7};
    config.turnos.turnos = {manana};
    config.eventos = {EventoConfig{QStringLiteral("Receso"), QStringLiteral("Mañana"), 2, 1, 10,
                                   QString(), QString()}};

    config.materias = {{QStringLiteral("Educación Física 1º"), QStringLiteral("Cancha")}};
    config.aulas = {{QStringLiteral("Cancha"), QStringLiteral("Cancha"), 88}};
    config.planes = {{QStringLiteral("A"), {{0, 1, 2}}}};
    config.cursos = {{1, QStringLiteral("A"), QStringLiteral("Mañana"), 40, QStringLiteral("A"), -1}};
    return config;
}

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

// ── 018 T1: aforo en un solo punto, capacidad con aforo y posiciones de par ──────────────

TEST(TestCapacidadTipo, aforoDelTipo_canchaDosYelRestoUno) {
    // RF-1/RF-2: el tipo se reconoce por su clave normalizada, no por el texto exacto.
    EXPECT_EQ(aforo_aula::claveCancha(), claveTipo(QStringLiteral("Cancha")));
    EXPECT_EQ(aforoDelTipo(claveTipo(QStringLiteral("Cancha"))), 2);
    EXPECT_EQ(aforoDelTipo(claveTipo(QStringLiteral("  CANCHA "))), 2);
    EXPECT_EQ(aforoDelTipo(claveTipo(QStringLiteral("Campo"))), 1);
    EXPECT_EQ(aforoDelTipo(claveGeneral()), 1);
    EXPECT_EQ(aforoDelTipo(claveLaboratorio()), 1);
    EXPECT_EQ(aforoDelTipo(QString()), 1);
}

TEST(TestCapacidadTipo, aforoDelAula_indiceInvalidoEsUno) {
    const SolverConfig config = configConCancha();

    EXPECT_EQ(aforoDelAula(config, 0), 2);    // la cancha
    EXPECT_EQ(aforoDelAula(config, -1), 1);   // índice ausente
    EXPECT_EQ(aforoDelAula(config, 1), 1);    // índice fuera de rango

    // En la configuración base no hay cancha: todas sus aulas tienen aforo 1.
    const SolverConfig base = configBase();
    EXPECT_EQ(aforoDelAula(base, 0), 1);
    EXPECT_EQ(aforoDelAula(base, 2), 1);
}

TEST(TestCapacidadTipo, capacidadDelTipo_multiplicaPorElAforo) {
    const SolverConfig config = configConCancha();

    // Aulas × franjas × aforo: 1 × 8 franjas del día × 2 = 16 (RF-6).
    EXPECT_EQ(aulasDelTipo(config, claveTipo(QStringLiteral("Cancha"))), 1);
    EXPECT_EQ(franjasDelTipo(config, claveTipo(QStringLiteral("Cancha"))).size(), 8);
    EXPECT_EQ(capacidadDelTipo(config, claveTipo(QStringLiteral("Cancha"))), 1 * 8 * 2);

    // Un tipo de aforo 1 conserva la cuenta anterior (aulas × franjas).
    const SolverConfig base = configBase();
    EXPECT_EQ(capacidadDelTipo(base, claveGeneral()), 2 * 6);
    EXPECT_EQ(capacidadDelTipo(base, claveLaboratorio()), 1 * 4);
}

TEST(TestCapacidadTipo, posicionesDePar_cuatroPorDiaYveintePorTurno) {
    const SolverConfig config = configConCancha();
    const TurnoConfig& manana = config.turnos.turnos[0];

    // Segmentos [0,1] y [2..7]: 1 + 3 = 4 posiciones alineadas por día.
    EXPECT_EQ(posicionesDePar(manana, config.eventos), 4);

    // El turno completo son 5 días: 20 posiciones (RF-5).
    EXPECT_EQ(5 * posicionesDePar(manana, config.eventos), 20);

    // Un turno sin eventos es un único segmento: 8 / 2 = 4 posiciones.
    TurnoConfig sinReceso = manana;
    EXPECT_EQ(posicionesDePar(sinReceso, {}), 4);

    // Un segmento de longitud impar no ofrece posición para su última franja.
    TurnoConfig tresFranjas;
    tresFranjas.nombre = QStringLiteral("Corto");
    tresFranjas.slot = {0, 1, 2};
    EXPECT_EQ(posicionesDePar(tresFranjas, {}), 1);

    // Un turno sin franjas no tiene posiciones.
    TurnoConfig vacio;
    EXPECT_EQ(posicionesDePar(vacio, {}), 0);
}

TEST(TestCapacidadTipo, toJson_noViajaElAforo) {
    // RNF-8: el aforo es regla del motor y no viaja en el contrato. Una config con cancha
    // se serializa sin la clave «aforo», ni en el aula ni en la raíz.
    const SolverConfig config = configConCancha();
    const QJsonObject obj = config.toJson();

    EXPECT_FALSE(obj.contains(QStringLiteral("aforo")));
    const QJsonArray aulas = obj.value(QStringLiteral("aulas")).toArray();
    ASSERT_EQ(aulas.size(), 1);
    EXPECT_FALSE(aulas.at(0).toObject().contains(QStringLiteral("aforo")));

    const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    EXPECT_FALSE(json.contains("\"aforo\"")) << json.toStdString();
}

