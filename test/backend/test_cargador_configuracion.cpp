#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <backend/services/CargadorConfiguracionSolver.hpp>
#include <backend/solver/config/solver_config.hpp>

/// Tests del cargador de configuración del solver y presets como JSON (RF-1, RF-4).
/// No debe tocar la base de datos.
class TestCargadorConfiguracion : public QObject
{
    Q_OBJECT

private:
    /// Configuración válida mínima (1 curso, 1 profesor, 1 materia, 1 aula).
    QJsonObject configValida() const
    {
        QJsonObject o;
        o["version"] = QStringLiteral("1.0");

        QJsonObject dim;
        dim["num_profesores"] = 1;
        dim["num_materias"]   = 1;
        dim["num_aulas"]      = 1;
        dim["num_cursos"]     = 1;
        dim["num_dias"]       = 5;
        dim["num_slots_dia"]  = 6;
        o["dimensiones"] = dim;

        QJsonObject franja;
        franja["duracion_minutos"] = 45;
        franja["slots_por_turno"]  = 6;
        o["franja_horaria"] = franja;

        QJsonObject turno;
        turno["nombre"] = QStringLiteral("manana");
        turno["slot"]   = QJsonArray{0, 1, 2, 3, 4, 5};
        QJsonObject turnos;
        turnos["turnos"] = QJsonArray{turno};
        o["turnos"] = turnos;

        o["recesos"] = QJsonArray{};

        QJsonObject curso;
        curso["nombre"]          = QStringLiteral("1ro A");
        curso["turno"]           = QStringLiteral("manana");
        curso["aula_fija"]       = 0;
        curso["num_estudiantes"] = 20;
        curso["plan"]            = QStringLiteral("Ciencias");
        QJsonObject materiaCurso;
        materiaCurso["materia_idx"]     = 0;
        materiaCurso["horas_semanales"] = 2;
        curso["materias"] = QJsonArray{materiaCurso};
        o["cursos"] = QJsonArray{curso};

        QJsonObject profesor;
        profesor["nombre"]              = QStringLiteral("Ana");
        profesor["horas_requeridas"]    = 2;
        profesor["horas_aula"]          = 2;
        profesor["turno"]               = QString();
        profesor["plan"]                = QString();
        profesor["materias_asignadas"]  = QJsonArray{0};
        profesor["materias_suplente"]   = QJsonArray{};
        profesor["disponibilidad"]      = QJsonArray{};
        o["profesores"] = QJsonArray{profesor};

        QJsonObject materia;
        materia["nombre"] = QStringLiteral("Matemática");
        o["materias"] = QJsonArray{materia};

        QJsonObject aula;
        aula["nombre"]    = QStringLiteral("A1");
        aula["tipo"]      = QStringLiteral("regular");
        aula["capacidad"] = 30;
        o["aulas"] = QJsonArray{aula};

        QJsonObject planificacion;
        planificacion["activa"] = false;
        o["planificacion"] = planificacion;

        QJsonObject generacion;
        generacion["cursos_a_generar"] = QJsonArray{0};
        o["generacion"] = generacion;

        QJsonObject penalizaciones;
        penalizaciones["capacidad_aula"]      = 100;
        penalizaciones["emergencia_profesor"] = 50;
        o["penalizaciones"] = penalizaciones;

        return o;
    }

private slots:

    void cargarArchivoInexistente_error()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const auto resultado =
            CargadorConfiguracionSolver::cargar(dir.filePath("no_existe.json"));

        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }

    void cargarArchivoCorrupto_error()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("corrupto.json");

        {
            QFile archivo(ruta);
            QVERIFY(archivo.open(QIODevice::WriteOnly));
            archivo.write("{ esto no es JSON");
        }

        const auto resultado = CargadorConfiguracionSolver::cargar(ruta);
        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }

    void guardarYcargar_roundtrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("preset.json");

        const auto config = SolverConfig::fromJson(configValida());
        QVERIFY2(config.ok, qPrintable(config.mensajeError));

        const auto guardado = CargadorConfiguracionSolver::guardar(ruta, config.valor);
        QVERIFY2(guardado.ok, qPrintable(guardado.mensajeError));
        QVERIFY(QFile::exists(ruta));

        const auto leido = CargadorConfiguracionSolver::cargar(ruta);
        QVERIFY2(leido.ok, qPrintable(leido.mensajeError));

        // Se reproduce la configuración: mismas 13 secciones y valores.
        QCOMPARE(leido.valor.toJson(), config.valor.toJson());
    }

    void guardarCarpetaInexistente_error()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const auto config = SolverConfig::fromJson(configValida());
        QVERIFY(config.ok);

        const auto resultado = CargadorConfiguracionSolver::guardar(
            dir.filePath("no/existe/preset.json"), config.valor);

        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }

    void listarPresets_devuelveJsonOrdenados()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const auto config = SolverConfig::fromJson(configValida());
        QVERIFY(config.ok);
        QVERIFY(CargadorConfiguracionSolver::guardar(dir.filePath("b.json"), config.valor).ok);
        QVERIFY(CargadorConfiguracionSolver::guardar(dir.filePath("a.json"), config.valor).ok);
        {
            QFile otro(dir.filePath("nota.txt"));
            QVERIFY(otro.open(QIODevice::WriteOnly));
            otro.write("x");
        }

        const auto resultado = CargadorConfiguracionSolver::listarPresets(dir.path());

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor, QStringList({QStringLiteral("a.json"), QStringLiteral("b.json")}));
    }

    void listarPresets_directorioInexistente_error()
    {
        const auto resultado =
            CargadorConfiguracionSolver::listarPresets(QStringLiteral("/ruta/que/no/existe"));

        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }
};

QTEST_MAIN(TestCargadorConfiguracion)
#include "test_cargador_configuracion.moc"
