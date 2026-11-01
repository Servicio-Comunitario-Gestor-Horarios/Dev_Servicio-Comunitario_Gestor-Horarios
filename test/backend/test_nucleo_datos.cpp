#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTest>
#include <QTime>
#include <QTemporaryDir>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/database/version_esquema.hpp>
#include <backend/services/NucleoDatos.hpp>

/// Tests del núcleo de datos: CRUD de dominios y persistencia tras reapertura (RF-2).
class TestNucleoDatos : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<NucleoDatos> m_nucleo;
    QString m_ruta;
    int m_idTipoAula = -1;

    /// Abre una conexión a la base de prueba y construye el núcleo de datos.
    void abrirBase()
    {
        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_ruta));
        QVERIFY(m_dbManager->isInitialized());
        m_nucleo.reset(new NucleoDatos(m_dbManager->database()));
    }

    /// Cierra la conexión para simular el cierre de la aplicación.
    void cerrarBase()
    {
        m_nucleo.reset();
        m_dbManager->close();
        m_dbManager.reset();
    }

    /// Indica si una tabla existe en la base abierta.
    bool tablaExiste(const QString& tabla)
    {
        QSqlQuery query(m_dbManager->database());
        query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:nombre");
        query.bindValue(":nombre", tabla);
        return query.exec() && query.next();
    }

    /// Franja de conveniencia para las preferencias.
    static FranjaHoraria franja(int dia, int hIni, int mIni, int hFin, int mFin)
    {
        FranjaHoraria f;
        f.dia = dia;
        f.inicio = QTime(hIni, mIni);
        f.fin = QTime(hFin, mFin);
        return f;
    }

    /// Número de filas de una tabla (para comprobar hijas borradas en la baja).
    int contarTabla(const QString& tabla) const
    {
        QSqlQuery query(m_dbManager->database());
        if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabla)) || !query.next())
            return -1;
        return query.value(0).toInt();
    }

private slots:

    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());
        m_ruta = m_dir->filePath("nucleo.db");
        abrirBase();

        const auto tipo = m_nucleo->crearTipoAula(QStringLiteral("Aula de estudio"));
        QVERIFY2(tipo.ok, qPrintable(tipo.mensajeError));
        m_idTipoAula = tipo.valor.id;
    }

    void cleanup()
    {
        cerrarBase();
        m_dir.reset();
    }

    // ─── Esquema actual ─────────────────────────────────────────────────────

    void esquemaActual_contieneTablasDeDominio()
    {
        QCOMPARE(VersionEsquema::leerVersionEsquema(m_dbManager->database()),
                 VersionEsquema::VERSION_ESQUEMA_ACTUAL);

        QVERIFY(tablaExiste(QStringLiteral("Cursos")));
        QVERIFY(!tablaExiste(QStringLiteral("Curso_Materia"))); // retirada (RF-5)
        QVERIFY(tablaExiste(QStringLiteral("Turnos")));
        QVERIFY(tablaExiste(QStringLiteral("Eventos")));
        QVERIFY(tablaExiste(QStringLiteral("TiposAula")));
        QVERIFY(!tablaExiste(QStringLiteral("Recesos")));       // retirada (RF-9)
    }

    // ─── Contrato de lectura: error vs vacío (RF-3) ─────────────────────────

    void listar_vacio_devuelveExitoConListaVacia()
    {
        const auto aulas = m_nucleo->listarAulas();
        QVERIFY(aulas.ok);
        QVERIFY(aulas.valor.isEmpty());
    }

    void listar_falloDeLectura_devuelveError()
    {
        // Cerrar la conexión fuerza que la siguiente consulta falle.
        m_dbManager->database().close();

        const auto aulas = m_nucleo->listarAulas();
        QVERIFY(!aulas.ok);
        QVERIFY(!aulas.mensajeError.isEmpty());
        QVERIFY(aulas.valor.isEmpty());
    }

    // ─── Docentes ───────────────────────────────────────────────────────────

    void docentes_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com", "555");
        QVERIFY(creado.ok);
        QCOMPARE(m_nucleo->listarDocentes().valor.size(), 1);

        auto actualizado =
            m_nucleo->actualizarDocente("P1", "Ana Gómez", "ana@test.com", "777");
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarDocentes().valor.first().nombre, QStringLiteral("Ana Gómez"));

        // La baja del docente arrastra sus preferencias (mismo camino que `eliminarConCascada`).
        QVERIFY(m_nucleo->agregarPreferencia("P1", franja(1, 8, 0, 10, 0),
                                             SentidoPreferencia::Preferida).ok);
        QCOMPARE(contarTabla(QStringLiteral("Preferencia_Profesor")), 1);

        QVERIFY(m_nucleo->eliminarDocente("P1"));
        QVERIFY(m_nucleo->listarDocentes().valor.isEmpty());
        QCOMPARE(contarTabla(QStringLiteral("Preferencia_Profesor")), 0);
    }

    // ─── Aulas ──────────────────────────────────────────────────────────────

    void aulas_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearAula("Aula 101", 30, "Edificio A", "Piso 1",
                                          m_idTipoAula);
        QVERIFY(creado.ok);
        QCOMPARE(creado.valor.idTipoAula, m_idTipoAula);
        QCOMPARE(creado.valor.tipoAula, QStringLiteral("Aula de estudio"));
        QCOMPARE(m_nucleo->listarAulas().valor.size(), 1);

        auto actualizado = m_nucleo->actualizarAula(creado.valor.id, "Aula 101", 45,
                                                    "Edificio B", "Piso 2", m_idTipoAula);
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarAulas().valor.first().capacidad, 45);

        QVERIFY(m_nucleo->eliminarAula(creado.valor.id));
        QVERIFY(m_nucleo->listarAulas().valor.isEmpty());
    }

    void aulas_sinTipo_seRechazan()
    {
        // La fachada no rellena el tipo de forma implícita: sin tipo no se crea.
        const auto sinTipo = m_nucleo->crearAula("Aula sin tipo", 30, QString(), QString(), -1);
        QVERIFY(!sinTipo.ok);
        QVERIFY(!sinTipo.mensajeError.isEmpty());

        const auto creada = m_nucleo->crearAula("Aula 102", 30, QString(), QString(),
                                                m_idTipoAula);
        QVERIFY(creada.ok);
        QVERIFY(!m_nucleo->actualizarAula(creada.valor.id, "Aula 102", 40, QString(),
                                          QString(), -1).ok);
        QCOMPARE(m_nucleo->listarAulas().valor.first().idTipoAula, m_idTipoAula);
    }

    // ─── Materias ───────────────────────────────────────────────────────────

    void materias_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearMateria("Matemática", QString(), m_idTipoAula);
        QVERIFY(creado.ok);
        QCOMPARE(creado.valor.idTipoAula, m_idTipoAula);
        QCOMPARE(creado.valor.tipoAula, QStringLiteral("Aula de estudio"));
        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 1);

        auto actualizado = m_nucleo->actualizarMateria(creado.valor.id, "Matemática II",
                                                       QString(), m_idTipoAula);
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarMaterias().valor.first().nombre, QStringLiteral("Matemática II"));

        QVERIFY(m_nucleo->eliminarMateria(creado.valor.id));
        QVERIFY(m_nucleo->listarMaterias().valor.isEmpty());
    }

    void materias_sinTipo_seRechazan()
    {
        QVERIFY(!m_nucleo->crearMateria("Materia sin tipo", QString(), -1).ok);

        const auto creada = m_nucleo->crearMateria("Física", QString(), m_idTipoAula);
        QVERIFY(creada.ok);
        QVERIFY(!m_nucleo->actualizarMateria(creada.valor.id, "Física", QString(), -1).ok);
        QCOMPARE(m_nucleo->listarMaterias().valor.first().idTipoAula, m_idTipoAula);
    }

    // ─── Planes de estudio ──────────────────────────────────────────────────

    void planes_altaModificacionBaja()
    {
        auto creado = m_nucleo->crearPlan("IS-2026", "Ingeniería de Sistemas");
        QVERIFY(creado.ok);
        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 1);

        auto actualizado =
            m_nucleo->actualizarPlan("IS-2026", "Ingeniería de Sistemas v2", "Descripción");
        QVERIFY(actualizado.ok);
        QCOMPARE(m_nucleo->listarPlanes().valor.first().nombre,
                 QStringLiteral("Ingeniería de Sistemas v2"));

        QVERIFY(m_nucleo->eliminarPlan("IS-2026"));
        QVERIFY(m_nucleo->listarPlanes().valor.isEmpty());
    }

    // ─── Cursos ─────────────────────────────────────────────────────────────

    void cursos_altaModificacionBaja()
    {
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        auto aula = m_nucleo->crearAula("Aula 1", 30, QString(), QString(), m_idTipoAula);
        QVERIFY(aula.ok);

        auto creado = m_nucleo->crearCurso(1, "A", "Mañana", aula.valor.id);
        QVERIFY(creado.ok);
        QCOMPARE(creado.valor.anio, 1);
        QCOMPARE(creado.valor.seccion, QStringLiteral("A"));
        QCOMPARE(creado.valor.turno, QStringLiteral("Mañana"));
        QCOMPARE(creado.valor.aula, aula.valor.id);

        auto lista = m_nucleo->listarCursos().valor;
        QCOMPARE(lista.size(), 1);
        QCOMPARE(lista.first().anio, 1);
        QCOMPARE(lista.first().seccion, QStringLiteral("A"));

        auto actualizado =
            m_nucleo->actualizarCurso(creado.valor.id, 1, "B", "Mañana", aula.valor.id);
        QVERIFY(actualizado.ok);
        QCOMPARE(actualizado.valor.seccion, QStringLiteral("B"));

        QVERIFY(m_nucleo->eliminarCurso(creado.valor.id));
        QVERIFY(m_nucleo->listarCursos().valor.isEmpty());
    }

    void cursos_validacionIdentidadYAula()
    {
        auto aula = m_nucleo->crearAula("Aula 1", 30, QString(), QString(), m_idTipoAula);
        QVERIFY(aula.ok);

        // Alta valida.
        QVERIFY(m_nucleo->crearCurso(1, "A").ok);

        // Duplicado año+sección (normaliza mayúsculas/espacios): mensaje con campo y valor.
        auto dup = m_nucleo->crearCurso(1, "a");
        QVERIFY(!dup.ok);
        QVERIFY(dup.mensajeError.contains(QStringLiteral("1")));
        QVERIFY(dup.mensajeError.contains(QStringLiteral("A")));

        // Año invalido.
        QVERIFY(!m_nucleo->crearCurso(0, "B").ok);
        QVERIFY(!m_nucleo->crearCurso(-1, "B").ok);

        // Seccion invalida (vacia, solo espacios, dos caracteres).
        QVERIFY(!m_nucleo->crearCurso(2, QString()).ok);
        QVERIFY(!m_nucleo->crearCurso(2, QStringLiteral("   ")).ok);
        QVERIFY(!m_nucleo->crearCurso(2, QStringLiteral("AB")).ok);

        // Seccion no ASCII valida, normalizada a mayusculas.
        auto noAscii = m_nucleo->crearCurso(2, QString::fromUtf8("ñ"));
        QVERIFY(noAscii.ok);
        QCOMPARE(noAscii.valor.seccion, QString::fromUtf8("Ñ"));

        // Aula inexistente se rechaza.
        QVERIFY(!m_nucleo->crearCurso(3, "A", QString(), 99999).ok);

        // Aula existente y curso sin aula: ambos validos.
        QVERIFY(m_nucleo->crearCurso(3, "A", QString(), aula.valor.id).ok);
        QVERIFY(m_nucleo->crearCurso(3, "B").ok);

        // Actualizar sin cambiar la identidad se acepta.
        auto creado = m_nucleo->crearCurso(4, "A");
        QVERIFY(creado.ok);
        QVERIFY(m_nucleo->actualizarCurso(creado.valor.id, 4, "A", QString(), -1).ok);

        // Actualizar hacia una identidad ya ocupada se rechaza.
        QVERIFY(!m_nucleo->actualizarCurso(creado.valor.id, 1, "A").ok);

        // Actualizar con aula inexistente se rechaza.
        QVERIFY(!m_nucleo->actualizarCurso(creado.valor.id, 4, "A", QString(), 99999).ok);
    }

    // ─── Turnos y eventos ───────────────────────────────────────────────────

    /// Fixture corregida a la rejilla de T4: con 13:00–18:00 y 5 franjas, un
    /// evento de 15 min en la posición 2 con la franja de la hora académica
    /// (35 min): su ventana es 15:10–15:25 (no 15:34–15:49 del reparto antiguo).
    /// El turno se actualiza antes de crear el evento (14:00–19:00 con 6
    /// franjas) y el evento se declara dentro de su ventana real, sin relajar la
    /// validación.
    void turnosEventos_altaModificacionBaja()
    {
        auto turno = m_nucleo->crearTurno("Tarde", QTime(13, 0), QTime(18, 0), 5);
        QVERIFY(turno.ok);
        QCOMPARE(turno.valor.nombre, QStringLiteral("Tarde"));
        QCOMPARE(turno.valor.numSlots, 5);

        auto actualizado = m_nucleo->actualizarTurno("Tarde", QTime(14, 0), QTime(19, 0), 6);
        QVERIFY2(actualizado.ok, qPrintable(actualizado.mensajeError));
        QCOMPARE(actualizado.valor.numSlots, 6);

        auto evento = m_nucleo->agregarEvento("Tarde", "Receso", 2, 15,
                                              QTime(15, 10), QTime(15, 25));
        QVERIFY2(evento.ok, qPrintable(evento.mensajeError));
        QCOMPARE(evento.valor.despuesDeSlot, 2);
        QCOMPARE(evento.valor.duracion, 15);

        auto lista = m_nucleo->listarTurnos().valor;
        QCOMPARE(lista.size(), 1);
        QCOMPARE(lista.first().eventos.size(), 1);
        QCOMPARE(lista.first().eventos.first().nombre, QStringLiteral("Receso"));

        QVERIFY(m_nucleo->eliminarEvento(evento.valor.id));
        QCOMPARE(m_nucleo->listarTurnos().valor.first().eventos.size(), 0);

        QVERIFY(m_nucleo->eliminarTurno("Tarde"));
        QVERIFY(m_nucleo->listarTurnos().valor.isEmpty());
    }

    // ─── Configuración global de la hora académica (011 T3) ──────────────────

    void configuracionHoraria_fachadaLeeYGuardaElValorEfectivo()
    {
        // El esquema deja la fila por defecto: 35 con origen guardado, no por defecto.
        const auto inicial = m_nucleo->configuracionHoraria();
        QVERIFY2(inicial.ok, qPrintable(inicial.mensajeError));
        QCOMPARE(inicial.valor.duracionMinutos, 35);
        QVERIFY(!inicial.valor.porDefecto);

        const auto guardado = m_nucleo->guardarConfiguracionHoraria(40);
        QVERIFY2(guardado.ok, qPrintable(guardado.mensajeError));
        QCOMPARE(guardado.valor.duracionMinutos, 40);

        const auto leido = m_nucleo->configuracionHoraria();
        QVERIFY(leido.ok);
        QCOMPARE(leido.valor.duracionMinutos, 40);
        QVERIFY(!leido.valor.porDefecto);

        // Una edición inválida no cambia el valor anterior.
        const auto rechazado = m_nucleo->guardarConfiguracionHoraria(29);
        QVERIFY(!rechazado.ok);
        QCOMPARE(m_nucleo->configuracionHoraria().valor.duracionMinutos, 40);
    }

    // ─── Preferencias horarias: fachada (RF-3) ──────────────────────────────

    void preferencias_fachadaAltaListadoEdicionBajaYlimpieza()
    {
        QVERIFY(m_nucleo->crearDocente(QStringLiteral("P-PREF"), QStringLiteral("Docente Pref"),
                                       QStringLiteral("pref@centro.edu")).ok);

        auto alta = m_nucleo->agregarPreferencia(QStringLiteral("P-PREF"), franja(1, 8, 0, 10, 0),
                                                 SentidoPreferencia::Preferida);
        QVERIFY2(alta.ok, qPrintable(alta.mensajeError));
        QVERIFY(alta.valor.id > 0);

        auto lista = m_nucleo->listarPreferencias(QStringLiteral("P-PREF"));
        QVERIFY2(lista.ok, qPrintable(lista.mensajeError));
        QCOMPARE(lista.valor.size(), 1);
        QCOMPARE(lista.valor.first().franja.dia, 1);

        auto edicion = m_nucleo->actualizarPreferencia(
            alta.valor.id, franja(2, 9, 0, 11, 0), SentidoPreferencia::Evitada);
        QVERIFY2(edicion.ok, qPrintable(edicion.mensajeError));
        QCOMPARE(edicion.valor.franja.dia, 2);
        QCOMPARE(edicion.valor.sentido, SentidoPreferencia::Evitada);

        auto baja = m_nucleo->eliminarPreferencia(alta.valor.id);
        QVERIFY2(baja.ok, qPrintable(baja.mensajeError));
        QVERIFY(baja.valor);
        QVERIFY(m_nucleo->listarPreferencias(QStringLiteral("P-PREF")).valor.isEmpty());

        QVERIFY(m_nucleo->agregarPreferencia(QStringLiteral("P-PREF"), franja(1, 8, 0, 10, 0),
                                             SentidoPreferencia::Preferida).ok);
        QVERIFY(m_nucleo->agregarPreferencia(QStringLiteral("P-PREF"), franja(3, 8, 0, 10, 0),
                                             SentidoPreferencia::Evitada).ok);
        auto limpieza = m_nucleo->limpiarPreferencias(QStringLiteral("P-PREF"));
        QVERIFY2(limpieza.ok, qPrintable(limpieza.mensajeError));
        QCOMPARE(limpieza.valor, 2);
        QVERIFY(m_nucleo->listarPreferencias(QStringLiteral("P-PREF")).valor.isEmpty());
    }

    /// Una escritura fallida queda pendiente y el reintento la aplica (RF-3).
    void preferencias_escrituraFallidaQuedaPendienteYSeReintenta()
    {
        // Docente inexistente: el alta falla y no se descarta: queda pendiente.
        const auto fallida = m_nucleo->agregarPreferencia(
            QStringLiteral("P-PEND"), franja(1, 8, 0, 10, 0), SentidoPreferencia::Preferida);
        QVERIFY(!fallida.ok);
        QVERIFY(m_nucleo->hayPendientes());

        const auto pendientes = m_nucleo->gestorPendientes().pendientes();
        QCOMPARE(pendientes.size(), 1);
        QCOMPARE(pendientes.first().dominio, QStringLiteral("preferencia"));
        QCOMPARE(pendientes.first().tipo, TipoOperacion::Alta);

        // Se crea el docente y el reintento aplica el cambio; el pendiente desaparece.
        QVERIFY(m_nucleo->crearDocente(QStringLiteral("P-PEND"), QStringLiteral("Docente Pend"),
                                       QStringLiteral("pend@centro.edu")).ok);
        const auto reintento = m_nucleo->reintentarPendiente(pendientes.first().id);
        QVERIFY2(reintento.ok, qPrintable(reintento.mensajeError));
        QVERIFY(reintento.valor);
        QVERIFY(!m_nucleo->hayPendientes());
        QCOMPARE(m_nucleo->listarPreferencias(QStringLiteral("P-PEND")).valor.size(), 1);
    }

    // ─── Catálogo de tipos de aula ──────────────────────────────────────────

    void tiposAula_altaListadoRenombradoBaja()
    {
        QCOMPARE(m_nucleo->listarTiposAula().valor.size(), 1);   // el del init

        const auto creado = m_nucleo->crearTipoAula("Laboratorio");
        QVERIFY(creado.ok);
        QCOMPARE(m_nucleo->listarTiposAula().valor.size(), 2);

        const auto obtenido = m_nucleo->obtenerTipoAula(creado.valor.id);
        QVERIFY(obtenido.ok);
        QCOMPARE(obtenido.valor.nombre, QStringLiteral("Laboratorio"));

        const auto renombrado = m_nucleo->renombrarTipoAula(creado.valor.id, "Taller");
        QVERIFY(renombrado.ok);
        QCOMPARE(m_nucleo->obtenerTipoAula(creado.valor.id).valor.nombre,
                 QStringLiteral("Taller"));

        QVERIFY(m_nucleo->eliminarTipoAula(creado.valor.id).ok);
        QCOMPARE(m_nucleo->listarTiposAula().valor.size(), 1);
    }

    // ─── Eventos por identidad ──────────────────────────────────────────────

    void eventos_altaEdicionBajaPorIdentidad()
    {
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);

        const auto primero = m_nucleo->agregarEvento("Mañana", "Receso", 2, 10);
        const auto segundo = m_nucleo->agregarEvento("Mañana", "Acto cívico", 2, 5);
        QVERIFY(primero.ok);
        QVERIFY(segundo.ok);
        QCOMPARE(primero.valor.orden, 1);
        QCOMPARE(segundo.valor.orden, 2);

        auto eventos = m_nucleo->listarTurnos().valor.first().eventos;
        QCOMPARE(eventos.size(), 2);
        QCOMPARE(eventos[0].id, primero.valor.id);
        QCOMPARE(eventos[1].id, segundo.valor.id);

        const auto editado = m_nucleo->actualizarEvento(segundo.valor.id, "Izada",
                                                        2, 5, QTime(), QTime(), 2);
        QVERIFY(editado.ok);
        QCOMPARE(m_nucleo->listarTurnos().valor.first().eventos[1].nombre,
                 QStringLiteral("Izada"));

        QVERIFY(m_nucleo->eliminarEvento(primero.valor.id));
        eventos = m_nucleo->listarTurnos().valor.first().eventos;
        QCOMPARE(eventos.size(), 1);
        QCOMPARE(eventos.first().orden, 2);   // no se renumera
    }

    // ─── Persistencia tras reapertura ───────────────────────────────────────

    void persistencia_todosLosDominios_trasReapertura()
    {
        QVERIFY(m_nucleo->crearPlan("IS-2026", "Ingeniería de Sistemas").ok);
        QVERIFY(m_nucleo->crearTurno("Mañana", QTime(7, 0), QTime(12, 0), 5).ok);
        QVERIFY(m_nucleo->agregarEvento("Mañana", "Receso", 2, 10).ok);
        QVERIFY(m_nucleo->crearDocente("P1", "Ana Pérez", "ana@test.com").ok);
        QVERIFY(m_nucleo->crearAula("Aula 1", 30, "Edificio A", QString(),
                                    m_idTipoAula).ok);
        auto materia = m_nucleo->crearMateria("Matemática", QString(), m_idTipoAula);
        QVERIFY(materia.ok);
        auto curso = m_nucleo->crearCurso(1, "A", "Mañana", -1);
        QVERIFY(curso.ok);

        // Cerrar y reabrir la base: todo debe seguir ahí.
        cerrarBase();
        abrirBase();

        QCOMPARE(m_nucleo->listarDocentes().valor.size(), 1);
        QCOMPARE(m_nucleo->listarAulas().valor.size(), 1);
        QCOMPARE(m_nucleo->listarMaterias().valor.size(), 1);
        QCOMPARE(m_nucleo->listarPlanes().valor.size(), 1);
        QCOMPARE(m_nucleo->listarCursos().valor.size(), 1);
        QCOMPARE(m_nucleo->listarTurnos().valor.size(), 1);
        QCOMPARE(m_nucleo->listarTurnos().valor.first().eventos.size(), 1);

        auto cursos = m_nucleo->listarCursos().valor;
        QCOMPARE(cursos.first().anio, 1);
        QCOMPARE(cursos.first().seccion, QStringLiteral("A"));

        // Una baja también persiste tras reapertura.
        QVERIFY(m_nucleo->eliminarDocente("P1"));
        cerrarBase();
        abrirBase();
        QVERIFY(m_nucleo->listarDocentes().valor.isEmpty());
    }
};

QTEST_MAIN(TestNucleoDatos)
#include "test_nucleo_datos.moc"
