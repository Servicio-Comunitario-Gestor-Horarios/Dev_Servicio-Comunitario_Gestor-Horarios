#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QVector>

#include <backend/services/CargadorConfiguracionSolver.hpp>
#include <backend/services/NucleoDatos.hpp>
#include <backend/services/ServicioCursos.hpp>
#include <backend/services/ValidadorEntradaSolver.hpp>
#include <backend/solver/config/solver_config.hpp>
#include <seed/perfil.hpp>
#include <seed/sembrador.hpp>

#include "services/entrada_solver_builder.hpp"

namespace {

/// Curso del perfil con ese año y sección, o nullptr.
const seed::CursoDef* cursoDePerfil(const seed::Perfil& perfil, int anio,
                                    const QString& seccion)
{
    for (const seed::CursoDef& curso : perfil.cursos) {
        if (curso.anio == anio && curso.seccion == seccion) {
            return &curso;
        }
    }
    return nullptr;
}

} // namespace

/// Verificación de integración del centro real **sin resolver**: siembra el conjunto,
/// compone la entrada del solver con `construirEntrada` (plan, turno y matrícula de cada
/// curso del perfil) y comprueba que es una entrada válida del contrato, que el preset
/// hace ida y vuelta y que el informe declara el conjunto como no generable.
///
/// Sustituye al antiguo test de generación del seed (017-T4): con 48 clases de Educación Física
/// por turno frente a 40 franjas de cancha, el conjunto aún no genera; esa verificación se hará
/// al cerrar el aforo simultáneo (RF-9, RNF-2).
class TestEntradaSeed : public QObject
{
    Q_OBJECT

private slots:
    void componeYValidaLaEntradaSinResolver()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        seed::OpcionesAplicacion siembra;
        siembra.rutaDb = ruta;
        const seed::ResultadoAplicacion sembrado = seed::aplicar(siembra);
        QVERIFY2(sembrado.ok(), qPrintable(sembrado.detalle));

        const seed::Perfil& perfil = seed::perfilPorDefecto();

        const QString conexion = QStringLiteral("test_entrada_seed");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conexion);
            db.setDatabaseName(ruta);
            QVERIFY2(db.open(), qPrintable(db.lastError().text()));
            NucleoDatos nucleo(db);

            // Franjas totales de la rejilla: 8 por turno (2 turnos).
            int totalSlots = 0;
            const auto turnos = nucleo.listarTurnos();
            QVERIFY2(turnos.ok, qPrintable(turnos.mensajeError));
            for (const TurnoDTO& turno : turnos.valor) {
                totalSlots += turno.numSlots;
            }
            QCOMPARE(totalSlots, 16);

            // Los 24 cursos del centro con el plan, el turno y la matrícula del perfil.
            GeneracionSolver::OpcionesUI ui;
            ui.numDias = 5;
            ui.numSlotsDia = totalSlots;
            ui.usarAulasFijas = true;

            const auto cursos = nucleo.listarCursos();
            QVERIFY2(cursos.ok, qPrintable(cursos.mensajeError));
            QCOMPARE(cursos.valor.size(), 24);
            int matriculaTotal = 0;
            for (const CursoDTO& curso : cursos.valor) {
                const seed::CursoDef* delPerfil =
                    cursoDePerfil(perfil, curso.anio, curso.seccion);
                QVERIFY2(delPerfil != nullptr,
                         qPrintable(nombreCurso(curso.anio, curso.seccion)));
                // La base guarda el turno del centro; el perfil aporta plan y matrícula.
                QCOMPARE(curso.turno, delPerfil->turno);

                GeneracionSolver::ParametrosCurso parametros;
                parametros.plan = delPerfil->plan;
                parametros.turno = delPerfil->turno;
                parametros.numEstudiantes = delPerfil->matricula;
                ui.cursos.insert(nombreCurso(curso.anio, curso.seccion), parametros);
                matriculaTotal += delPerfil->matricula;
            }
            QCOMPARE(ui.cursos.size(), 24);
            QCOMPARE(matriculaTotal, 889);

            // Composición: sin avisos y con las 0 preferencias del conjunto.
            const GeneracionSolver::Composicion comp =
                GeneracionSolver::construirEntrada(nucleo, ui);
            QVERIFY2(comp.avisos.isEmpty(),
                     qPrintable(comp.avisos.join(QStringLiteral("; "))));
            QVERIFY2(!comp.entrada.isEmpty(), "la composición debe producir una entrada");
            QVERIFY(comp.preferenciasNoAplicadas.isEmpty());

            // Validación completa del contrato (V1–V19): estructura, tipos, turnos, planes,
            // disponibilidad, candidatos elegibles, franjas útiles, aulas y preferencias.
            const Resultado<SolverConfig> validacion = validarEntradaSolver(comp.entrada);
            QVERIFY2(validacion.ok, qPrintable(validacion.mensajeError));
            const SolverConfig& config = validacion.valor;

            QCOMPARE(config.version, QStringLiteral("2.0"));
            QCOMPARE(config.dimensiones.num_dias, 5);
            QCOMPARE(config.dimensiones.num_slots_dia, 16);
            QCOMPARE(config.cursos.size(), 24);
            QCOMPARE(config.profesores.size(), 32);
            QCOMPARE(config.planes.size(), 2);
            QCOMPARE(config.materias.size(), 85);
            QCOMPARE(config.aulas.size(), 13);
            QCOMPARE(config.franja_horaria.duracion_minutos, 40);
            QCOMPARE(config.turnos.turnos.size(), 2);
            for (const TurnoConfig& turno : config.turnos.turnos) {
                QCOMPARE(turno.slot.size(), 8);
            }
            QCOMPARE(config.eventos.size(), 3);

            // Cada curso conserva plan, turno y matrícula (V7, V9, V10, V11); su aula es de
            // tipo general (V18).
            for (const CursoSolverConfig& curso : config.cursos) {
                const seed::CursoDef* delPerfil =
                    cursoDePerfil(perfil, curso.anio, curso.seccion);
                QVERIFY2(delPerfil != nullptr,
                         qPrintable(nombreCurso(curso.anio, curso.seccion)));
                QVERIFY2(!curso.plan.isEmpty(), qPrintable(nombreCurso(curso.anio, curso.seccion)));
                QCOMPARE(curso.plan, delPerfil->plan);
                QCOMPARE(curso.turno, delPerfil->turno);
                QCOMPARE(curso.num_estudiantes, delPerfil->matricula);
                QVERIFY(curso.aula >= 0);
                QVERIFY(curso.aula < config.aulas.size());
                QCOMPARE(config.aulas[curso.aula].tipo, QStringLiteral("Aula de estudio"));
            }

            // Ida y vuelta del preset: el mismo JSON guardado y recargado con el cargador de
            // «Cargar preset…» conserva los 24 cursos, 32 docentes y la hora de 40 min.
            const QString rutaPreset = dir.filePath(QStringLiteral("preset.json"));
            {
                QFile archivo(rutaPreset);
                QVERIFY2(archivo.open(QIODevice::WriteOnly | QIODevice::Truncate),
                         qPrintable(archivo.errorString()));
                archivo.write(QJsonDocument(comp.entrada).toJson(QJsonDocument::Indented));
                archivo.close();
            }
            const Resultado<SolverConfig> recargado =
                CargadorConfiguracionSolver::cargar(rutaPreset);
            QVERIFY2(recargado.ok, qPrintable(recargado.mensajeError));
            QCOMPARE(recargado.valor.cursos.size(), 24);
            QCOMPARE(recargado.valor.profesores.size(), 32);
            QCOMPARE(recargado.valor.franja_horaria.duracion_minutos, 40);
            for (const CursoSolverConfig& curso : recargado.valor.cursos) {
                const seed::CursoDef* delPerfil =
                    cursoDePerfil(perfil, curso.anio, curso.seccion);
                QVERIFY2(delPerfil != nullptr,
                         qPrintable(nombreCurso(curso.anio, curso.seccion)));
                QCOMPARE(curso.plan, delPerfil->plan);
                QCOMPARE(curso.turno, delPerfil->turno);
                QCOMPARE(curso.num_estudiantes, delPerfil->matricula);
            }

            // El informe declara el conjunto como no generable, con la causa y la acción
            // (RF-9): 48 clases de Educación Física por turno frente a 40 franjas de cancha.
            const QString resumen = seed::resumenTexto(perfil, sembrado.recuentos);
            QVERIFY2(resumen.contains(QStringLiteral("Generable: no — ")), qPrintable(resumen));
            QVERIFY2(resumen.contains(QStringLiteral("48 clases de Educación Física")),
                     qPrintable(resumen));
            QVERIFY2(resumen.contains(QStringLiteral("40 franjas de cancha")), qPrintable(resumen));
            QVERIFY2(resumen.contains(QStringLiteral("Clases: 876 (EF 96; 48 por turno)")),
                     qPrintable(resumen));
            QVERIFY2(resumen.contains(QStringLiteral("Matrícula: 889 (579 + 310)")),
                     qPrintable(resumen));

            // No se resuelve: esta prueba termina en la validación del contrato. La
            // generación real se exigirá al cerrar el aforo simultáneo (RNF-2).
        }
        QSqlDatabase::removeDatabase(conexion);
    }
};

QTEST_GUILESS_MAIN(TestEntradaSeed)

#include "test_entrada_seed.moc"
