#include <QDateTime>
#include <QJsonObject>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QTest>
#include <QTime>

#include <middleware/internalclient.h>
#include <middleware/internalserver.h>
#include <middleware/messages.h>

#include <backend/services/ConstructorEntradaSolver.hpp>
#include <backend/services/ServicioGeneracion.hpp>
#include <backend/solver/servicio_solver.hpp>

/// Tests del modo proceso de cálculo (RF-1, RF-2): resuelve por IPC y nunca
/// abre la base de datos; rechaza operaciones de negocio.
class TestIpcAislamientoBd : public QObject
{
    Q_OBJECT

private:
    InternalServer* m_servidor = nullptr;

    QJsonObject entradaValida() const
    {
        DatosDominio d;

        MateriaDTO mat; mat.id = 1; mat.nombre = QStringLiteral("Matemática");
        d.materias = {mat};

        AulaDTO aula; aula.id = 10; aula.nombre = QStringLiteral("A1"); aula.capacidad = 30;
        d.aulas = {aula};

        TurnoDTO turno;
        turno.nombre = QStringLiteral("manana");
        turno.inicio = QTime(7, 0); turno.fin = QTime(12, 0);
        turno.numSlots = 6;
        d.turnos = {turno};

        CursoMateriaDTO cm; cm.idMateria = 1; cm.nombreMateria = QStringLiteral("Matemática");
        cm.horasSemanales = 2;
        CursoDTO curso;
        curso.id = 100; curso.nombre = QStringLiteral("1ro A");
        curso.turno = QStringLiteral("manana");
        curso.aulaFija = 10; curso.numEstudiantes = 20;
        curso.codigoPlan = QStringLiteral("Ciencias");
        curso.materias = {cm};
        d.cursos = {curso};

        MateriaAsignadaDTO asig; asig.id = 1; asig.nombre = QStringLiteral("Matemática");
        ProfesorDTO prof;
        prof.id = QStringLiteral("P1"); prof.nombre = QStringLiteral("Ana");
        prof.materias = {asig};
        d.docentes = {prof};

        SolverConfig p;
        p.version = QStringLiteral("1.0");
        p.dimensiones.num_dias = 5;
        p.dimensiones.num_slots_dia = 6;
        p.franja_horaria.duracion_minutos = 45;
        p.franja_horaria.slots_por_turno = 6;
        ProfesorSolverConfig ps;
        ps.nombre = QStringLiteral("Ana");
        ps.horas_requeridas = 10;
        ps.horas_aula = 8;
        p.profesores = {ps};

        return construirJsonEntrada(d, p, QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0)));
    }

private slots:

    void initTestCase()
    {
        m_servidor = new InternalServer(this);
        QVERIFY(m_servidor->start());

        // Misma ruta que registra `aplicacion_backend`.
        m_servidor->registerRoute(Middleware::OP_RESOLVER_HORARIO,
            [this](const QJsonObject& payload, QLocalSocket* socket) {
                const RespuestaSolver r = resolverEntradaSolver(payload);
                if (!r.ok)
                    m_servidor->responder(socket, Middleware::RESP_INVALIDO, r.error);
                else if (!r.factible)
                    m_servidor->responder(socket, Middleware::RESP_SIN_SOLUCION, r.error);
                else
                    m_servidor->responder(socket, Middleware::RESP_EXITO, r.salida);
            });
    }

    void cleanupTestCase()
    {
        delete m_servidor;
        m_servidor = nullptr;
    }

    void resolver_noAbreBaseDeDatos()
    {
        const int conexionesAntes = QSqlDatabase::connectionNames().size();

        InternalClient client;
        QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
        QVERIFY(spy.isValid());

        client.enviarSolicitud(Middleware::OP_RESOLVER_HORARIO, entradaValida());
        QVERIFY(spy.wait(15000));

        const QJsonObject r = spy.at(0).at(0).toJsonObject();
        const int code = r["code"].toInt();
        // El proceso resolvió (con o sin solución factible), pero nunca abrió la BD.
        QVERIFY(code == Middleware::RESP_EXITO || code == Middleware::RESP_SIN_SOLUCION);

        if (code == Middleware::RESP_EXITO)
            QVERIFY(r["data"].toObject().contains("horarios"));

        // RF-2: el proceso de cálculo no abrió ninguna conexión de base de datos.
        QCOMPARE(QSqlDatabase::connectionNames().size(), conexionesAntes);
    }

    void operacionesDeNegocio_seRechazan()
    {
        InternalClient client;
        QSignalSpy spy(&client, &InternalClient::respuestaRecibida);

        client.enviarSolicitud(Middleware::OP_CREAR_PROFESOR, QJsonObject{});
        QVERIFY(spy.wait(3000));

        const QJsonObject r = spy.at(0).at(0).toJsonObject();
        QCOMPARE(r["code"].toInt(), Middleware::RESP_INVALIDO);
    }

    void entradaInvalida_devuelveError()
    {
        InternalClient client;
        QSignalSpy spy(&client, &InternalClient::respuestaRecibida);

        client.enviarSolicitud(Middleware::OP_RESOLVER_HORARIO, QJsonObject{});
        QVERIFY(spy.wait(3000));

        const QJsonObject r = spy.at(0).at(0).toJsonObject();
        QCOMPARE(r["code"].toInt(), Middleware::RESP_INVALIDO);
    }
};

QTEST_MAIN(TestIpcAislamientoBd)
#include "test_ipc_aislamiento_bd.moc"
