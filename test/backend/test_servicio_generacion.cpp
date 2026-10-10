#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QTime>

#include <backend/services/ConstructorEntradaSolver.hpp>
#include <backend/services/ServicioGeneracion.hpp>

using Estado = ResultadoGeneracion::Estado;

/// Puerto falso hacia el proceso de cálculo: guarda el callback para responder
/// cuando el test quiera (o nunca, para simular un timeout).
class PuertoFalso : public PuertoSolver
{
public:
    int llamadas = 0;
    std::function<void(const RespuestaSolver&)> cb;

    void resolver(const QJsonObject&, std::function<void(const RespuestaSolver&)> alResponder) override
    {
        ++llamadas;
        cb = std::move(alResponder);
    }

    void responder(const RespuestaSolver& r)
    {
        if (cb)
            cb(r);
    }
};

/// Tests del servicio de generación: envío, timeout, no factible, contrato
/// inválido y «datos anteriores» (RF-1, RF-3, RF-5).
class TestServicioGeneracion : public QObject
{
    Q_OBJECT

private:
    DatosDominio dominio() const
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

        return d;
    }

    SolverConfig parametros() const
    {
        SolverConfig p;
        p.version = QStringLiteral("1.0");
        p.dimensiones.num_dias = 5;
        p.dimensiones.num_slots_dia = 6;
        p.franja_horaria.duracion_minutos = 45;
        p.franja_horaria.slots_por_turno = 6;
        ProfesorSolverConfig prof;
        prof.nombre = QStringLiteral("Ana");
        prof.horas_requeridas = 10;
        prof.horas_aula = 8;
        p.profesores = {prof};
        return p;
    }

    QJsonObject entradaValida() const
    {
        return construirJsonEntrada(dominio(), parametros(),
                                    QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0)));
    }

    /// Salida válida: cubre las 2 horas sin solapamientos.
    RespuestaSolver respuestaFactible() const
    {
        HorarioSalida h;
        CursoOutput curso;
        curso.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{0, 0, 0, 0}, AsignacionOutput{1, 0, 0, 0}};
        curso.dias = {dia};
        h.horarios[QStringLiteral("1ro A")] = curso;

        RespuestaSolver r;
        r.ok = true;
        r.factible = true;
        r.salida = h.toJson();
        return r;
    }

    RespuestaSolver respuestaConSolapamiento() const
    {
        RespuestaSolver r = respuestaFactible();
        QJsonObject salida = r.salida;
        QJsonObject horarios = salida["horarios"].toObject();
        QJsonObject otro;
        otro["turno"] = QStringLiteral("manana");
        QJsonArray dias;
        QJsonObject dia;
        dia["dia"] = 0;
        dia["asignaciones"] = QJsonArray{QJsonObject{{"slot", 0}, {"materia", 0},
                                                     {"profesor", 0}, {"aula", 0}}};
        dias.append(dia);
        otro["dias"] = dias;
        horarios["1ro B"] = otro;
        salida["horarios"] = horarios;
        r.salida = salida;
        return r;
    }

private slots:

    void entradaInvalida_noEnvia()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);

        servicio.generar(QJsonObject{}, QStringLiteral("h"));

        QCOMPARE(servicio.ultimoResultado().estado, Estado::EntradaInvalida);
        QCOMPARE(puerto.llamadas, 0);
    }

    void segundaGeneracion_noAdmitida()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);

        servicio.generar(entradaValida(), QStringLiteral("h"));
        QVERIFY(servicio.enCurso());
        servicio.generar(entradaValida(), QStringLiteral("h"));

        QCOMPARE(puerto.llamadas, 1);
    }

    void noFactible()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r; r.ok = true; r.factible = false;
        puerto.responder(r);

        QCOMPARE(servicio.ultimoResultado().estado, Estado::NoFactible);
        QVERIFY(!servicio.ultimoResultado().presentable);
    }

    void contratoInvalido()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r; r.ok = true; r.factible = true; r.salida = QJsonObject{};
        puerto.responder(r);

        QCOMPARE(servicio.ultimoResultado().estado, Estado::ContratoInvalido);
    }

    void solapamiento_calculoFallido()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        puerto.responder(respuestaConSolapamiento());

        QCOMPARE(servicio.ultimoResultado().estado, Estado::CalculoFallido);
        QVERIFY(!servicio.ultimoResultado().presentable);
    }

    void ok_presentable()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        puerto.responder(respuestaFactible());

        QCOMPARE(servicio.ultimoResultado().estado, Estado::Listo);
        QVERIFY(servicio.ultimoResultado().presentable);
    }

    void datosAnteriores_siLaHuellaCambia()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0, []() { return QStringLiteral("otra"); });
        servicio.generar(entradaValida(), QStringLiteral("h"));

        puerto.responder(respuestaFactible());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.estado, Estado::DatosAnteriores);
        QVERIFY(r.datosAnteriores);
        QVERIFY(r.presentable);
    }

    void timeout_abandonaEignoraRespuestaTardia()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 50);
        QSignalSpy spy(&servicio, &ServicioGeneracion::finalizada);

        servicio.generar(entradaValida(), QStringLiteral("h"));
        QVERIFY(servicio.enCurso());
        QVERIFY(spy.wait(3000));

        QCOMPARE(servicio.ultimoResultado().estado, Estado::TiempoAgotado);
        QVERIFY(!servicio.ultimoResultado().presentable);
        QVERIFY(!servicio.enCurso());

        // Una respuesta posterior se ignora: el estado no cambia.
        puerto.responder(respuestaFactible());
        QTest::qWait(20);
        QCOMPARE(servicio.ultimoResultado().estado, Estado::TiempoAgotado);
        QCOMPARE(spy.count(), 1);
    }

    void huellaDatos_esDeterministaYcambiaConLosDatos()
    {
        const QString h1 = huellaDatos(dominio());
        QCOMPARE(h1, huellaDatos(dominio()));

        DatosDominio modificado = dominio();
        modificado.cursos[0].numEstudiantes = 99;
        QVERIFY(huellaDatos(modificado) != h1);
    }

    void estamparFechaGeneracion_usaElInstante()
    {
        HorarioSalida h;
        const QDateTime ahora(QDate(2026, 10, 10), QTime(15, 0, 0));
        QCOMPARE(estamparFechaGeneracion(h, ahora).metadata.fecha_generacion,
                 ahora.toString(Qt::ISODate));
    }
};

QTEST_MAIN(TestServicioGeneracion)
#include "test_servicio_generacion.moc"
