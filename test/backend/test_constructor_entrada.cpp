#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTime>
#include <QTest>

#include <backend/data/materia.hpp>
#include <backend/data/plan_estudio.hpp>
#include <backend/services/ConstructorEntradaSolver.hpp>
#include <backend/solver/config/solver_config.hpp>

/// Tests del constructor del JSON de entrada del solver (RF-1).
class TestConstructorEntrada : public QObject
{
    Q_OBJECT

private:
    DatosDominio dominioCompleto() const
    {
        DatosDominio d;

        MateriaDTO mat1; mat1.id = 1; mat1.nombre = QStringLiteral("Matemática");
        MateriaDTO mat2; mat2.id = 2; mat2.nombre = QStringLiteral("Física");
        d.materias = {mat1, mat2};

        AulaDTO aula; aula.id = 10; aula.nombre = QStringLiteral("A1"); aula.capacidad = 30;
        d.aulas = {aula};

        EventoDTO evento;
        evento.id = 1; evento.nombre = QStringLiteral("Receso");
        evento.despuesDeSlot = 1; evento.orden = 1; evento.duracion = 10;
        evento.inicio = QTime(8, 20); evento.fin = QTime(8, 30);
        TurnoDTO turno;
        turno.nombre = QStringLiteral("manana");
        turno.inicio = QTime(7, 0); turno.fin = QTime(12, 0);
        turno.numSlots = 6;
        turno.eventos = {evento};
        d.turnos = {turno};

        CursoDTO curso;
        curso.id = 100; curso.anio = 1; curso.seccion = QStringLiteral("A");
        curso.turno = QStringLiteral("manana");
        curso.aula = 10;
        d.cursos = {curso};

        // Plan compuesto (materias + horas) referenciado por el curso.
        Materia mPlan; mPlan.nombre = QStringLiteral("Matemática");
        MateriaEnPlan mp; mp.curso = 1; mp.horas = 5; mp.materia = mPlan;
        PlanEstudio plan; plan.codigo = QStringLiteral("Ciencias"); plan.nombre = QStringLiteral("Ciencias");
        plan.materias = {mp};
        d.planes = {plan};

        FranjaHoraria franja; franja.dia = 1; franja.inicio = QTime(7, 0); franja.fin = QTime(9, 0);
        DisponibilidadDTO disp; disp.id = 0; disp.franja = franja;
        MateriaAsignadaDTO asig; asig.id = 1; asig.nombre = QStringLiteral("Matemática");
        ProfesorDTO prof;
        prof.id = QStringLiteral("P1"); prof.nombre = QStringLiteral("Ana");
        prof.email = QStringLiteral("ana@test.com");
        prof.disponibilidad = {disp};
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
        p.aula_fija = true;

        CursoSolverConfig curso;      // del preset: plan + num_estudiantes
        curso.anio = 1; curso.seccion = QStringLiteral("A");
        curso.plan = QStringLiteral("Ciencias"); curso.num_estudiantes = 20;
        p.cursos = {curso};

        ProfesorSolverConfig prof;
        prof.nombre = QStringLiteral("Ana");
        prof.horas_requeridas = 10;
        prof.horas_aula = 8;
        p.profesores = {prof};
        return p;
    }

private slots:

    void construyeJsonEstructuralmenteCompleto()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 30, 0));
        const QJsonObject json = construirJsonEntrada(dominioCompleto(), parametros(), ahora);

        // 13 secciones: las 12 del contrato implementado + `meta`.
        for (const QString& seccion : {QStringLiteral("version"), QStringLiteral("aula_fija"),
                                       QStringLiteral("meta"), QStringLiteral("dimensiones"),
                                       QStringLiteral("franja_horaria"), QStringLiteral("turnos"),
                                       QStringLiteral("recesos"), QStringLiteral("planes"),
                                       QStringLiteral("cursos"), QStringLiteral("profesores"),
                                       QStringLiteral("materias"), QStringLiteral("aulas")})
        {
            QVERIFY2(json.contains(seccion), qPrintable(seccion));
        }
    }

    void dimensionesSonDerivadas()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        const QJsonObject dim = construirJsonEntrada(dominioCompleto(), parametros(), ahora)["dimensiones"].toObject();

        QCOMPARE(dim["num_profesores"].toInt(), 1);
        QCOMPARE(dim["num_materias"].toInt(), 2);
        QCOMPARE(dim["num_aulas"].toInt(), 1);
        QCOMPARE(dim["num_cursos"].toInt(), 1);
        QCOMPARE(dim["num_dias"].toInt(), 5);
        QCOMPARE(dim["num_slots_dia"].toInt(), 6);
    }

    void metaUsaElInstanteInyectado()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 30, 0));
        const QJsonObject json = construirJsonEntrada(dominioCompleto(), parametros(), ahora);

        QCOMPARE(json["meta"].toObject()["fecha_modificacion"].toString(),
                 ahora.toString(Qt::ISODate));
    }

    void turnosYrecesoSeDerivanDelDominio()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        const QJsonObject json = construirJsonEntrada(dominioCompleto(), parametros(), ahora);

        const QJsonArray turnos = json["turnos"].toObject()["turnos"].toArray();
        QCOMPARE(turnos.size(), 1);
        QCOMPARE(turnos[0].toObject()["nombre"].toString(), QStringLiteral("manana"));
        QCOMPARE(turnos[0].toObject()["slot"].toArray().size(), 6);

        const QJsonArray recesos = json["recesos"].toArray();
        QCOMPARE(recesos.size(), 1);
        QCOMPARE(recesos[0].toObject()["turno"].toString(), QStringLiteral("manana"));
        QCOMPARE(recesos[0].toObject()["despues_de_slot"].toInt(), 1);
    }

    /// RF-12: el puente emite una entrada por evento, aunque compartan posición.
    void dosEventosEnLaMismaPosicion_emitenDosRecesos()
    {
        DatosDominio datos = dominioCompleto();
        EventoDTO acto;
        acto.id = 2; acto.nombre = QStringLiteral("Acto cívico");
        acto.despuesDeSlot = 1; acto.orden = 2; acto.duracion = 5;
        datos.turnos[0].eventos.append(acto);

        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        const QJsonObject json = construirJsonEntrada(datos, parametros(), ahora);

        const QJsonArray recesos = json["recesos"].toArray();
        QCOMPARE(recesos.size(), 2);
        QCOMPARE(recesos[0].toObject()["turno"].toString(), QStringLiteral("manana"));
        QCOMPARE(recesos[0].toObject()["despues_de_slot"].toInt(), 1);
        QCOMPARE(recesos[0].toObject()["duracion"].toInt(), 10);
        QCOMPARE(recesos[1].toObject()["despues_de_slot"].toInt(), 1);
        QCOMPARE(recesos[1].toObject()["duracion"].toInt(), 5);
    }

    void cursosYprofesoresReferencianElPoolDeMaterias()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        const QJsonObject json = construirJsonEntrada(dominioCompleto(), parametros(), ahora);

        // Las materias del curso viven en su plan EMBEBIDO (RF-7/RF-13).
        QCOMPARE(json["planes"].toArray().size(), 1);
        QCOMPARE(json["planes"].toArray()[0].toObject()["materias"].toArray()[0]
                     .toObject()["materia_idx"].toInt(), 0);
        QCOMPARE(json["profesores"].toArray()[0].toObject()["materias_asignadas"].toArray()[0].toInt(),
                 0);
        // El docente del dominio toma sus horas del preset (casadas por nombre).
        QCOMPARE(json["profesores"].toArray()[0].toObject()["horas_requeridas"].toInt(), 10);
    }

    void disponibilidadSeMapeaAIndicesDelSolver()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        const QJsonObject json = construirJsonEntrada(dominioCompleto(), parametros(), ahora);

        const QJsonObject disp =
            json["profesores"].toArray()[0].toObject()["disponibilidad"].toArray()[0].toObject();
        // Dominio: día 1 (lunes) → solver: día 0.
        QCOMPARE(disp["dia"].toInt(), 0);
        // 07:00–09:00 con slots de 45 min → 2 slots (0 y 1) del turno de la mañana.
        const QJsonArray listaSlots = disp["slot"].toArray();
        QCOMPARE(listaSlots.size(), 2);
        QCOMPARE(listaSlots[0].toInt(), 0);
        QCOMPARE(listaSlots[1].toInt(), 1);
    }

    void jsonGeneradoEsConsumibleporElSolver()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        const QJsonObject json = construirJsonEntrada(dominioCompleto(), parametros(), ahora);

        const auto config = SolverConfig::fromJson(json);
        QVERIFY2(config.ok, qPrintable(config.mensajeError));
    }

    /// El preset no declara `version`: la entrada construida debe salir sellada con "1.0".
    void entradaSinVersionDeclarada_seSellaA_1_0()
    {
        SolverConfig p = parametros();
        p.version.clear();   // el origen no declara version

        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        const QJsonObject json = construirJsonEntrada(dominioCompleto(), p, ahora);

        QCOMPARE(json[QStringLiteral("version")].toString(), QStringLiteral("1.0"));
        // Sigue siendo consumible por el solver (no se rechaza por version).
        const auto config = SolverConfig::fromJson(json);
        QVERIFY2(config.ok, qPrintable(config.mensajeError));
    }

    void esPura_conElMismoInstanteDaElMismoJson()
    {
        const QDateTime ahora(QDate(2026, 10, 10), QTime(12, 0, 0));
        QCOMPARE(construirJsonEntrada(dominioCompleto(), parametros(), ahora),
                 construirJsonEntrada(dominioCompleto(), parametros(), ahora));
    }
};

QTEST_MAIN(TestConstructorEntrada)
#include "test_constructor_entrada.moc"
