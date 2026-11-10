/**
 * @file test_solver_centro_real.cpp
 * @brief Aceptación lenta del centro real **con Educación Física** (018 T5, RF-9).
 *
 * Siembra el centro en un directorio temporal, compone la entrada desde la base (los 24 cursos
 * del perfil, igual que `test_entrada_seed`), valida V1–V19 y **resuelve**. Es el criterio
 * ejecutable del aforo simultáneo de la cancha:
 *
 *  - 876 clases y desvío 0, sin bloques de uno ni huecos fuera del final;
 *  - las 96 clases de Educación Física, todas en el aula de clave `cancha`;
 *  - ninguna aula por encima de su aforo y **ningún par cruzado** en la cancha;
 *  - menos de 300 s en Release.
 *
 * Complementa —no sustituye— el fixture sin EF de la 014 (`test_solver_centro_sin_ef`). Se
 * ejecuta fuera de la suite normal con `ctest --preset slow-release` (etiqueta `slow`).
 */

#include <iostream>
#include <tuple>

#include <QMap>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QVector>

#include "backend/services/NucleoDatos.hpp"
#include "backend/services/ServicioCursos.hpp"
#include "backend/services/ValidadorEntradaSolver.hpp"
#include "backend/services/ValidadorSalidaSolver.hpp"
#include "backend/services/analisis_bloques.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/solver.hpp"
#include "common/aforo_aula.hpp"
#include "seed/perfil.hpp"
#include "seed/sembrador.hpp"
#include "services/entrada_solver_builder.hpp"

namespace {

/// Curso del perfil con ese año y sección, o nullptr.
const seed::CursoDef* cursoDePerfil(const seed::Perfil& perfil, int anio, const QString& seccion)
{
    for (const seed::CursoDef& curso : perfil.cursos)
        if (curso.anio == anio && curso.seccion == seccion) return &curso;
    return nullptr;
}

/// Índice del aula de clave `cancha` en la configuración, o -1.
int indiceCancha(const SolverConfig& config)
{
    for (int a = 0; a < config.aulas.size(); ++a)
        if (claveTipo(config.aulas[a].tipo) == aforo_aula::claveCancha()) return a;
    return -1;
}

/// ¿La materia `m` exige el tipo de la cancha?
bool esEducacionFisica(const SolverConfig& config, int m)
{
    if (m < 0 || m >= config.materias.size()) return false;
    return claveTipo(config.materias[m].tipo) == aforo_aula::claveCancha();
}

}  // namespace

class TestSolverCentroReal : public QObject
{
    Q_OBJECT

private slots:
    void generaElCentroRealConEducacionFisicaEnLaCancha()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        seed::OpcionesAplicacion siembra;
        siembra.rutaDb = ruta;
        const seed::ResultadoAplicacion sembrado = seed::aplicar(siembra);
        QVERIFY2(sembrado.ok(), qPrintable(sembrado.detalle));

        const seed::Perfil& perfil = seed::perfilPorDefecto();

        const QString conexion = QStringLiteral("test_solver_centro_real");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conexion);
            db.setDatabaseName(ruta);
            QVERIFY2(db.open(), qPrintable(db.lastError().text()));
            NucleoDatos nucleo(db);

            // La rejilla del centro: 8 franjas por turno en 2 turnos.
            int totalSlots = 0;
            const auto turnos = nucleo.listarTurnos();
            QVERIFY2(turnos.ok, qPrintable(turnos.mensajeError));
            for (const TurnoDTO& turno : turnos.valor) totalSlots += turno.numSlots;
            QCOMPARE(totalSlots, 16);

            // Los 24 cursos del centro con el plan, el turno y la matrícula del perfil.
            GeneracionSolver::OpcionesUI ui;
            ui.numDias = 5;
            ui.numSlotsDia = totalSlots;
            ui.usarAulasFijas = true;

            const auto cursos = nucleo.listarCursos();
            QVERIFY2(cursos.ok, qPrintable(cursos.mensajeError));
            QCOMPARE(cursos.valor.size(), 24);
            for (const CursoDTO& curso : cursos.valor) {
                const seed::CursoDef* delPerfil =
                    cursoDePerfil(perfil, curso.anio, curso.seccion);
                QVERIFY2(delPerfil != nullptr,
                         qPrintable(nombreCurso(curso.anio, curso.seccion)));
                GeneracionSolver::ParametrosCurso parametros;
                parametros.plan = delPerfil->plan;
                parametros.turno = delPerfil->turno;
                parametros.numEstudiantes = delPerfil->matricula;
                ui.cursos.insert(nombreCurso(curso.anio, curso.seccion), parametros);
            }
            QCOMPARE(ui.cursos.size(), 24);

            const GeneracionSolver::Composicion comp =
                GeneracionSolver::construirEntrada(nucleo, ui);
            QVERIFY2(comp.avisos.isEmpty(),
                     qPrintable(comp.avisos.join(QStringLiteral("; "))));

            const Resultado<SolverConfig> validacion = validarEntradaSolver(comp.entrada);
            QVERIFY2(validacion.ok, qPrintable(validacion.mensajeError));
            const SolverConfig& config = validacion.valor;
            QCOMPARE(config.cursos.size(), 24);

            // ── Resolución completa (valores por defecto: 8 workers, 60/20/10) ──
            Solver solver;
            const Solver::ResultadoSolver resultado = solver.resolver(config);
            QVERIFY2(resultado.exito,
                     qPrintable(resultado.errores.join(QStringLiteral("\n"))));
            const HorarioSalida& horario = resultado.resultado;

            // ── Informe de bloques: 876 clases, desvío 0, sin bloques de uno ni huecos ──
            const AnalisisBloques analisis = analizarBloques(horario, config);
            QCOMPARE(horario.metadata.total_asignaciones, 876);
            QCOMPARE(analisis.desvio, 0);
            QCOMPARE(analisis.bloquesDeUno, 0);
            QCOMPARE(analisis.huecosNoFinal, 0);
            QCOMPARE(analisis.bloques.size(), 876 / 2);

            // ── Comprobación de la salida (P4) con aforo y pares cruzados ──
            const AnalisisSalida salida = analizarSalidaSolver(horario, config);
            QVERIFY2(salida.presentable(),
                     qPrintable(salida.conflictos.join(QStringLiteral("\n"))));

            // ── Educación Física: 96 clases, todas en la cancha ──
            const int cancha = indiceCancha(config);
            QVERIFY2(cancha >= 0, "el centro debe tener un aula de clave cancha");
            int totalEf = 0;
            int efEnCancha = 0;
            for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd();
                 ++it) {
                for (const DiaOutput& dia : it.value().dias) {
                    for (const AsignacionOutput& asignacion : dia.asignaciones) {
                        if (!esEducacionFisica(config, asignacion.materia)) continue;
                        ++totalEf;
                        if (asignacion.aula == cancha) ++efEnCancha;
                    }
                }
            }
            QCOMPARE(totalEf, 96);
            QCOMPARE(efEnCancha, 96);

            // ── Recuento propio: ninguna aula por encima de su aforo ──
            QMap<std::tuple<int, int, int>, int> porAulaFranja;   // (aula, día, slot) -> clases
            for (auto it = horario.horarios.constBegin(); it != horario.horarios.constEnd(); ++it)
                for (const DiaOutput& dia : it.value().dias)
                    for (const AsignacionOutput& asignacion : dia.asignaciones)
                        if (asignacion.aula >= 0)
                            ++porAulaFranja[{asignacion.aula, dia.dia, asignacion.slot}];
            for (auto it = porAulaFranja.constBegin(); it != porAulaFranja.constEnd(); ++it) {
                const int aula = std::get<0>(it.key());
                const int dia = std::get<1>(it.key());
                const int slot = std::get<2>(it.key());
                QVERIFY2(it.value() <= aforoDelAula(config, aula),
                         qPrintable(QStringLiteral("el aula %1 supera su aforo el día %2 en la "
                                                   "franja %3 (%4 clases)")
                                        .arg(aula)
                                        .arg(dia)
                                        .arg(slot)
                                        .arg(it.value())));
            }

            // ── Recuento propio: ningún par cruzado en la cancha ──
            int paresCruzados = 0;
            for (int c = 0; c < config.cursos.size(); ++c) {
                const TurnoConfig* turno = nullptr;
                for (const TurnoConfig& t : config.turnos.turnos) {
                    if (t.nombre == config.cursos[c].turno) {
                        turno = &t;
                        break;
                    }
                }
                if (turno == nullptr) continue;

                const QVector<QVector<int>> segmentos = segmentosTurno(*turno, config.eventos);
                const QString clave = claveCurso(config.cursos[c].anio, config.cursos[c].seccion);
                const CursoOutput curso = horario.horarios.value(clave);
                for (const DiaOutput& dia : curso.dias) {
                    QSet<int> canchaSlots;
                    for (const AsignacionOutput& asignacion : dia.asignaciones)
                        if (asignacion.aula >= 0 && aforoDelAula(config, asignacion.aula) > 1)
                            canchaSlots.insert(asignacion.slot);
                    if (canchaSlots.isEmpty()) continue;
                    for (const QVector<int>& segmento : segmentos)
                        for (int k = 0; k + 1 < segmento.size(); k += 2)
                            if (canchaSlots.contains(segmento[k])
                                != canchaSlots.contains(segmento[k + 1]))
                                ++paresCruzados;
                }
            }
            QCOMPARE(paresCruzados, 0);

            // ── Plazo (RNF-1): 300 s en Release; en Debug el techo es mayor (no es la medida) ──
#ifdef NDEBUG
            constexpr double kTechoMs = 300000.0;
#else
            constexpr double kTechoMs = 900000.0;
#endif
            QVERIFY2(resultado.tiempo_ms < kTechoMs,
                     qPrintable(QStringLiteral("tiempo %1 ms ≥ techo %2 ms")
                                    .arg(resultado.tiempo_ms)
                                    .arg(kTechoMs)));

            std::cout << "[   INFO   ] centro real: " << horario.metadata.total_asignaciones
                      << " clases, EF " << totalEf << " en la cancha, desvío " << analisis.desvio
                      << ", bloques de uno " << analisis.bloquesDeUno << ", huecos no finales "
                      << analisis.huecosNoFinal << ", separación "
                      << analisis.incumplimientosSeparacion << ", pares cruzados "
                      << paresCruzados << ", tiempo " << resultado.tiempo_ms << " ms"
                      << std::endl;
        }
        QSqlDatabase::removeDatabase(conexion);
    }
};

QTEST_GUILESS_MAIN(TestSolverCentroReal)

#include "test_solver_centro_real.moc"
