#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTime>
#include <QTest>

#include <backend/data/materia.hpp>
#include <backend/data/plan_estudio.hpp>
#include <backend/services/ConstructorEntradaSolver.hpp>
#include <backend/solver/config/solver_config.hpp>

/// Tests del constructor del JSON de entrada del solver (RF-1, contrato 2.0).
class TestConstructorEntrada : public QObject
{
    Q_OBJECT

private:
    static QDateTime ahoraFijo()
    {
        return QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0));
    }

    DatosDominio dominioCompleto() const
    {
        DatosDominio d;

        MateriaDTO mat1;
        mat1.id = 1; mat1.nombre = QStringLiteral("Matemática");
        mat1.idTipoAula = 1; mat1.tipoAula = QStringLiteral("Aula de estudio");
        MateriaDTO mat2;
        mat2.id = 2; mat2.nombre = QStringLiteral("Física");
        mat2.idTipoAula = 1; mat2.tipoAula = QStringLiteral("Aula de estudio");
        d.materias = {mat1, mat2};

        AulaDTO aula;
        aula.id = 10; aula.nombre = QStringLiteral("A1"); aula.capacidad = 30;
        aula.idTipoAula = 1; aula.tipoAula = QStringLiteral("Aula de estudio");
        d.aulas = {aula};

        EventoDTO evento;
        evento.id = 1; evento.nombre = QStringLiteral("Receso");
        evento.despuesDeSlot = 1; evento.orden = 1; evento.duracion = 10;
        // Ventana de la posición 1: turno 07:00–12:00 (300 min) − 10 min del evento = 290;
        // 290 / 6 = 48 min de franja → 07:48–07:58, donde cabe el evento.
        evento.inicio = QTime(7, 48); evento.fin = QTime(7, 58);
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
        p.version = QStringLiteral("2.0");
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

    /// Desempaqueta la construcción válida sin repetir el mensaje de error en cada caso.
    QJsonObject entradaValida() const
    {
        const auto resultado = construirJsonEntrada(dominioCompleto(), parametros(), ahoraFijo());
        const bool ok = QTest::qVerify(resultado.ok, "construirJsonEntrada(dominioCompleto())",
                                       qPrintable(resultado.mensajeError), __FILE__, __LINE__);
        Q_UNUSED(ok);
        return resultado.valor;
    }

private slots:

    void construyeJsonEstructuralmenteCompleto()
    {
        const QJsonObject json = entradaValida();

        // 13 secciones: las 12 del contrato 2.0 + `meta`; la de recesos ya no existe.
        for (const QString& seccion : {QStringLiteral("version"), QStringLiteral("aula_fija"),
                                       QStringLiteral("meta"), QStringLiteral("dimensiones"),
                                       QStringLiteral("franja_horaria"), QStringLiteral("turnos"),
                                       QStringLiteral("eventos"), QStringLiteral("planes"),
                                       QStringLiteral("cursos"), QStringLiteral("profesores"),
                                       QStringLiteral("materias"), QStringLiteral("aulas")})
        {
            QVERIFY2(json.contains(seccion), qPrintable(seccion));
        }
        QVERIFY(!json.contains(QStringLiteral("recesos")));
        QCOMPARE(json[QStringLiteral("version")].toString(), QStringLiteral("2.0"));
    }

    void dimensionesSonDerivadas()
    {
        const QJsonObject dim = entradaValida()["dimensiones"].toObject();

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
        const auto resultado = construirJsonEntrada(dominioCompleto(), parametros(), ahora);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));

        QCOMPARE(resultado.valor["meta"].toObject()["fecha_modificacion"].toString(),
                 ahora.toString(Qt::ISODate));
    }

    /// RF-1: los turnos llevan su ventana (`inicio`/`fin`) y los eventos viajan en su sección,
    /// sin identidad, con nombre, turno, posición, orden, duración y horario.
    void turnosYeventosSeDerivanDelDominio()
    {
        const QJsonObject json = entradaValida();

        const QJsonArray turnos = json["turnos"].toObject()["turnos"].toArray();
        QCOMPARE(turnos.size(), 1);
        QCOMPARE(turnos[0].toObject()["nombre"].toString(), QStringLiteral("manana"));
        QCOMPARE(turnos[0].toObject()["slot"].toArray().size(), 6);
        QCOMPARE(turnos[0].toObject()["inicio"].toString(), QStringLiteral("07:00"));
        QCOMPARE(turnos[0].toObject()["fin"].toString(), QStringLiteral("12:00"));

        const QJsonArray eventos = json["eventos"].toArray();
        QCOMPARE(eventos.size(), 1);
        const QJsonObject evento = eventos[0].toObject();
        QVERIFY(!evento.contains(QStringLiteral("id")));
        QCOMPARE(evento["nombre"].toString(), QStringLiteral("Receso"));
        QCOMPARE(evento["turno"].toString(), QStringLiteral("manana"));
        QCOMPARE(evento["despues_de_slot"].toInt(), 1);
        QCOMPARE(evento["orden"].toInt(), 1);
        QCOMPARE(evento["duracion"].toInt(), 10);
        QCOMPARE(evento["inicio"].toString(), QStringLiteral("07:48"));
        QCOMPARE(evento["fin"].toString(), QStringLiteral("07:58"));
    }

    /// RF-1: se emite una entrada por evento, aunque compartan posición.
    void dosEventosEnLaMismaPosicion_emitenDosEventos()
    {
        DatosDominio datos = dominioCompleto();
        EventoDTO acto;
        acto.id = 2; acto.nombre = QStringLiteral("Acto cívico");
        acto.despuesDeSlot = 1; acto.orden = 2; acto.duracion = 5;
        datos.turnos[0].eventos.append(acto);

        const auto resultado = construirJsonEntrada(datos, parametros(), ahoraFijo());
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));

        const QJsonArray eventos = resultado.valor["eventos"].toArray();
        QCOMPARE(eventos.size(), 2);
        QCOMPARE(eventos[0].toObject()["nombre"].toString(), QStringLiteral("Receso"));
        QCOMPARE(eventos[0].toObject()["turno"].toString(), QStringLiteral("manana"));
        QCOMPARE(eventos[0].toObject()["despues_de_slot"].toInt(), 1);
        QCOMPARE(eventos[0].toObject()["duracion"].toInt(), 10);
        QCOMPARE(eventos[1].toObject()["nombre"].toString(), QStringLiteral("Acto cívico"));
        QCOMPARE(eventos[1].toObject()["despues_de_slot"].toInt(), 1);
        QCOMPARE(eventos[1].toObject()["orden"].toInt(), 2);
        QCOMPARE(eventos[1].toObject()["duracion"].toInt(), 5);
    }

    /// RF-1: el tipo del aula y el tipo requerido por cada materia salen del dominio, sin
    /// valores por defecto.
    void tiposDeMateriasYAulasSeCopianDelDominio()
    {
        const QJsonObject json = entradaValida();

        const QJsonArray materias = json["materias"].toArray();
        QCOMPARE(materias.size(), 2);
        QCOMPARE(materias[0].toObject()["nombre"].toString(), QStringLiteral("Matemática"));
        QCOMPARE(materias[0].toObject()["tipo"].toString(), QStringLiteral("Aula de estudio"));
        QCOMPARE(materias[1].toObject()["nombre"].toString(), QStringLiteral("Física"));
        QCOMPARE(materias[1].toObject()["tipo"].toString(), QStringLiteral("Aula de estudio"));

        const QJsonArray aulas = json["aulas"].toArray();
        QCOMPARE(aulas.size(), 1);
        QCOMPARE(aulas[0].toObject()["tipo"].toString(), QStringLiteral("Aula de estudio"));
        QVERIFY(aulas[0].toObject()["tipo"].toString() != QStringLiteral("regular"));
    }

    /// RF-1: si un aula de la instantánea no declara tipo, la construcción falla nombrando la
    /// entidad y no emite JSON.
    void aulaSinTipo_fallaSinEmitirJson()
    {
        DatosDominio datos = dominioCompleto();
        datos.aulas[0].tipoAula.clear();

        const auto resultado = construirJsonEntrada(datos, parametros(), ahoraFijo());

        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("A1")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("tipo")),
                 qPrintable(resultado.mensajeError));
        QVERIFY(resultado.valor.isEmpty());
    }

    /// RF-1: si una materia de la instantánea no declara tipo, la construcción también falla.
    void materiaSinTipo_fallaSinEmitirJson()
    {
        DatosDominio datos = dominioCompleto();
        datos.materias[1].tipoAula = QStringLiteral("   ");   // solo espacios: ausente

        const auto resultado = construirJsonEntrada(datos, parametros(), ahoraFijo());

        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("Física")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("tipo")),
                 qPrintable(resultado.mensajeError));
        QVERIFY(resultado.valor.isEmpty());
    }

    /// El orden del primer defecto es determinista: primero aulas, después materias.
    void tipoAusenteEnAulaYmateria_informaElAula()
    {
        DatosDominio datos = dominioCompleto();
        datos.aulas[0].tipoAula.clear();
        datos.materias[0].tipoAula.clear();

        const auto resultado = construirJsonEntrada(datos, parametros(), ahoraFijo());

        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("A1")),
                 qPrintable(resultado.mensajeError));
    }

    void cursosYprofesoresReferencianElPoolDeMaterias()
    {
        const QJsonObject json = entradaValida();

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
        const QJsonObject json = entradaValida();

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

    void jsonGeneradoEsConsumiblePorElSolver()
    {
        const auto config = SolverConfig::fromJson(entradaValida());
        QVERIFY2(config.ok, qPrintable(config.mensajeError));
    }

    /// RF-1: ida y vuelta de la entrada 2.0 sin pérdida de tipos ni de eventos.
    void idaYVuelta2_0_reproduceTiposYeventos()
    {
        const auto primera = SolverConfig::fromJson(entradaValida());
        QVERIFY2(primera.ok, qPrintable(primera.mensajeError));

        QCOMPARE(primera.valor.version, QStringLiteral("2.0"));
        QCOMPARE(primera.valor.materias.size(), 2);
        QCOMPARE(primera.valor.materias[0].tipo, QStringLiteral("Aula de estudio"));
        QCOMPARE(primera.valor.aulas[0].tipo, QStringLiteral("Aula de estudio"));
        QCOMPARE(primera.valor.turnos.turnos[0].inicio, QStringLiteral("07:00"));
        QCOMPARE(primera.valor.turnos.turnos[0].fin, QStringLiteral("12:00"));
        QCOMPARE(primera.valor.eventos.size(), 1);
        QCOMPARE(primera.valor.eventos[0].nombre, QStringLiteral("Receso"));
        QCOMPARE(primera.valor.eventos[0].despues_de_slot, 1);
        QCOMPARE(primera.valor.eventos[0].duracion, 10);

        const QJsonObject serializado = primera.valor.toJson();
        const auto segunda = SolverConfig::fromJson(serializado);
        QVERIFY2(segunda.ok, qPrintable(segunda.mensajeError));
        QCOMPARE(segunda.valor.toJson(), serializado);
    }

    /// El constructor no declara versión: la entrada se sella en 2.0 y sigue siendo consumible.
    void entradaSinVersionDeclarada_seSellaA_2_0()
    {
        SolverConfig p = parametros();
        p.version.clear();   // el origen no declara version

        const auto resultado = construirJsonEntrada(dominioCompleto(), p, ahoraFijo());
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));

        QCOMPARE(resultado.valor[QStringLiteral("version")].toString(), QStringLiteral("2.0"));
        const auto config = SolverConfig::fromJson(resultado.valor);
        QVERIFY2(config.ok, qPrintable(config.mensajeError));
    }

    /// La fachada devuelve el mismo resultado que la función libre.
    void fachadaDevuelveElMismoResultado()
    {
        const auto libre = construirJsonEntrada(dominioCompleto(), parametros(), ahoraFijo());
        const auto fachada =
            ConstructorEntradaSolver::construir(dominioCompleto(), parametros(), ahoraFijo());

        QCOMPARE(fachada.ok, libre.ok);
        QCOMPARE(fachada.valor, libre.valor);
        QVERIFY(fachada.ok);
    }

    void esPura_conElMismoInstanteDaLaMismaEntrada()
    {
        const auto a = construirJsonEntrada(dominioCompleto(), parametros(), ahoraFijo());
        const auto b = construirJsonEntrada(dominioCompleto(), parametros(), ahoraFijo());

        QVERIFY(a.ok && b.ok);
        QCOMPARE(a.valor, b.valor);
    }
};

QTEST_MAIN(TestConstructorEntrada)
#include "test_constructor_entrada.moc"
