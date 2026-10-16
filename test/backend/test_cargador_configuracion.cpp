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
        o["version"]   = QStringLiteral("1.0");
        o["aula_fija"] = true;   // GLOBAL (por defecto true)

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

        // Planes EMBEBIDOS (materias + horas + año).
        QJsonObject materiaPlan;
        materiaPlan["materia_idx"]     = 0;
        materiaPlan["curso"]           = 1;   // año al que aplica
        materiaPlan["horas_semanales"] = 2;
        QJsonObject plan;
        plan["codigo"]   = QStringLiteral("Ciencias");
        plan["materias"] = QJsonArray{materiaPlan};
        o["planes"] = QJsonArray{plan};

        // Curso por IDENTIDAD (sin `nombre`).
        QJsonObject curso;
        curso["anio"]            = 1;
        curso["seccion"]         = QStringLiteral("A");
        curso["turno"]           = QStringLiteral("manana");
        curso["plan"]            = QStringLiteral("Ciencias");
        curso["num_estudiantes"] = 20;
        curso["aula"]            = 0;
        o["cursos"] = QJsonArray{curso};

        QJsonObject profesor;
        profesor["nombre"]             = QStringLiteral("Ana");
        profesor["horas_aula"]         = 2;          // `horas_requeridas` opcional (ausente)
        profesor["turno"]              = QString();
        profesor["plan"]               = QString();
        profesor["anios"]              = QJsonArray{1};
        profesor["materias_asignadas"] = QJsonArray{0};
        profesor["materias_suplente"]  = QJsonArray{};
        profesor["disponibilidad"]     = QJsonArray{};
        o["profesores"] = QJsonArray{profesor};

        QJsonObject materia;
        materia["nombre"] = QStringLiteral("Matemática");
        o["materias"] = QJsonArray{materia};

        QJsonObject aula;
        aula["nombre"]    = QStringLiteral("A1");
        aula["tipo"]      = QStringLiteral("regular");
        aula["capacidad"] = 30;
        o["aulas"] = QJsonArray{aula};

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

        // El JSON serializado NO contiene `nombre` (el nombre es presentación, spec 004).
        const QJsonObject serializado = config.valor.toJson();
        QVERIFY(!serializado.contains(QStringLiteral("nombre")));

        // Se reproduce la configuración: mismas secciones y valores.
        QCOMPARE(leido.valor.toJson(), config.valor.toJson());
    }

    void camposDesconocidos_seToleran()
    {
        QJsonObject o = configValida();
        o[QStringLiteral("campo_extra")] = QStringLiteral("se ignora");
        QJsonObject cursoExtra = o["cursos"].toArray()[0].toObject();
        cursoExtra[QStringLiteral("nombre")] = QStringLiteral("1ro A"); // nombre se ignora
        o["cursos"] = QJsonArray{cursoExtra};

        const auto config = SolverConfig::fromJson(o);
        QVERIFY2(config.ok, qPrintable(config.mensajeError));
        // Ni el campo desconocido ni `nombre` reaparecen al serializar.
        QVERIFY(!config.valor.toJson().contains(QStringLiteral("campo_extra")));
        QVERIFY(!config.valor.toJson().contains(QStringLiteral("nombre")));
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
