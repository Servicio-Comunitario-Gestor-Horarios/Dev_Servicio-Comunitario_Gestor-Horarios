#include <QFile>
#include <QJsonArray>
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
#include <backend/solver/config/contrato_solver.hpp>
#include <backend/solver/config/solver_config.hpp>
#include <backend/solver/servicio_solver.hpp>
#include <seed/sembrador.hpp>

#include "backend/data/horario_salida.hpp"
#include "dialogs/schedule_model.hpp"
#include "services/entrada_solver_builder.hpp"

/// Verificación de integración: con los datos de prueba sembrados (Plan 31059,
/// variante holgada) y un reparto plan/turno por curso, la composición del
/// frontend produce una entrada VÁLIDA para el solver, esa entrada se puede
/// guardar y recargar como preset (JSON) y el motor encuentra un horario.
/// Reproduce el flujo de la vista «Generación de horario».
class TestGeneracionSeed : public QObject
{
    Q_OBJECT

    /// Nombre del turno con el inicio más temprano (mañana) y más tardío (tarde).
    static void turnosExtremos(const QVector<TurnoDTO>& turnos, QString& manana, QString& tarde)
    {
        int mejor = -1;
        int peor = -1;
        for (const TurnoDTO& t : turnos) {
            if (!t.inicio.isValid())
                continue;
            const int s = t.inicio.hour() * 3600 + t.inicio.minute() * 60;
            if (mejor < 0 || s < mejor) { mejor = s; manana = t.nombre; }
            if (peor < 0 || s > peor) { peor = s; tarde = t.nombre; }
        }
    }

private slots:
    void componeYResuelveHorarioConDatosSembrados()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("centro.db"));

        seed::OpcionesAplicacion siembra;
        siembra.rutaDb = ruta;
        const seed::ResultadoAplicacion sembrado = seed::aplicar(siembra);
        QVERIFY2(sembrado.ok(), qPrintable(sembrado.detalle));

        const QString conexion = QStringLiteral("test_generacion_seed");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conexion);
            db.setDatabaseName(ruta);
            QVERIFY2(db.open(), qPrintable(db.lastError().text()));
            NucleoDatos nucleo(db);

            // Turnos de la base y duración del bloque derivada (como hace la vista).
            QString manana;
            QString tarde;
            int totalSlots = 0;
            int duracion = 0;
            if (const auto turnos = nucleo.listarTurnos(); turnos.ok) {
                turnosExtremos(turnos.valor, manana, tarde);
                for (const TurnoDTO& t : turnos.valor) {
                    totalSlots += t.numSlots;
                    if (duracion == 0 && t.numSlots > 0 && t.inicio.isValid() && t.fin.isValid()) {
                        int recesoMin = 0;
                        for (const RecesoDTO& r : t.recesos)
                            recesoMin += r.duracion;
                        duracion = (t.inicio.secsTo(t.fin) / 60 - recesoMin) / t.numSlots;
                    }
                }
            }
            QVERIFY(!manana.isEmpty());
            QVERIFY(!tarde.isEmpty());
            QCOMPARE(duracion, 40); // el conjunto siembra franjas de 40 min

            // Reparto plan/turno de ejemplo (guía datos-prueba-frontend §4).
            struct Asignacion { int anio; const char* seccion; const char* plan; bool esManana; };
            const QVector<Asignacion> reparto = {
                {1, "A", "GEN", true},  {1, "B", "EXA", true},  {1, "C", "NAT", true},
                {2, "A", "SOC", true},  {2, "B", "GEN", true},  {2, "C", "EXA", true},
                {3, "A", "NAT", false}, {3, "B", "SOC", false}, {3, "C", "GEN", false},
                {4, "A", "EXA", false}, {4, "B", "NAT", false},
                {5, "A", "SOC", false}, {5, "B", "GEN", false},
            };

            GeneracionSolver::OpcionesUI ui;
            ui.duracionMinutos = duracion;
            ui.numDias = 5;
            ui.numSlotsDia = totalSlots;
            ui.usarAulasFijas = true;
            for (const Asignacion& a : reparto) {
                GeneracionSolver::ParametrosCurso pc;
                pc.plan = QString::fromUtf8(a.plan);
                pc.turno = a.esManana ? manana : tarde;
                pc.numEstudiantes = 30;
                ui.cursos.insert(nombreCurso(a.anio, QString::fromUtf8(a.seccion)), pc);
            }
            QCOMPARE(ui.cursos.size(), 13);

            const GeneracionSolver::Composicion comp =
                GeneracionSolver::construirEntrada(nucleo, ui);
            QVERIFY2(comp.avisos.isEmpty(), qPrintable(comp.avisos.join(QStringLiteral("; "))));

            const Resultado<SolverConfig> validacion = validarEntradaSolver(comp.entrada);
            QVERIFY2(validacion.ok, qPrintable(validacion.mensajeError));

            // Round-trip de preset: la misma entrada, guardada como JSON, debe
            // recargarse con el cargador que usa «Cargar preset…» de la vista.
            const QString rutaPreset = dir.filePath(QStringLiteral("preset.json"));
            {
                QFile archivo(rutaPreset);
                QVERIFY2(archivo.open(QIODevice::WriteOnly | QIODevice::Truncate),
                         qPrintable(archivo.errorString()));
                archivo.write(QJsonDocument(comp.entrada).toJson(QJsonDocument::Indented));
                archivo.close();
            }
            const Resultado<SolverConfig> recargado = CargadorConfiguracionSolver::cargar(rutaPreset);
            QVERIFY2(recargado.ok, qPrintable(recargado.mensajeError));
            QCOMPARE(recargado.valor.cursos.size(), 13);
            QCOMPARE(recargado.valor.profesores.size(), 65);
            QCOMPARE(recargado.valor.franja_horaria.duracion_minutos, 40);
            for (const CursoSolverConfig& curso : recargado.valor.cursos) {
                QVERIFY2(!curso.plan.isEmpty(), "el preset debe conservar el plan de cada curso");
                QVERIFY2(!curso.turno.isEmpty(), "el preset debe conservar el turno de cada curso");
            }

            // Mismo camino que el proceso de cálculo: el backend registra
            // `solver_resolve` sobre `resolverEntradaSolver` (JSON -> HorarioSalida).
            const RespuestaSolver respuesta = resolverEntradaSolver(comp.entrada);
            QVERIFY2(respuesta.ok, qPrintable(respuesta.error));
            QVERIFY2(respuesta.factible, qPrintable(respuesta.error));
            QCOMPARE(respuesta.salida.value(QStringLiteral("horarios")).toObject().size(), 13);

            // El metadato debe contar CLASES asignadas, no variables del modelo:
            // invariante contra la suma de asignaciones realmente extraídas.
            const QJsonObject salida = respuesta.salida;
            const QJsonObject horarios = salida.value(QStringLiteral("horarios")).toObject();
            int asignacionesReales = 0;
            for (auto itCurso = horarios.constBegin(); itCurso != horarios.constEnd(); ++itCurso) {
                const QJsonArray dias =
                    itCurso.value().toObject().value(QStringLiteral("dias")).toArray();
                for (const QJsonValue& dia : dias)
                    asignacionesReales +=
                        dia.toObject().value(QStringLiteral("asignaciones")).toArray().size();
            }
            const int totalMetadato =
                salida.value(QStringLiteral("metadata")).toObject()
                    .value(QStringLiteral("total_asignaciones")).toInt();
            QVERIFY(asignacionesReales > 0);
            QCOMPARE(totalMetadato, asignacionesReales);

            // ── Visualización (RF-1) ─────────────────────────────────────────
            // El documento que consume la sección de horarios declara su rejilla
            // (turnos, franjas y recesos) y conserva TODAS las clases, incluidas
            // las de la tarde (franjas 12+), que antes se descartaban.
            const HorarioSalida horarioSalida = HorarioSalida::fromJson(salida);
            const QJsonObject documento =
                GeneracionSolver::aFormatoVisualizacion(horarioSalida, comp.contexto);

            const QJsonObject rejilla =
                documento.value(QStringLiteral("_cuadricula")).toObject();
            QCOMPARE(rejilla.value(QStringLiteral("num_slots_dia")).toInt(), totalSlots);
            QCOMPARE(rejilla.value(QStringLiteral("duracion_minutos")).toInt(), duracion);
            QCOMPARE(int(rejilla.value(QStringLiteral("turnos")).toArray().size()), 2);
            QVERIFY(int(rejilla.value(QStringLiteral("recesos")).toArray().size()) >= 2);

            int clasesDocumento = 0;
            int clasesTarde = 0;
            const QJsonArray years = documento.value(QStringLiteral("years")).toArray();
            QCOMPARE(int(years.size()), 5);
            for (const QJsonValue& yv : years) {
                const QJsonArray secciones = yv.toObject().value(QStringLiteral("sections")).toArray();
                for (const QJsonValue& sv : secciones) {
                    const QJsonObject seccion = sv.toObject();
                    QVERIFY2(!seccion.value(QStringLiteral("turno")).toString().isEmpty(),
                             "cada curso del documento debe declarar su turno");
                    const QJsonObject schedule = seccion.value(QStringLiteral("schedule")).toObject();
                    for (auto itDia = schedule.constBegin(); itDia != schedule.constEnd(); ++itDia) {
                        const QJsonObject dia = itDia.value().toObject();
                        for (auto itSlot = dia.constBegin(); itSlot != dia.constEnd(); ++itSlot) {
                            if (ScheduleModel::parseBlock(itSlot.value()).isEmpty())
                                continue;
                            ++clasesDocumento;
                            if (itSlot.key().toInt() >= 12)
                                ++clasesTarde;
                        }
                    }
                }
            }
            QCOMPARE(clasesDocumento, asignacionesReales);
            QVERIFY2(clasesTarde > 0, "las franjas de la tarde (12+) deben conservarse");
        }
        QSqlDatabase::removeDatabase(conexion);
    }
};

QTEST_GUILESS_MAIN(TestGeneracionSeed)

#include "test_generacion_seed.moc"
