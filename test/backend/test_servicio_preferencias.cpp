#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QTime>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/ServicioGeneracion.hpp>
#include <backend/services/ServicioPreferencias.hpp>

/// Tests del servicio de preferencias horarias del docente (RF-1, RF-2, RF-3,
/// RNF-1): CRUD, orden, duplicados y solapes de sentidos opuestos. Las
/// preferencias solo se persisten: no afectan a la generación.
class TestServicioPreferencias : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<ServicioPreferencias> m_servicio;

    /// Franja de conveniencia: día, hora de inicio y hora de fin.
    static FranjaHoraria franja(int dia, int hIni, int mIni, int hFin, int mFin)
    {
        FranjaHoraria f;
        f.dia = dia;
        f.inicio = QTime(hIni, mIni);
        f.fin = QTime(hFin, mFin);
        return f;
    }

    /// Docente mínimo en la tabla, sin pasar por el servicio de docentes.
    void crearDocente(const QString& id)
    {
        QSqlQuery q(m_dbManager->database());
        q.prepare(QStringLiteral("INSERT INTO Profesores (id, nombre, email) "
                                 "VALUES (:id, :nombre, :email)"));
        q.bindValue(QStringLiteral(":id"), id);
        q.bindValue(QStringLiteral(":nombre"), QStringLiteral("Docente ") + id);
        q.bindValue(QStringLiteral(":email"), id + QStringLiteral("@centro.edu"));
        QVERIFY2(q.exec(), qPrintable(q.lastError().text()));
    }

    /// Franja de disponibilidad del docente (para comprobar que son listas distintas).
    void crearDisponibilidad(const QString& idProfesor, const FranjaHoraria& f)
    {
        QSqlQuery q(m_dbManager->database());
        q.prepare(QStringLiteral("INSERT INTO Disponibilidad_Profesor "
                                 "(id_Profesor, dia, hora_inicio, hora_fin) "
                                 "VALUES (:profesor, :dia, :inicio, :fin)"));
        q.bindValue(QStringLiteral(":profesor"), idProfesor);
        q.bindValue(QStringLiteral(":dia"), QString::number(f.dia));
        q.bindValue(QStringLiteral(":inicio"), f.inicio.toString(QStringLiteral("HH:mm")));
        q.bindValue(QStringLiteral(":fin"), f.fin.toString(QStringLiteral("HH:mm")));
        QVERIFY2(q.exec(), qPrintable(q.lastError().text()));
    }

    int contar(const QString& tabla) const
    {
        QSqlQuery q(m_dbManager->database());
        if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tabla)) || !q.next())
            return -1;
        return q.value(0).toInt();
    }

    /// Comprueba que el mensaje cita los fragmentos esperados (valor y acción).
    static void exigirMensaje(const Resultado<PreferenciaDTO>& resultado,
                              const QStringList& fragmentos)
    {
        QVERIFY(!resultado.ok);
        for (const QString& fragmento : fragmentos)
            QVERIFY2(resultado.mensajeError.contains(fragmento),
                     qPrintable(resultado.mensajeError + QStringLiteral(" | falta: ") + fragmento));
    }

private slots:

    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());

        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_dir->filePath(QStringLiteral("preferencias.db"))));
        QVERIFY(m_dbManager->isInitialized());

        m_servicio.reset(new ServicioPreferencias(m_dbManager->database()));

        crearDocente(QStringLiteral("P1"));
        crearDocente(QStringLiteral("P2"));
    }

    void cleanup()
    {
        m_servicio.reset();
        m_dbManager->close();
        m_dbManager.reset();
        m_dir.reset();
    }

    // ─── agregar ────────────────────────────────────────────────────────────

    void agregar_guardaYDevuelveId()
    {
        auto r = m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                     SentidoPreferencia::Preferida);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QVERIFY(r.valor.id > 0);
        QCOMPARE(r.valor.idProfesor, QStringLiteral("P1"));
        QCOMPARE(r.valor.franja.dia, 1);
        QCOMPARE(r.valor.franja.inicio, QTime(8, 0));
        QCOMPARE(r.valor.franja.fin, QTime(10, 0));
        QCOMPARE(r.valor.sentido, SentidoPreferencia::Preferida);
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 1);
    }

    void agregar_docenteVacio_fallaConValorYAccion()
    {
        exigirMensaje(m_servicio->agregar(QString(), franja(1, 8, 0, 10, 0),
                                          SentidoPreferencia::Preferida),
                      {QStringLiteral("docente"),
                       QStringLiteral("Indique el docente")});
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    void agregar_docenteInexistente_fallaConElIdYAccion()
    {
        exigirMensaje(m_servicio->agregar(QStringLiteral("NO-EXISTE"), franja(1, 8, 0, 10, 0),
                                          SentidoPreferencia::Preferida),
                      {QStringLiteral("NO-EXISTE"), QStringLiteral("Regístrelo")});
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    void agregar_diaFueraDeRango_fallaConValorYAccion()
    {
        for (int dia : {-1, 7}) {
            exigirMensaje(m_servicio->agregar(QStringLiteral("P1"), franja(dia, 8, 0, 10, 0),
                                              SentidoPreferencia::Preferida),
                          {QStringLiteral("se recibió %1").arg(dia),
                           QStringLiteral("Corrija el día")});
        }
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    void agregar_horasInvalidas_fallaConAccion()
    {
        FranjaHoraria sinFin = franja(1, 8, 0, 10, 0);
        sinFin.fin = QTime();

        exigirMensaje(m_servicio->agregar(QStringLiteral("P1"), sinFin,
                                          SentidoPreferencia::Preferida),
                      {QStringLiteral("HH:mm"), QStringLiteral("Corrija el rango")});
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    void agregar_inicioNoAnteriorAlFin_fallaConAccion()
    {
        for (const FranjaHoraria& f : {franja(1, 10, 0, 10, 0), franja(1, 12, 0, 8, 0)}) {
            exigirMensaje(m_servicio->agregar(QStringLiteral("P1"), f,
                                              SentidoPreferencia::Preferida),
                          {QStringLiteral("anterior"), QStringLiteral("Corrija el rango")});
        }
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    void agregar_sentidoInvalido_fallaConValorYAccion()
    {
        exigirMensaje(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                          static_cast<SentidoPreferencia>(99)),
                      {QStringLiteral("99"), QStringLiteral("Corrija el sentido")});
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 0);
    }

    void agregar_duplicadoExacto_fallaConDatosYAccion()
    {
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);

        exigirMensaje(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                          SentidoPreferencia::Preferida),
                      {QStringLiteral("idéntica"), QStringLiteral("P1"),
                       QStringLiteral("08:00"), QStringLiteral("10:00"),
                       QStringLiteral("preferida"),
                       QStringLiteral("Cambie el rango o el sentido")});
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 1);
    }

    void agregar_solapeDeSentidosOpuestos_fallaNombrandoLasDosFranjas()
    {
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);

        exigirMensaje(m_servicio->agregar(QStringLiteral("P1"), franja(1, 9, 0, 11, 0),
                                          SentidoPreferencia::Evitada),
                      {QStringLiteral("08:00–10:00"), QStringLiteral("preferida"),
                       QStringLiteral("09:00–11:00"), QStringLiteral("evitada"),
                       QStringLiteral("P1"),
                       QStringLiteral("Cambie el rango o el sentido")});
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 1);
    }

    void agregar_solapesDelMismoSentido_seAceptan()
    {
        QVERIFY2(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                     SentidoPreferencia::Preferida).ok, "primera");
        auto r = m_servicio->agregar(QStringLiteral("P1"), franja(1, 9, 0, 11, 0),
                                     SentidoPreferencia::Preferida);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 2);
    }

    void agregar_rangosContiguosDeSentidosOpuestos_seAceptan()
    {
        QVERIFY2(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                     SentidoPreferencia::Preferida).ok, "primera");
        auto r = m_servicio->agregar(QStringLiteral("P1"), franja(1, 10, 0, 12, 0),
                                     SentidoPreferencia::Evitada);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 2);
    }

    void agregar_diasFueraDeLunesAViernes_seAceptan()
    {
        QVERIFY2(m_servicio->agregar(QStringLiteral("P1"), franja(0, 8, 0, 10, 0),
                                     SentidoPreferencia::Preferida).ok, "domingo");
        QVERIFY2(m_servicio->agregar(QStringLiteral("P1"), franja(6, 8, 0, 10, 0),
                                     SentidoPreferencia::Preferida).ok, "sábado");
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 2);
    }

    void agregar_preferenciaQueCoincideConDisponibilidad_seAcepta()
    {
        crearDisponibilidad(QStringLiteral("P1"), franja(1, 8, 0, 10, 0));

        auto r = m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                     SentidoPreferencia::Preferida);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 1);
        QCOMPARE(contar(QStringLiteral("Disponibilidad_Profesor")), 1);
    }

    // ─── listar ─────────────────────────────────────────────────────────────

    void listar_ordenaPorDiaInicioFinEId()
    {
        // Se insertan desordenadas; el listado sale por día, inicio y fin (el id es el
        // desempate final, inalcanzable con filas válidas: el duplicado exacto se rechaza).
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(3, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 14, 0, 16, 0),
                                    SentidoPreferencia::Preferida).ok);
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 12, 0),
                                    SentidoPreferencia::Preferida).ok);
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);

        auto r = m_servicio->listar(QStringLiteral("P1"));

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor.size(), 4);
        QCOMPARE(r.valor[0].franja.dia, 1);
        QCOMPARE(r.valor[0].franja.inicio, QTime(8, 0));
        QCOMPARE(r.valor[0].franja.fin, QTime(10, 0));
        QCOMPARE(r.valor[1].franja.inicio, QTime(8, 0));
        QCOMPARE(r.valor[1].franja.fin, QTime(12, 0));
        QCOMPARE(r.valor[2].franja.inicio, QTime(14, 0));
        QCOMPARE(r.valor[3].franja.dia, 3);
    }

    void listar_sinPreferencias_esVacioSinError()
    {
        const auto deDocente = m_servicio->listar(QStringLiteral("P1"));
        QVERIFY2(deDocente.ok, qPrintable(deDocente.mensajeError));
        QVERIFY(deDocente.valor.isEmpty());

        // Un docente inexistente tampoco es error: no tiene preferencias.
        const auto inexistente = m_servicio->listar(QStringLiteral("NO-EXISTE"));
        QVERIFY2(inexistente.ok, qPrintable(inexistente.mensajeError));
        QVERIFY(inexistente.valor.isEmpty());
    }

    void listar_deOtroDocente_noLasDevuelve()
    {
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);

        const auto deP2 = m_servicio->listar(QStringLiteral("P2"));
        QVERIFY(deP2.ok);
        QVERIFY(deP2.valor.isEmpty());
    }

    // ─── actualizar ─────────────────────────────────────────────────────────

    void actualizar_cambiaRangoYSentido()
    {
        auto creada = m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                          SentidoPreferencia::Preferida);
        QVERIFY(creada.ok);

        auto r = m_servicio->actualizar(creada.valor.id, franja(2, 14, 0, 16, 0),
                                        SentidoPreferencia::Evitada);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor.id, creada.valor.id);
        QCOMPARE(r.valor.franja.dia, 2);
        QCOMPARE(r.valor.franja.inicio, QTime(14, 0));
        QCOMPARE(r.valor.sentido, SentidoPreferencia::Evitada);

        const auto lista = m_servicio->listar(QStringLiteral("P1")).valor;
        QCOMPARE(lista.size(), 1);
        QCOMPARE(lista.first().franja.dia, 2);
    }

    void actualizar_solapeOpuesto_rechazaYConservaElAnterior()
    {
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);
        auto evitada = m_servicio->agregar(QStringLiteral("P1"), franja(1, 14, 0, 16, 0),
                                           SentidoPreferencia::Evitada);
        QVERIFY(evitada.ok);

        // Mover la evitada encima de la preferida crea un solape de sentidos opuestos.
        auto r = m_servicio->actualizar(evitada.valor.id, franja(1, 9, 0, 11, 0),
                                        SentidoPreferencia::Evitada);

        exigirMensaje(r, {QStringLiteral("08:00–10:00"), QStringLiteral("09:00–11:00"),
                          QStringLiteral("preferida"), QStringLiteral("evitada"),
                          QStringLiteral("Cambie el rango o el sentido")});

        const auto lista = m_servicio->listar(QStringLiteral("P1")).valor;
        QCOMPARE(lista.size(), 2);
        bool encontrada = false;
        for (const PreferenciaDTO& p : lista) {
            if (p.id == evitada.valor.id) {
                encontrada = true;
                QCOMPARE(p.franja.inicio, QTime(14, 0));
                QCOMPARE(p.franja.fin, QTime(16, 0));
                QCOMPARE(p.sentido, SentidoPreferencia::Evitada);
            }
        }
        QVERIFY(encontrada);
    }

    void actualizar_cambioDeDiaQueCreaSolapeOpuesto_rechaza()
    {
        // En días distintos no hay solape: la comprobación es por día.
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);
        auto evitada = m_servicio->agregar(QStringLiteral("P1"), franja(2, 8, 0, 10, 0),
                                           SentidoPreferencia::Evitada);
        QVERIFY(evitada.ok);

        auto r = m_servicio->actualizar(evitada.valor.id, franja(1, 8, 0, 10, 0),
                                        SentidoPreferencia::Evitada);

        exigirMensaje(r, {QStringLiteral("08:00–10:00"), QStringLiteral("preferida"),
                          QStringLiteral("evitada"), QStringLiteral("Cambie el rango o el sentido")});

        // El día anterior se conserva.
        const auto lista = m_servicio->listar(QStringLiteral("P1")).valor;
        for (const PreferenciaDTO& p : lista)
            if (p.id == evitada.valor.id)
                QCOMPARE(p.franja.dia, 2);
    }

    void actualizar_duplicadoExactoContraOtra_falla()
    {
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);
        auto segunda = m_servicio->agregar(QStringLiteral("P1"), franja(1, 14, 0, 16, 0),
                                           SentidoPreferencia::Preferida);
        QVERIFY(segunda.ok);

        auto r = m_servicio->actualizar(segunda.valor.id, franja(1, 8, 0, 10, 0),
                                        SentidoPreferencia::Preferida);

        exigirMensaje(r, {QStringLiteral("idéntica"), QStringLiteral("08:00"),
                          QStringLiteral("Cambie el rango o el sentido")});
        QCOMPARE(contar(QStringLiteral("Preferencia_Profesor")), 2);
    }

    void actualizar_idInexistente_fallaConValorYAccion()
    {
        exigirMensaje(m_servicio->actualizar(9999, franja(1, 8, 0, 10, 0),
                                             SentidoPreferencia::Preferida),
                      {QStringLiteral("9999"), QStringLiteral("Recargue la lista")});
    }

    void actualizar_idInvalido_fallaConValorYAccion()
    {
        exigirMensaje(m_servicio->actualizar(0, franja(1, 8, 0, 10, 0),
                                             SentidoPreferencia::Preferida),
                      {QStringLiteral("se recibió 0"), QStringLiteral("Recargue la lista")});
    }

    // ─── eliminar ───────────────────────────────────────────────────────────

    void eliminar_borraSoloElIndicado()
    {
        auto primera = m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                           SentidoPreferencia::Preferida);
        auto segunda = m_servicio->agregar(QStringLiteral("P1"), franja(1, 14, 0, 16, 0),
                                           SentidoPreferencia::Preferida);
        QVERIFY(primera.ok);
        QVERIFY(segunda.ok);

        auto r = m_servicio->eliminar(primera.valor.id);

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QVERIFY(r.valor);
        const auto lista = m_servicio->listar(QStringLiteral("P1")).valor;
        QCOMPARE(lista.size(), 1);
        QCOMPARE(lista.first().id, segunda.valor.id);
    }

    void eliminar_inexistente_fallaConValorYAccion()
    {
        const auto r = m_servicio->eliminar(9999);
        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains(QStringLiteral("9999")), qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains(QStringLiteral("Recargue la lista")),
                 qPrintable(r.mensajeError));
    }

    void eliminar_idInvalido_fallaConValorYAccion()
    {
        const auto r = m_servicio->eliminar(0);
        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains(QStringLiteral("se recibió 0")),
                 qPrintable(r.mensajeError));
        QVERIFY2(r.mensajeError.contains(QStringLiteral("Recargue la lista")),
                 qPrintable(r.mensajeError));
    }

    // ─── limpiar ────────────────────────────────────────────────────────────

    void limpiar_borraSoloPreferenciasYNoLaDisponibilidad()
    {
        crearDisponibilidad(QStringLiteral("P1"), franja(1, 8, 0, 10, 0));
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);
        QVERIFY(m_servicio->agregar(QStringLiteral("P1"), franja(2, 14, 0, 16, 0),
                                    SentidoPreferencia::Evitada).ok);
        QVERIFY(m_servicio->agregar(QStringLiteral("P2"), franja(1, 8, 0, 10, 0),
                                    SentidoPreferencia::Preferida).ok);

        auto r = m_servicio->limpiar(QStringLiteral("P1"));

        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor, 2);
        QVERIFY(m_servicio->listar(QStringLiteral("P1")).valor.isEmpty());
        QCOMPARE(m_servicio->listar(QStringLiteral("P2")).valor.size(), 1);
        QCOMPARE(contar(QStringLiteral("Disponibilidad_Profesor")), 1);
    }

    // ─── Sin efecto en la generación ni en la huella (RF-5) ─────────────────

    /// Las preferencias no viajan al solver ni entran en la huella: cambiarlas no marca
    /// «datos anteriores» mientras el solver no las consuma.
    void huellaDatos_noCambiaConLasPreferenciasYtoProfesorLasIgnora()
    {
        DatosDominio sinPreferencias;
        ProfesorDTO docente;
        docente.id = QStringLiteral("P1");
        docente.nombre = QStringLiteral("Ana");
        DisponibilidadDTO disponibilidad;
        disponibilidad.franja = franja(1, 8, 0, 10, 0);
        docente.disponibilidad = {disponibilidad};
        sinPreferencias.docentes = {docente};

        const QString huellaBase = huellaDatos(sinPreferencias, 35);
        QVERIFY(!huellaBase.isEmpty());

        PreferenciaDTO preferencia;
        preferencia.id = 7;
        preferencia.idProfesor = QStringLiteral("P1");
        preferencia.franja = franja(2, 14, 0, 16, 0);
        preferencia.sentido = SentidoPreferencia::Preferida;

        DatosDominio conPreferencias = sinPreferencias;
        conPreferencias.docentes[0].preferencias = {preferencia};
        QCOMPARE(conPreferencias.docentes[0].preferencias.size(), 1);

        // La huella es la misma con y sin la preferencia (RF-5).
        QCOMPARE(huellaDatos(conPreferencias, 35), huellaBase);

        // Tampoco influye su contenido: otro sentido deja la misma huella.
        DatosDominio conOtra = conPreferencias;
        conOtra.docentes[0].preferencias[0].sentido = SentidoPreferencia::Evitada;
        QCOMPARE(huellaDatos(conOtra, 35), huellaBase);

        // `toProfesor()` no copia las preferencias: el modelo del solver no cambia.
        const Profesor antes = sinPreferencias.docentes[0].toProfesor();
        const Profesor despues = conPreferencias.docentes[0].toProfesor();
        QCOMPARE(despues.toJson(), antes.toJson());
        QCOMPARE(despues.disponibilidad.size(), 1);
    }

    void limpiar_sinPreferencias_devuelveCero()
    {
        const auto r = m_servicio->limpiar(QStringLiteral("P1"));
        QVERIFY2(r.ok, qPrintable(r.mensajeError));
        QCOMPARE(r.valor, 0);
    }

    void limpiar_docenteVacio_fallaConAccion()
    {
        const auto r = m_servicio->limpiar(QString());
        QVERIFY(!r.ok);
        QVERIFY2(r.mensajeError.contains(QStringLiteral("Indique el docente")),
                 qPrintable(r.mensajeError));
    }
};

QTEST_MAIN(TestServicioPreferencias)
#include "test_servicio_preferencias.moc"
