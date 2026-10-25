#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>

#include <backend/services/CargadorConfiguracionSolver.hpp>
#include <backend/solver/config/solver_config.hpp>

/// Tests del cargador de configuración del solver y presets como JSON (RF-1, RF-2, RF-4).
/// Solo se acepta el contrato 2.0: una configuración 1.0, sin versión o con versión no reconocida
/// se rechaza identificando la versión (y el archivo, si es un preset en disco). No toca la base de
/// datos.
class TestCargadorConfiguracion : public QObject
{
    Q_OBJECT

private:
    /// Acción disponible que el cargador ofrece al rechazar por versión (RF-2).
    static QString accionDisponible()
    {
        return QStringLiteral("Genere una configuración 2.0 desde los datos");
    }

    /// Configuración válida mínima 2.0 (1 curso, 1 profesor, 1 materia, 1 aula, 1 evento).
    QJsonObject configValida() const
    {
        QJsonObject o;
        o["version"]   = QStringLiteral("2.0");
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

        // La ventana del turno (07:00-12:00) deja 6 franjas de 45 min al descontar el evento.
        QJsonObject turno;
        turno["nombre"] = QStringLiteral("manana");
        turno["slot"]   = QJsonArray{0, 1, 2, 3, 4, 5};
        turno["inicio"] = QStringLiteral("07:00");
        turno["fin"]    = QStringLiteral("12:00");
        QJsonObject turnos;
        turnos["turnos"] = QJsonArray{turno};
        o["turnos"] = turnos;

        // Evento del turno: después de la franja 2, dentro de su ventana (08:30-09:45).
        QJsonObject evento;
        evento["nombre"]          = QStringLiteral("Receso");
        evento["turno"]           = QStringLiteral("manana");
        evento["despues_de_slot"] = 2;
        evento["orden"]           = 1;
        evento["duracion"]        = 30;
        evento["inicio"]          = QStringLiteral("09:15");
        evento["fin"]             = QStringLiteral("09:45");
        o["eventos"] = QJsonArray{evento};

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
        // Con disponibilidad real: V13 exige que el candidato tenga bloques.
        QJsonObject disp; disp["dia"] = 0; disp["slot"] = QJsonArray{0, 1, 2, 3, 4, 5};
        profesor["disponibilidad"]     = QJsonArray{disp};
        o["profesores"] = QJsonArray{profesor};

        // Tipos declarados y coherentes entre materia y aula (V16/V17).
        QJsonObject materia;
        materia["nombre"] = QStringLiteral("Matemática");
        materia["tipo"]   = QStringLiteral("Aula de estudio");
        o["materias"] = QJsonArray{materia};

        QJsonObject aula;
        aula["nombre"]    = QStringLiteral("A1");
        aula["tipo"]      = QStringLiteral("Aula de estudio");
        aula["capacidad"] = 30;
        o["aulas"] = QJsonArray{aula};

        return o;
    }

    /// Escribe `contenido` en `ruta` (para probar el cargador contra disco).
    static void escribir(const QString& ruta, const QByteArray& contenido)
    {
        QFile archivo(ruta);
        QVERIFY(archivo.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(archivo.write(contenido), qint64(contenido.size()));
    }

    static QByteArray leerBytes(const QString& ruta)
    {
        QFile archivo(ruta);
        if (!archivo.open(QIODevice::ReadOnly))
            return {};
        return archivo.readAll();
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
        escribir(dir.filePath("corrupto.json"), "{ esto no es JSON");

        const auto resultado = CargadorConfiguracionSolver::cargar(dir.filePath("corrupto.json"));
        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }

    /// RF-1: la configuración 2.0 (tipos y eventos incluidos) va y vuelve sin pérdida.
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

        // El JSON serializado NO contiene `nombre` de curso (presentación, spec 004).
        const QJsonObject serializado = config.valor.toJson();
        QVERIFY(!serializado.contains(QStringLiteral("nombre")));

        // Se reproduce la configuración: mismas secciones y valores.
        QCOMPARE(leido.valor.toJson(), config.valor.toJson());
        QCOMPARE(leido.valor.version, QStringLiteral("2.0"));
    }

    /// RF-1: los tipos y los eventos declarados se conservan tal cual (sin valores por defecto).
    void guardarYcargar_conservaTiposYeventos()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("preset.json");

        const auto config = SolverConfig::fromJson(configValida());
        QVERIFY2(config.ok, qPrintable(config.mensajeError));
        QVERIFY(CargadorConfiguracionSolver::guardar(ruta, config.valor).ok);

        const auto leido = CargadorConfiguracionSolver::cargar(ruta);
        QVERIFY2(leido.ok, qPrintable(leido.mensajeError));

        const QJsonObject serializado = leido.valor.toJson();
        QCOMPARE(serializado["materias"].toArray().at(0).toObject()["tipo"].toString(),
                 QStringLiteral("Aula de estudio"));
        QCOMPARE(serializado["aulas"].toArray().at(0).toObject()["tipo"].toString(),
                 QStringLiteral("Aula de estudio"));

        const QJsonArray eventos = serializado["eventos"].toArray();
        QCOMPARE(eventos.size(), 1);
        const QJsonObject evento = eventos.at(0).toObject();
        QCOMPARE(evento["nombre"].toString(), QStringLiteral("Receso"));
        QCOMPARE(evento["turno"].toString(), QStringLiteral("manana"));
        QCOMPARE(evento["despues_de_slot"].toInt(), 2);
        QCOMPARE(evento["orden"].toInt(), 1);
        QCOMPARE(evento["duracion"].toInt(), 30);
        QCOMPARE(evento["inicio"].toString(), QStringLiteral("09:15"));
        QCOMPARE(evento["fin"].toString(), QStringLiteral("09:45"));
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

    /// RF-2: la versión 1.0 se rechaza identificando la versión declarada y la esperada.
    void version_1_0_seRechaza()
    {
        QJsonObject o = configValida();
        o[QStringLiteral("version")] = QStringLiteral("1.0");

        const auto config = SolverConfig::fromJson(o);
        QVERIFY2(!config.ok, "una configuración 1.0 debe rechazarse (RF-2)");
        QVERIFY2(config.mensajeError.contains(QStringLiteral("1.0")),
                 qPrintable(config.mensajeError));
        QVERIFY2(config.mensajeError.contains(QStringLiteral("2.0")),
                 qPrintable(config.mensajeError));
    }

    /// RF-2: sin versión declarada también es entrada inválida.
    void versionAusente_seRechaza()
    {
        QJsonObject o = configValida();
        o.remove(QStringLiteral("version"));

        const auto config = SolverConfig::fromJson(o);
        QVERIFY(!config.ok);
        QVERIFY2(config.mensajeError.contains(QStringLiteral("versión"), Qt::CaseInsensitive),
                 qPrintable(config.mensajeError));
    }

    /// RF-2: una versión no reconocida se rechaza identificando la versión encontrada.
    void versionNoReconocida_seRechaza()
    {
        QJsonObject o = configValida();
        o[QStringLiteral("version")] = QStringLiteral("9.9");

        const auto config = SolverConfig::fromJson(o);
        QVERIFY2(!config.ok, "una versión no reconocida debe rechazarse (RF-2)");
        QVERIFY2(config.mensajeError.contains(QStringLiteral("9.9")),
                 qPrintable(config.mensajeError));
        QVERIFY2(config.mensajeError.contains(QStringLiteral("2.0")),
                 qPrintable(config.mensajeError));
    }

    /// RF-2: el preset 1.0 versionado se rechaza nombrando el archivo, la versión y la acción.
    /// El rechazo es por versión (no por secciones) y el archivo queda intacto.
    void preset1_0_seRechazaConArchivoVersionYaccion()
    {
        const QString ruta = QFINDTESTDATA("../data/preset-1.0.json");
        QVERIFY2(!ruta.isEmpty(),
                 "no se encontró el fixture versionado test/data/preset-1.0.json");

        const QByteArray antes = leerBytes(ruta);
        QVERIFY(!antes.isEmpty());

        const auto resultado = CargadorConfiguracionSolver::cargar(ruta);
        QVERIFY(!resultado.ok);

        const QString mensaje = resultado.mensajeError;
        QVERIFY2(mensaje.contains(ruta), qPrintable(mensaje));                       // el archivo
        QVERIFY2(mensaje.contains(QStringLiteral("1.0")), qPrintable(mensaje));      // la versión
        QVERIFY2(mensaje.contains(accionDisponible()), qPrintable(mensaje));         // la acción

        // La versión manda: no se reprocha la sección de eventos/recesos ni se reinterpreta el
        // preset 1.0.
        QVERIFY2(!mensaje.contains(QStringLiteral("eventos")), qPrintable(mensaje));
        QVERIFY2(!mensaje.contains(QStringLiteral("recesos")), qPrintable(mensaje));

        // El preset no se modifica: mismos bytes (hash) antes y después de la carga.
        const QByteArray despues = leerBytes(ruta);
        QCOMPARE(despues.size(), antes.size());
        QCOMPARE(QCryptographicHash::hash(despues, QCryptographicHash::Sha256),
                 QCryptographicHash::hash(antes, QCryptographicHash::Sha256));
    }

    /// RF-2: un archivo sin versión se rechaza nombrando el archivo y la acción disponible.
    void presetSinVersion_seRechazaConArchivoYaccion()
    {
        QJsonObject o = configValida();
        o.remove(QStringLiteral("version"));

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("sin-version.json");
        escribir(ruta, QJsonDocument(o).toJson());

        const auto resultado = CargadorConfiguracionSolver::cargar(ruta);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(ruta), qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("no declara versión")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(accionDisponible()),
                 qPrintable(resultado.mensajeError));
    }

    /// RF-2: un 2.0 con otro error no se presenta como rechazo de versión.
    void config2_0ConErrorDeSeccion_noApuntaAVersion()
    {
        QJsonObject o = configValida();
        o.remove(QStringLiteral("materias"));

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath("sin-materias.json");
        escribir(ruta, QJsonDocument(o).toJson());

        const auto resultado = CargadorConfiguracionSolver::cargar(ruta);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("materias")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(!resultado.mensajeError.contains(accionDisponible()),
                 qPrintable(resultado.mensajeError));
    }

    /// Al PRODUCIR una configuracion (construida en C++ sin declarar `version`) se sella "2.0".
    void configSinVersionDeclarada_seSellaA_2_0()
    {
        // Configuracion valida construida en C++ a la que el origen NO declara version.
        const auto parseada = SolverConfig::fromJson(configValida());
        QVERIFY(parseada.ok);
        SolverConfig config = parseada.valor;
        config.version.clear();

        // (i) al serializar sale "2.0", nunca vacio ni 1.0.
        QCOMPARE(config.toJson()[QStringLiteral("version")].toString(), QStringLiteral("2.0"));

        // (ii) guardar y recargar sigue siendo valida.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ruta = dir.filePath(QStringLiteral("preset.json"));
        const auto guardado = CargadorConfiguracionSolver::guardar(ruta, config);
        QVERIFY2(guardado.ok, qPrintable(guardado.mensajeError));

        const auto leido = CargadorConfiguracionSolver::cargar(ruta);
        QVERIFY2(leido.ok, qPrintable(leido.mensajeError));
        QCOMPARE(leido.valor.version, QStringLiteral("2.0"));
    }

    /// Una configuracion por defecto (recien construida en C++) ya declara la version 2.0.
    void configPorDefectoYaDeclaraVersion_2_0()
    {
        const SolverConfig config;
        QCOMPARE(config.version, QStringLiteral("2.0"));
        QCOMPARE(config.toJson()[QStringLiteral("version")].toString(), QStringLiteral("2.0"));
    }

    /// RF-2: serializar no completa tipos ausentes con un valor por defecto.
    void toJson_noCompletaTiposPorDefecto()
    {
        const auto parseada = SolverConfig::fromJson(configValida());
        QVERIFY(parseada.ok);
        SolverConfig config = parseada.valor;
        config.materias[0].tipo.clear();
        config.aulas[0].tipo.clear();

        const QJsonObject json = config.toJson();
        QCOMPARE(json["materias"].toArray().at(0).toObject()["tipo"].toString(), QString());
        QCOMPARE(json["aulas"].toArray().at(0).toObject()["tipo"].toString(), QString());
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
