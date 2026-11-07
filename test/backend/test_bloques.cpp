/**
 * @file test_bloques.cpp
 * @brief Tests de las funciones puras de la rejilla de bloques de dos horas (014 T2).
 *
 * Cubre tramos y pares (2, 4, 5 y 6 franjas; el 5 no es emparejable), el desvío único
 * `Σ máx(0, descubiertas − h mod 2)`, los bloques de 1 como comprobación aparte, la cota
 * `Σ(h mod 2)`, la causa (5) con sus dos condiciones y los pesos blandos por instancia.
 */

#include <gtest/gtest.h>

#include <initializer_list>

#include <QMap>
#include <QPair>
#include <QSet>
#include <QString>
#include <QVector>

#include "backend/solver/config/bloques.hpp"
#include "backend/solver/config/solver_config.hpp"

namespace {

using rejilla::PesosBlandos;

QSet<FranjaSemana> franjas(std::initializer_list<FranjaSemana> lista) {
    QSet<FranjaSemana> conjunto;
    for (const FranjaSemana& franja : lista) conjunto.insert(franja);
    return conjunto;
}

/// Curso 1|A con un plan de materias `{nombre, horas}` y un turno de `franjasTurno`.
SolverConfig configDeCurso(const QVector<QPair<QString, int>>& materias,
                           int numDias = 1,
                           const QVector<int>& franjasTurno = QVector<int>{0, 1, 2, 3, 4, 5},
                           const QVector<EventoConfig>& eventos = {}) {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = true;
    config.dimensiones = {0, static_cast<int>(materias.size()), 0, 1, numDias,
                          static_cast<int>(franjasTurno.size())};
    config.franja_horaria = {40, static_cast<int>(franjasTurno.size())};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), franjasTurno, QString(), QString()}};
    config.eventos = eventos;

    QVector<PlanMateriaConfig> plan;
    for (int materia = 0; materia < materias.size(); ++materia) {
        config.materias.append(
            MateriaSolverConfig{materias[materia].first, QStringLiteral("regular")});
        plan.append(PlanMateriaConfig{materia, 1, materias[materia].second});
    }
    config.planes = {PlanSolverConfig{QStringLiteral("Plan"), plan}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};
    return config;
}

QMap<int, QSet<FranjaSemana>> porMateria(
    std::initializer_list<QPair<int, QSet<FranjaSemana>>> entradas) {
    QMap<int, QSet<FranjaSemana>> mapa;
    for (const auto& entrada : entradas) mapa.insert(entrada.first, entrada.second);
    return mapa;
}

} // namespace

// ─── Tramos y pares ──────────────────────────────────────────────────────────

TEST(TestBloques, tramosDeDosCuatroCincoYSeisFranjas) {
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 17}}, 4);
    const QSet<FranjaSemana> ocupadas = franjas({
        {0, 0}, {0, 1},                          // tramo de 2
        {1, 0}, {1, 1}, {1, 2}, {1, 3},          // tramo de 4
        {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5},  // tramo de 5
        {3, 0}, {3, 1}, {3, 2}, {3, 3}, {3, 4}, {3, 5},  // tramo de 6
    });

    const QVector<rejilla::Tramo> tramos =
        rejilla::tramosDeCurso(config, 0, ocupadas);

    ASSERT_EQ(tramos.size(), 4);
    EXPECT_EQ(tramos[0].dia, 0);
    EXPECT_EQ(tramos[0].inicio, 0);
    EXPECT_EQ(tramos[0].longitud, 2);
    EXPECT_EQ(tramos[1].dia, 1);
    EXPECT_EQ(tramos[1].inicio, 0);
    EXPECT_EQ(tramos[1].longitud, 4);
    EXPECT_EQ(tramos[2].dia, 2);
    EXPECT_EQ(tramos[2].inicio, 1);
    EXPECT_EQ(tramos[2].longitud, 5);
    EXPECT_EQ(tramos[3].dia, 3);
    EXPECT_EQ(tramos[3].inicio, 0);
    EXPECT_EQ(tramos[3].longitud, 6);
    for (const rejilla::Tramo& tramo : tramos) EXPECT_EQ(tramo.segmento, 0);
}

TEST(TestBloques, paresDesdeElInicioYElTramoImparNoEsEmparejable) {
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 5}}, 1);
    const QSet<FranjaSemana> ocupadas =
        franjas({{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}});

    const QVector<rejilla::Tramo> tramos =
        rejilla::tramosDeCurso(config, 0, ocupadas);
    ASSERT_EQ(tramos.size(), 1);
    ASSERT_EQ(tramos[0].longitud, 5);

    const QVector<rejilla::Par> pares = rejilla::paresDeCurso(config, 0, ocupadas);
    EXPECT_EQ(pares.size(), 2);  // (0,1) y (2,3); la franja 4 queda sin par
    EXPECT_EQ(pares[0].inicio, 0);
    EXPECT_EQ(pares[1].inicio, 2);
    EXPECT_NE(static_cast<int>(pares.size()) * 2, tramos[0].longitud);
    EXPECT_EQ(rejilla::bloquesDeUno(config, 0, ocupadas), 1);
}

TEST(TestBloques, paresDeTramosParesNoDejanBloquesDeUno) {
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 12}}, 3);
    const QSet<FranjaSemana> ocupadas = franjas({
        {0, 0}, {0, 1},                          // 2 → 1 par
        {1, 0}, {1, 1}, {1, 2}, {1, 3},          // 4 → 2 pares
        {2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5},  // 6 → 3 pares
    });

    const QVector<rejilla::Par> pares = rejilla::paresDeCurso(config, 0, ocupadas);
    EXPECT_EQ(pares.size(), 6);
    EXPECT_EQ(rejilla::bloquesDeUno(config, 0, ocupadas), 0);
}

TEST(TestBloques, unEventoRompeElTramo) {
    const QVector<EventoConfig> evento = {
        EventoConfig{QStringLiteral("Receso"), QStringLiteral("manana"), 2, 1, 10, QString(),
                     QString()}};

    const SolverConfig sinEvento = configDeCurso({{QStringLiteral("Matemática"), 4}}, 1,
                                                 QVector<int>{0, 1, 2, 3});
    const SolverConfig conEvento = configDeCurso({{QStringLiteral("Matemática"), 4}}, 1,
                                                 QVector<int>{0, 1, 2, 3}, evento);
    const QSet<FranjaSemana> ocupadas = franjas({{0, 0}, {0, 1}, {0, 2}, {0, 3}});

    const QVector<rejilla::Tramo> tramosSin = rejilla::tramosDeCurso(sinEvento, 0, ocupadas);
    ASSERT_EQ(tramosSin.size(), 1);
    EXPECT_EQ(tramosSin[0].longitud, 4);

    const QVector<rejilla::Tramo> tramosCon = rejilla::tramosDeCurso(conEvento, 0, ocupadas);
    ASSERT_EQ(tramosCon.size(), 2);
    EXPECT_EQ(tramosCon[0].segmento, 0);
    EXPECT_EQ(tramosCon[0].longitud, 2);
    EXPECT_EQ(tramosCon[1].segmento, 1);
    EXPECT_EQ(tramosCon[1].longitud, 2);
}

// ─── Desvío único y bloques de 1 ─────────────────────────────────────────────

TEST(TestBloques, desvioDeMateriaParAlineadaEsCero) {
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 2}});
    EXPECT_EQ(rejilla::desvioDeMateria(config, 0, 0, franjas({{0, 0}, {0, 1}})), 0);
    EXPECT_EQ(rejilla::desvioDeMateria(config, 0, 0, franjas({{0, 0}, {0, 2}})), 2);
}

TEST(TestBloques, bloqueDeUnoNoSumaAlDesvioDeUnaMateriaImpar) {
    // Una materia de 1 h es su propio resto: la franja suelta (bloque de 1) no es desvío.
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 1}});
    const QSet<FranjaSemana> ocupadas = franjas({{0, 0}});

    EXPECT_EQ(rejilla::desvioDeMateria(config, 0, 0, ocupadas), 0);
    EXPECT_EQ(rejilla::desvioDeCurso(config, 0, porMateria({{0, ocupadas}})), 0);
    // El bloque de 1 se comprueba aparte (P4), no como término de la métrica.
    EXPECT_EQ(rejilla::bloquesDeUno(config, 0, ocupadas), 1);
}

TEST(TestBloques, desvioDeMateriasImparAlineadaYDesalineada) {
    // 3 h: un bloque simple (0,1) + el resto en 2 → exactamente 1 franja descubierta.
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 3}});
    EXPECT_EQ(rejilla::desvioDeMateria(config, 0, 0, franjas({{0, 0}, {0, 1}, {0, 2}})), 0);

    // Las tres sueltas: 3 descubiertas frente al tope 1 → desvío 2.
    EXPECT_EQ(rejilla::desvioDeMateria(config, 0, 0, franjas({{0, 0}, {0, 2}, {0, 4}})), 2);
}

TEST(TestBloques, desvioDeMateriaDeCuatroACincoYSeisHoras) {
    // 4 h en dos pares contiguos → 0.
    const SolverConfig cuatro = configDeCurso({{QStringLiteral("Matemática"), 4}});
    EXPECT_EQ(rejilla::desvioDeMateria(cuatro, 0, 0,
                                       franjas({{0, 0}, {0, 1}, {0, 2}, {0, 3}})),
              0);
    // 4 h con un tramo de 3 y una suelta: 2 descubiertas y tope 0.
    EXPECT_EQ(rejilla::desvioDeMateria(cuatro, 0, 0,
                                       franjas({{0, 0}, {0, 1}, {0, 2}, {0, 4}})),
              2);
    // 5 h (impar): un tramo de 4 más la última suelta → 1 descubierta = resto.
    const SolverConfig cinco = configDeCurso({{QStringLiteral("Matemática"), 5}});
    EXPECT_EQ(rejilla::desvioDeMateria(cinco, 0, 0,
                                       franjas({{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}})),
              0);
    // 6 h en un tramo de 6 → 0.
    const SolverConfig seis = configDeCurso({{QStringLiteral("Matemática"), 6}});
    EXPECT_EQ(rejilla::desvioDeMateria(
                  seis, 0, 0,
                  franjas({{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}})),
              0);
}

TEST(TestBloques, bloqueCompartidoLegitimoNoEsDesvio) {
    // Dos materias de 3 h: cada una tiene un bloque simple y su resto; los dos restos
    // (2 y 3) forman el bloque compartido. El curso tiene 3 pares y ningún bloque de 1.
    const SolverConfig config = configDeCurso(
        {{QStringLiteral("Matemática"), 3}, {QStringLiteral("Historia"), 3}});
    const QMap<int, QSet<FranjaSemana>> ocupadas = porMateria({
        {0, franjas({{0, 0}, {0, 1}, {0, 2}})},
        {1, franjas({{0, 3}, {0, 4}, {0, 5}})},
    });

    EXPECT_EQ(rejilla::desvioDeCurso(config, 0, ocupadas), 0);
    EXPECT_EQ(rejilla::franjasDescubiertasDeCurso(config, 0, ocupadas), 2);
    EXPECT_EQ(rejilla::cotaDeCurso(config, 0), 2);
}

TEST(TestBloques, materiaParEnBloqueCompartidoSumaDesvio) {
    // Dos materias de 2 h intercaladas: la rejilla del curso se empareja, pero ninguna
    // materia queda en un bloque suyo (2 franjas descubiertas con tope 0 cada una).
    const SolverConfig config = configDeCurso(
        {{QStringLiteral("Matemática"), 2}, {QStringLiteral("Historia"), 2}});
    const QMap<int, QSet<FranjaSemana>> ocupadas = porMateria({
        {0, franjas({{0, 0}, {0, 2}})},
        {1, franjas({{0, 1}, {0, 3}})},
    });

    EXPECT_EQ(rejilla::desvioDeCurso(config, 0, ocupadas), 4);
    EXPECT_EQ(rejilla::franjasDescubiertasDeCurso(config, 0, ocupadas), 4);
    EXPECT_EQ(rejilla::cotaDeCurso(config, 0), 0);
    EXPECT_EQ(rejilla::bloquesDeUno(
                  config, 0, franjas({{0, 0}, {0, 1}, {0, 2}, {0, 3}})),
              0);
}

// ─── Cota y apariciones ──────────────────────────────────────────────────────

TEST(TestBloques, cotaDeCursoSumaDeRestos) {
    const SolverConfig config = configDeCurso({{QStringLiteral("A"), 1},
                                               {QStringLiteral("B"), 2},
                                               {QStringLiteral("C"), 3},
                                               {QStringLiteral("D"), 4}});
    EXPECT_EQ(rejilla::cotaDeCurso(config, 0), 2);
}

TEST(TestBloques, cumpleTopeDeCursoComparaDescubiertasConLaCota) {
    const SolverConfig config = configDeCurso(
        {{QStringLiteral("Matemática"), 3}, {QStringLiteral("Historia"), 3}});

    // Alineadas: cada una descubre su resto → 2 descubiertas = cota 2.
    const QMap<int, QSet<FranjaSemana>> alineadas = porMateria({
        {0, franjas({{0, 0}, {0, 1}, {0, 2}})},
        {1, franjas({{0, 3}, {0, 4}, {0, 5}})},
    });
    EXPECT_TRUE(rejilla::cumpleTopeDeCurso(config, 0, alineadas));

    // Todas sueltas: 6 descubiertas frente a la cota 2.
    const QMap<int, QSet<FranjaSemana>> sueltas = porMateria({
        {0, franjas({{0, 0}, {0, 2}, {0, 4}})},
        {1, franjas({{0, 1}, {0, 3}, {0, 5}})},
    });
    EXPECT_FALSE(rejilla::cumpleTopeDeCurso(config, 0, sueltas));
}

TEST(TestBloques, aparicionesEsperadasEsElTechoDeLaMitad) {
    const QVector<QPair<int, int>> tabla = {
        {1, 1}, {2, 1}, {3, 2}, {4, 2}, {5, 3}, {6, 3},
        {7, 4}, {8, 4}, {9, 5}, {10, 5}, {11, 6},
    };
    for (const auto& fila : tabla)
        EXPECT_EQ(rejilla::aparicionesEsperadas(fila.first), fila.second);
}

TEST(TestBloques, diasDeMateriaYIncumplimientosDeSeparacion) {
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 6}}, 5,
                                              QVector<int>{0, 1});

    // Días 0, 2 y 4: terna separada (sin pares consecutivos).
    const QSet<FranjaSemana> separadas =
        franjas({{0, 0}, {2, 0}, {4, 0}, {0, 1}, {2, 1}, {4, 1}});
    const QVector<int> dias = rejilla::diasDeMateria(config, 0, 0, separadas);
    EXPECT_EQ(dias, (QVector<int>{0, 2, 4}));
    EXPECT_EQ(rejilla::incumplimientosSeparacion(dias), 0);

    // Días 2, 3 y 4: dos pares consecutivos.
    const QSet<FranjaSemana> seguidas =
        franjas({{2, 0}, {3, 0}, {4, 0}});
    const QVector<int> diasSeguidos = rejilla::diasDeMateria(config, 0, 0, seguidas);
    EXPECT_EQ(diasSeguidos, (QVector<int>{2, 3, 4}));
    EXPECT_EQ(rejilla::incumplimientosSeparacion(diasSeguidos), 2);

    // Una materia que no está en el plan no tiene días.
    EXPECT_TRUE(rejilla::diasDeMateria(config, 0, 7, separadas).isEmpty());
}

// ─── Causa (5) ──────────────────────────────────────────────────────────────

TEST(TestBloques, causaCincoPorTotalImpar) {
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 3}}, 5);
    QString motivo;
    EXPECT_TRUE(rejilla::causaCincoDeCurso(config, 0, &motivo));
    EXPECT_TRUE(motivo.contains(QStringLiteral("Causa (5)")));
    EXPECT_TRUE(motivo.contains(QStringLiteral("1|A")));
    EXPECT_TRUE(motivo.contains(QStringLiteral("3 horas")));
    EXPECT_TRUE(motivo.contains(QStringLiteral("impar")));
}

TEST(TestBloques, causaCincoPorMasAparicionesQueDias) {
    // Total par (11 + 1) para que no dispare primero la condición de total impar.
    const SolverConfig config = configDeCurso(
        {{QStringLiteral("Matemática"), 11}, {QStringLiteral("Historia"), 1}}, 5);
    QString motivo;
    EXPECT_TRUE(rejilla::causaCincoDeCurso(config, 0, &motivo));
    EXPECT_TRUE(motivo.contains(QStringLiteral("Causa (5)")));
    EXPECT_TRUE(motivo.contains(QStringLiteral("Matemática")));
    EXPECT_TRUE(motivo.contains(QStringLiteral("6 apariciones")));
    EXPECT_TRUE(motivo.contains(QStringLiteral("5 días")));
}

TEST(TestBloques, causaCincoPorOnceHorasOMas) {
    // Con 5 días, 7–10 h se informa como incumplimiento de separación; 11 h o más no es
    // solución (más apariciones que días).
    const QVector<QVector<QPair<QString, int>>> casos = {
        {{QStringLiteral("A"), 7}, {QStringLiteral("B"), 1}},   // total 8
        {{QStringLiteral("A"), 8}},                             // total 8
        {{QStringLiteral("A"), 9}, {QStringLiteral("B"), 1}},   // total 10
        {{QStringLiteral("A"), 10}},                            // total 10
    };
    for (const auto& materias : casos) {
        const SolverConfig config = configDeCurso(materias, 5);
        QString motivo;
        EXPECT_FALSE(rejilla::causaCincoDeCurso(config, 0, &motivo));
        EXPECT_TRUE(motivo.isEmpty());
    }

    const SolverConfig once = configDeCurso(
        {{QStringLiteral("Matemática"), 11}, {QStringLiteral("Historia"), 1}}, 5);
    QString motivo;
    EXPECT_TRUE(rejilla::causaCincoDeCurso(once, 0, &motivo));
    EXPECT_TRUE(motivo.contains(QStringLiteral("5 días"))) << motivo.toStdString();
}


TEST(TestBloques, causaCincoSinMotivoNoFallaYSeLimpia) {
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 4}}, 5);
    EXPECT_FALSE(rejilla::causaCincoDeCurso(config, 0, nullptr));

    QString motivo = QStringLiteral("anterior");
    EXPECT_FALSE(rejilla::causaCincoDeCurso(config, 0, &motivo));
    EXPECT_TRUE(motivo.isEmpty());
}

// ─── Pesos blandos ──────────────────────────────────────────────────────────

TEST(TestBloques, pesosBlandosSeparacionDominanteYViernes) {
    // Curso de 5 días × 6 franjas (30) con 24 h: 6 huecos máximos. La separación pesa
    // `maxHuecos + 1` y domina a todos los huecos no-viernes; los huecos al final son duros.
    const SolverConfig config = configDeCurso({{QStringLiteral("Matemática"), 24}}, 5);
    const PesosBlandos pesos = rejilla::pesosBlandos(config);
    const int maxHuecos = 6;
    EXPECT_EQ(pesos.huecoViernes, 1);
    EXPECT_EQ(pesos.separacion, maxHuecos + 1);
    EXPECT_GT(pesos.separacion, maxHuecos * pesos.huecoViernes);
}


