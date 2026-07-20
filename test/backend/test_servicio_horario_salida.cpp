#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <backend/services/ServicioHorarioSalida.hpp>
#include <backend/data/output_schedule.hpp>

class TestServicioHorarioSalida : public QObject {
    Q_OBJECT

private slots:
    void init() {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());
        m_servicio.reset(new ServicioHorarioSalida(m_tempDir->path()));
    }

    void cleanup() {
        m_servicio.reset();
        m_tempDir.reset();
    }

    // ─── Helpers ────────────────────────────────────────────────────────────

    /** Crea un archivo JSON de ejemplo en el directorio temporal. */
    QString crearArchivoSalida(const QString& nombre, const QJsonObject& contenido) {
        QString ruta = m_tempDir->filePath(nombre);
        QFile archivo(ruta);
        archivo.open(QIODevice::WriteOnly);
        QJsonDocument doc(contenido);
        archivo.write(doc.toJson());
        archivo.close();
        return nombre;
    }

    QJsonObject crearJsonSalidaSimple() {
        QJsonObject metadata;
        metadata["fecha_generacion"] = "2026-07-20";
        metadata["configuracion"] = "default";
        metadata["tiempo_ejecucion_ms"] = 100;
        metadata["total_asignaciones"] = 2;
        metadata["cursos_generados"] = 1;
        metadata["profesores_asignados"] = 1;
        metadata["conflictos"] = 0;

        QJsonObject asignacion1;
        asignacion1["slot"] = 0;
        asignacion1["materia"] = 1;
        asignacion1["profesor"] = 2;
        asignacion1["aula"] = 3;

        QJsonObject asignacion2;
        asignacion2["slot"] = 1;
        asignacion2["materia"] = 4;
        asignacion2["profesor"] = 5;
        asignacion2["aula"] = 6;

        QJsonArray arrAsignaciones;
        arrAsignaciones.append(asignacion1);
        arrAsignaciones.append(asignacion2);

        QJsonObject dia;
        dia["dia"] = 0;
        dia["asignaciones"] = arrAsignaciones;

        QJsonArray arrDias;
        arrDias.append(dia);

        QJsonObject curso;
        curso["turno"] = "manana";
        curso["dias"] = arrDias;

        QJsonObject horarios;
        horarios["1A"] = curso;

        QJsonObject raiz;
        raiz["metadata"] = metadata;
        raiz["horarios"] = horarios;
        return raiz;
    }

    HorarioSalida crearHorarioEnMemoria() {
        HorarioSalida h;
        h.metadata.fecha_generacion = "2026-07-20";
        h.metadata.total_asignaciones = 2;

        CursoOutput c;
        c.turno = "manana";
        DiaOutput d;
        d.dia = 0;
        d.asignaciones.append({0, 1, 2, 3});
        d.asignaciones.append({1, 4, 5, 6});
        c.dias.append(d);
        h.horarios["1A"] = c;
        return h;
    }

    // ─── Tests: listarArchivos ──────────────────────────────────────────────

    void listarArchivos_directorioVacio() {
        auto archivos = m_servicio->listarArchivos();
        QVERIFY(archivos.isEmpty());
    }

    void listarArchivos_conArchivosJson() {
        crearArchivoSalida("a.json", crearJsonSalidaSimple());
        crearArchivoSalida("b.json", crearJsonSalidaSimple());

        auto archivos = m_servicio->listarArchivos();
        QCOMPARE(archivos.size(), 2);
        QVERIFY(archivos.contains("a.json"));
        QVERIFY(archivos.contains("b.json"));
    }

    void listarArchivos_ignoraNoJson() {
        crearArchivoSalida("a.json", crearJsonSalidaSimple());
        // Crear un archivo .txt
        QFile txt(m_tempDir->filePath("b.txt"));
        txt.open(QIODevice::WriteOnly);
        txt.write("hello");
        txt.close();

        auto archivos = m_servicio->listarArchivos();
        QCOMPARE(archivos.size(), 1);
        QVERIFY(archivos.contains("a.json"));
    }

    void listarArchivos_directorioInexistente() {
        ServicioHorarioSalida svc(m_tempDir->filePath("no_existe"));
        auto archivos = svc.listarArchivos();
        QVERIFY(archivos.isEmpty());
    }

    // ─── Tests: cargarHorario ───────────────────────────────────────────────

    void cargarHorario_archivoExistente() {
        QString nombre = crearArchivoSalida("salida.json", crearJsonSalidaSimple());
        auto resultado = m_servicio->cargarHorario(nombre);

        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.horarios.size(), 1);
        QVERIFY(resultado.valor.horarios.contains("1A"));
        QCOMPARE(resultado.valor.metadata.fecha_generacion, "2026-07-20");
        QCOMPARE(resultado.valor.horarios["1A"].dias[0].asignaciones.size(), 2);
    }

    void cargarHorario_archivoInexistente_falla() {
        auto resultado = m_servicio->cargarHorario("no_existe.json");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró"));
    }

    void cargarHorario_jsonInvalido_falla() {
        QString ruta = m_tempDir->filePath("malo.json");
        QFile archivo(ruta);
        archivo.open(QIODevice::WriteOnly);
        archivo.write("{ json roto ]");
        archivo.close();

        auto resultado = m_servicio->cargarHorario("malo.json");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("parsear JSON"));
    }

    void cargarHorario_camposFaltantes_falla() {
        QJsonObject incompleto;
        incompleto["metadata"] = QJsonObject();

        QString nombre = crearArchivoSalida("incompleto.json", incompleto);
        auto resultado = m_servicio->cargarHorario(nombre);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("Faltan campos"));
    }

    // ─── Tests: guardarHorario ──────────────────────────────────────────────

    void guardarHorario_roundTrip() {
        auto horario = crearHorarioEnMemoria();
        auto guardado = m_servicio->guardarHorario(horario, "guardado.json");
        QVERIFY(guardado.ok);

        auto cargado = m_servicio->cargarHorario("guardado.json");
        QVERIFY(cargado.ok);
        QCOMPARE(cargado.valor.horarios.size(), 1);
        QVERIFY(cargado.valor.horarios.contains("1A"));
        QCOMPARE(cargado.valor.metadata.fecha_generacion, "2026-07-20");
        QCOMPARE(cargado.valor.horarios["1A"].turno, "manana");
        QCOMPARE(cargado.valor.horarios["1A"].dias[0].asignaciones.size(), 2);
    }

    void guardarHorario_directorioNoExiste() {
        ServicioHorarioSalida svc(m_tempDir->filePath("subdir/nueva"));
        auto horario = crearHorarioEnMemoria();
        auto resultado = svc.guardarHorario(horario, "test.json");
        QVERIFY(resultado.ok);
        // Verificar que se creó y se puede leer
        auto cargado = svc.cargarHorario("test.json");
        QVERIFY(cargado.ok);
    }

    // ─── Tests: agregarAsignacion ───────────────────────────────────────────

    void agregarAsignacion_exitoso() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        auto cargado = m_servicio->cargarHorario("h.json");
        QVERIFY(cargado.ok);

        AsignacionOutput nueva{2, 7, 8, 9};
        auto resultado = m_servicio->agregarAsignacion("1A", 0, nueva);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.slot, 2);
        QCOMPARE(resultado.valor.materia, 7);

        // Verificar que quedó guardada
        auto curso = m_servicio->obtenerCurso("1A");
        QVERIFY(curso.ok);
        QCOMPARE(curso.valor.dias[0].asignaciones.size(), 3);
    }

    void agregarAsignacion_cursoInexistente_falla() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        AsignacionOutput nueva{0, 1, 2, 3};
        auto resultado = m_servicio->agregarAsignacion("99", 0, nueva);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró el curso"));
    }

    void agregarAsignacion_slotDuplicado_falla() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        // Slot 0 ya existe en día 0
        AsignacionOutput duplicada{0, 99, 99, 99};
        auto resultado = m_servicio->agregarAsignacion("1A", 0, duplicada);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("Ya existe"));
    }

    void agregarAsignacion_nuevoDia() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        // Agregar en día 1 (no existe aún)
        AsignacionOutput nueva{0, 10, 11, 12};
        auto resultado = m_servicio->agregarAsignacion("1A", 1, nueva);
        QVERIFY(resultado.ok);

        auto curso = m_servicio->obtenerCurso("1A");
        QVERIFY(curso.ok);
        QCOMPARE(curso.valor.dias.size(), 2);
    }

    // ─── Tests: eliminarAsignacion ──────────────────────────────────────────

    void eliminarAsignacion_exitoso() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        auto eliminada = m_servicio->eliminarAsignacion("1A", 0, 0);
        QVERIFY(eliminada.ok);

        auto curso = m_servicio->obtenerCurso("1A");
        QVERIFY(curso.ok);
        QCOMPARE(curso.valor.dias[0].asignaciones.size(), 1);
        QCOMPARE(curso.valor.dias[0].asignaciones[0].slot, 1);
    }

    void eliminarAsignacion_slotInexistente_falla() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        auto resultado = m_servicio->eliminarAsignacion("1A", 0, 99);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró una asignación"));
    }

    void eliminarAsignacion_diaInexistente_falla() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        auto resultado = m_servicio->eliminarAsignacion("1A", 5, 0);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró el día"));
    }

    // ─── Tests: modificarAsignacion ─────────────────────────────────────────

    void modificarAsignacion_exitoso() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        AsignacionOutput nueva{0, 100, 200, 300};
        auto resultado = m_servicio->modificarAsignacion("1A", 0, 0, nueva);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.materia, 100);
        QCOMPARE(resultado.valor.profesor, 200);
        QCOMPARE(resultado.valor.aula, 300);

        // Verificar in-memory
        auto curso = m_servicio->obtenerCurso("1A");
        QVERIFY(curso.ok);
        QCOMPARE(curso.valor.dias[0].asignaciones[0].materia, 100);
    }

    void modificarAsignacion_inexistente_falla() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        AsignacionOutput nueva{99, 99, 99, 99};
        auto resultado = m_servicio->modificarAsignacion("1A", 0, 99, nueva);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró una asignación"));
    }

    // ─── Tests: obtenerCurso / listarCursos ─────────────────────────────────

    void obtenerCurso_exitoso() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        auto resultado = m_servicio->obtenerCurso("1A");
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.turno, "manana");
    }

    void obtenerCurso_inexistente_falla() {
        auto horario = crearHorarioEnMemoria();
        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        auto resultado = m_servicio->obtenerCurso("99");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("No se encontró el curso"));
    }

    void listarCursos_conDatos() {
        auto horario = crearHorarioEnMemoria();
        // Agregar segundo curso
        CursoOutput c2;
        c2.turno = "tarde";
        horario.horarios["2B"] = c2;

        m_servicio->guardarHorario(horario, "h.json");
        m_servicio->cargarHorario("h.json");

        auto resultado = m_servicio->listarCursos();
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.size(), 2);
        QVERIFY(resultado.valor.contains("1A"));
        QVERIFY(resultado.valor.contains("2B"));
    }

    void listarCursos_vacio() {
        auto horario = crearHorarioEnMemoria();
        horario.horarios.clear();
        m_servicio->guardarHorario(horario, "vacio.json");
        m_servicio->cargarHorario("vacio.json");

        auto resultado = m_servicio->listarCursos();
        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.isEmpty());
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<ServicioHorarioSalida> m_servicio;
};

QTEST_MAIN(TestServicioHorarioSalida)
#include "test_servicio_horario_salida.moc"
