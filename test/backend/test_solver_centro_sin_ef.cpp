/**
 * @file test_solver_centro_sin_ef.cpp
 * @brief Aceptación 014a sin Educación Física (24 cursos / 848 clases), con 8 workers.
 *
 * El producto usa 8 workers por defecto (014b); esta prueba usa ese mismo defecto. Se ejecuta
 * con `ctest -L slow` / `--preset slow-release` y queda fuera de la suite normal.
 */

#include <gtest/gtest.h>

#include <iostream>

#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>

#include "backend/services/ValidadorSalidaSolver.hpp"
#include "backend/services/analisis_bloques.hpp"
#include "backend/solver/config/bloques.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/restricciones/restricciones_aulas.hpp"
#include "backend/solver/restricciones/restricciones_bloques.hpp"
#include "backend/solver/restricciones/restricciones_profesores.hpp"
#include "backend/solver/restricciones/restricciones_turnos.hpp"
#include "backend/solver/solver.hpp"
#include "backend/solver/variables.hpp"
#include "fixtures/centro_sin_ef.hpp"

namespace {


/// Separación recalculada desde el horario con las funciones puras (contraste del informe).
int separacionRecalculada(const SolverConfig& config, const HorarioSalida& horario) {
    int total = 0;
    for (int curso = 0; curso < config.cursos.size(); ++curso) {
        const QString clave =
            claveCurso(config.cursos[curso].anio, config.cursos[curso].seccion);
        QMap<int, QSet<FranjaSemana>> porMateria;
        for (const DiaOutput& dia : horario.horarios.value(clave).dias)
            for (const AsignacionOutput& asignacion : dia.asignaciones)
                porMateria[asignacion.materia].insert(FranjaSemana{dia.dia, asignacion.slot});

        for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[curso]))
            total += rejilla::incumplimientosSeparacion(rejilla::diasDeMateria(
                config, curso, mc.materiaIDx, porMateria.value(mc.materiaIDx)));
    }
    return total;
}

} // namespace

TEST(TestSolverCentroSinEf, fixtureDeAceptacion) {
    const SolverConfig config = fixtureCentroSinEf();
    ASSERT_EQ(config.cursos.size(), 24);

    // Tamaño del modelo del fixture (se informa; no forma parte de la aserción de tiempo).
    {
        CpModelBuilder modelo;
        const VariablesSolver vars = crearVariables(modelo, config);
        agregarRestriccionesTurnos(modelo, config, vars);
        agregarRestriccionesAulas(modelo, config, vars);
        agregarRestriccionProfesores(modelo, config, vars);
        agregarRestriccionesBloques(modelo, config, vars);
        const CpModelProto proto = modelo.Build();
        std::cout << "[   INFO   ] centro sin EF: " << proto.variables_size()
                  << " variables, " << proto.constraints_size() << " restricciones" << std::endl;
    }

    Solver solver;
    // Valores por defecto de la 014b: 60/20/10 s y 8 workers.
    const auto primera = solver.resolver(config);
    ASSERT_TRUE(primera.exito) << primera.errores.join("\n").toStdString();

    const AnalisisBloques analisis = analizarBloques(primera.resultado, config);
    EXPECT_EQ(primera.resultado.metadata.total_asignaciones, 848);
    EXPECT_EQ(analisis.bloques.size(), 424);
    EXPECT_EQ(analisis.bloquesDeUno, 0);
    EXPECT_EQ(analisis.desvio, 0);
    EXPECT_EQ(analisis.tope, 8);
    EXPECT_EQ(analisis.incumplimientosSeparacion, separacionRecalculada(config, primera.resultado));
    EXPECT_EQ(analisis.huecosNoFinal, 0);

    // P4 acepta la salida real.
    const AnalisisSalida validacion = analizarSalidaSolver(primera.resultado, config);
    EXPECT_TRUE(validacion.presentable()) << validacion.conflictos.join("\n").toStdString();

    // Referencia (2a agotada): la separación informada no puede ser peor.
    Solver::PresupuestoFases sinSeparacion;
    sinSeparacion.separacionHuecos = 0.0;
    const auto referencia = solver.resolver(config, sinSeparacion);
    ASSERT_TRUE(referencia.exito) << referencia.errores.join("\n").toStdString();
    const AnalisisBloques analisisReferencia = analizarBloques(referencia.resultado, config);
    EXPECT_LE(analisis.incumplimientosSeparacion,
              analisisReferencia.incumplimientosSeparacion);

    // Techo de tiempo. Se mide en Release: con los valores por defecto (60/20/10 y 8 workers)
    // la resolución completa tarda ~150-230 s en este host según la carga, por debajo del plazo
    // de 300 s del servicio (RNF-2). El techo del test se alinea con ese plazo (300 s) en vez de
    // usar un margen más estricto que la spec: con 8 workers sobre 4 núcleos la varianza del host
    // hace frágil cualquier techo por debajo del criterio. En Debug es aún mayor.
#ifdef NDEBUG
    constexpr double kTechoMs = 300000.0;
#else
    constexpr double kTechoMs = 1200000.0;
#endif
    EXPECT_LT(primera.tiempo_ms, kTechoMs);

    std::cout << "[   INFO   ] centro sin EF: 848 clases, desvío 0, "
              << analisis.bloquesSimples << " simples, " << analisis.bloquesCompartidos
              << " compartidos, separación " << analisis.incumplimientosSeparacion
              << " (referencia " << analisisReferencia.incumplimientosSeparacion
              << "), huecos no finales " << analisis.huecosNoFinal
              << ", tiempo " << primera.tiempo_ms << " ms" << std::endl;
}
