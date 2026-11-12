#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QTime>

#include <backend/services/ConstructorEntradaSolver.hpp>
#include <backend/data/materia.hpp>
#include <backend/data/plan_estudio.hpp>
#include <backend/services/ServicioGeneracion.hpp>
#include <backend/services/ServicioTurnosEventos.hpp>
#include <backend/services/preferencias_cumplimiento.hpp>
#include <backend/services/proyeccion_preferencias.hpp>
#include <backend/solver/config/contrato_solver.hpp>

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
        mat.idTipoAula = 1; mat.tipoAula = QStringLiteral("Aula de estudio");
        d.materias = {mat};

        AulaDTO aula; aula.id = 10; aula.nombre = QStringLiteral("A1"); aula.capacidad = 30;
        aula.idTipoAula = 1; aula.tipoAula = QStringLiteral("Aula de estudio");
        d.aulas = {aula};

        TurnoDTO turno;
        turno.nombre = QStringLiteral("manana");
        turno.inicio = QTime(7, 0); turno.fin = QTime(12, 0);
        turno.numSlots = 6;
        d.turnos = {turno};

        CursoDTO curso;
        curso.id = 100; curso.anio = 1; curso.seccion = QStringLiteral("A");
        curso.turno = QStringLiteral("manana");
        curso.aula = 10;
        d.cursos = {curso};

        Materia mPlan; mPlan.nombre = QStringLiteral("Matemática");
        MateriaEnPlan mp; mp.curso = 1; mp.horas = 1; mp.materia = mPlan;   // 1 h -> 2 bloques
        PlanEstudio plan; plan.codigo = QStringLiteral("Ciencias"); plan.nombre = QStringLiteral("Ciencias");
        plan.materias = {mp};
        d.planes = {plan};

        FranjaHoraria franja; franja.dia = 1; franja.inicio = QTime(7, 0); franja.fin = QTime(9, 0);
        DisponibilidadDTO disp; disp.id = 0; disp.franja = franja;
        MateriaAsignadaDTO asig; asig.id = 1; asig.nombre = QStringLiteral("Matemática");
        ProfesorDTO prof;
        prof.id = QStringLiteral("P1"); prof.nombre = QStringLiteral("Ana");
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
        CursoSolverConfig curso; curso.anio = 1; curso.seccion = QStringLiteral("A");
        curso.plan = QStringLiteral("Ciencias"); curso.num_estudiantes = 20;
        p.cursos = {curso};
        ProfesorSolverConfig prof;
        prof.nombre = QStringLiteral("Ana");
        prof.horas_requeridas = 10;
        prof.horas_aula = 8;
        p.profesores = {prof};
        return p;
    }

    QJsonObject entradaValida() const
    {
        const auto resultado = construirJsonEntrada(dominio(), parametros(), 35,
                                                    QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0)));
        const bool ok = QTest::qVerify(resultado.ok, "construirJsonEntrada(dominio())",
                                       qPrintable(resultado.mensajeError), __FILE__, __LINE__);
        Q_UNUSED(ok);
        return resultado.valor;
    }

    /// La entrada válida con el primer docente sin capacidad: `horas_aula = 0` declarado o el
    /// campo ausente (020, RF-1). El resto de la entrada no cambia.
    QJsonObject entradaSinHorasAula(bool quitarElCampo) const
    {
        QJsonObject j = entradaValida();
        QJsonArray profesores = j[QStringLiteral("profesores")].toArray();
        QJsonObject profesor = profesores[0].toObject();
        if (quitarElCampo)
            profesor.remove(QStringLiteral("horas_aula"));
        else
            profesor[QStringLiteral("horas_aula")] = 0;
        profesores[0] = profesor;
        j[QStringLiteral("profesores")] = profesores;
        return j;
    }

    /// Una preferencia ya proyectada, para fijar qué entra en la huella (013, RF-6).
    static PreferenciaProyectada interviniente(int docente, int dia, const QVector<int>& slot,
                                               SentidoPreferencia sentido)
    {
        PreferenciaProyectada p;
        p.docente = docente;
        p.dia = dia;
        p.slot = slot;
        p.sentido = sentido;
        return p;
    }

    /// Añade al dominio una preferencia de reloj del docente `indice` (dominio: 0 = domingo).
    static void agregarPreferenciaAlDominio(DatosDominio& datos, int indice, int dia,
                                            const QTime& inicio, const QTime& fin,
                                            SentidoPreferencia sentido)
    {
        PreferenciaDTO preferencia;
        preferencia.id = 1;
        preferencia.idProfesor = datos.docentes[indice].id;
        preferencia.franja = FranjaHoraria{dia, inicio, fin};
        preferencia.sentido = sentido;
        datos.docentes[indice].preferencias.append(preferencia);
    }

    /// Preferencia ya proyectada para la configuración del solver (día de la generación).
    static PreferenciaSolverConfig preferencia(int docente, int dia, const QVector<int>& franjas,
                                                const QString& sentido)
    {
        PreferenciaSolverConfig p;
        p.docente = docente;
        p.dia = dia;
        p.slot = franjas;
        p.sentido = sentido;
        return p;
    }

    /// Preferencia no aplicada, con su franja de DOMINIO y su único motivo.
    static PreferenciaNoAplicada noAplicada(int docente, int diaDominio,
                                             SentidoPreferencia sentido,
                                             MotivoPreferencia motivo)
    {
        PreferenciaNoAplicada p;
        p.docente = docente;
        p.franja = FranjaHoraria{diaDominio, QTime(7, 0), QTime(9, 0)};
        p.sentido = sentido;
        p.motivo = motivo;
        return p;
    }

    /// Añade una clase al horario de prueba (y crea curso/día si hacen falta).
    static void agregarClase(HorarioSalida& horario, const QString& curso, int dia, int slot,
                             int materia, int profesor)
    {
        CursoOutput& output = horario.horarios[curso];
        output.turno = QStringLiteral("manana");
        const AsignacionOutput asignacion{slot, materia, profesor, 0};
        for (DiaOutput& d : output.dias) {
            if (d.dia != dia)
                continue;
            d.asignaciones.append(asignacion);
            return;
        }
        DiaOutput nuevo;
        nuevo.dia = dia;
        nuevo.asignaciones = {asignacion};
        output.dias.append(nuevo);
    }

    /// Horario de prueba del curso 1|A: dos clases de la materia 0 con el docente 0 (día 0).
    static HorarioSalida horarioDosClases()
    {
        HorarioSalida horario;
        agregarClase(horario, claveCurso(1, QStringLiteral("A")), 0, 0, 0, 0);
        agregarClase(horario, claveCurso(1, QStringLiteral("A")), 0, 1, 0, 0);
        return horario;
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
        h.horarios[claveCurso(1, QStringLiteral("A"))] = curso;

        RespuestaSolver r;
        r.desenlace = DesenlaceSolver::Exito;
        r.ok = true;
        r.factible = true;
        r.salida = h.toJson();
        return r;
    }

    /// Salida válida con la metadata `fases_sin_optimalidad` inyectada (020, D8).
    RespuestaSolver respuestaFactibleConFases(const QStringList& fases) const
    {
        RespuestaSolver r = respuestaFactible();
        QJsonObject salida = r.salida;
        QJsonObject metadata = salida.value(QStringLiteral("metadata")).toObject();
        QJsonArray lista;
        for (const QString& fase : fases)
            lista.append(fase);
        metadata[QStringLiteral("fases_sin_optimalidad")] = lista;
        salida[QStringLiteral("metadata")] = metadata;
        r.salida = salida;
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
        horarios[claveCurso(1, QStringLiteral("B"))] = otro;
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

    void baseInvalida_noEnvia()
    {
        PuertoFalso puerto;
        const QString mensajeBase = QStringLiteral(
            "La base de datos no cumple el esquema esperado: falta el disparador "
            "«TiposAula_general_no_borrar». Restaure un respaldo de la versión vigente o cree "
            "una base nueva (con respaldo obligatorio).");
        ServicioGeneracion servicio(puerto, 0, {}, {}, [mensajeBase]() { return mensajeBase; });
        QSignalSpy spy(&servicio, &ServicioGeneracion::finalizada);

        servicio.generar(entradaValida(), QStringLiteral("h"));

        QCOMPARE(servicio.ultimoResultado().estado, Estado::BaseInvalida);
        QCOMPARE(puerto.llamadas, 0);
        QCOMPARE(spy.count(), 1);
        QVERIFY(!servicio.enCurso());
        QCOMPARE(servicio.ultimoResultado().mensaje, mensajeBase);
        QVERIFY(servicio.ultimoResultado().mensaje.contains(QStringLiteral("esquema esperado")));
        QVERIFY(servicio.ultimoResultado().mensaje.contains(QStringLiteral("base nueva")));
    }

    void baseInvalidaTienePrioridadSobreLaEntrada()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0, {}, {}, []() {
            return QStringLiteral("La base de datos no cumple el esquema esperado: falta el tipo "
                                  "general. Restaure un respaldo de la versión vigente o cree "
                                  "una base nueva.");
        });
        QSignalSpy spy(&servicio, &ServicioGeneracion::finalizada);

        // La entrada también es inválida: el diagnóstico de la base tiene prioridad.
        servicio.generar(QJsonObject{}, QStringLiteral("h"));

        QCOMPARE(servicio.ultimoResultado().estado, Estado::BaseInvalida);
        QCOMPARE(puerto.llamadas, 0);
        QCOMPARE(spy.count(), 1);
        QVERIFY(servicio.ultimoResultado().mensaje.contains(QStringLiteral("tipo general")));
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

    void noFactible_conservaElTextoDelMotor()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r;
        r.desenlace = DesenlaceSolver::Infactible;
        r.ok = true;
        r.factible = false;
        r.error = QStringLiteral("Causa (4) conflicto de asignación de docente: la materia «Matemática»"
                                 " de 1|A no tiene ningún docente elegible.");
        puerto.responder(r);

        QCOMPARE(servicio.ultimoResultado().estado, Estado::NoFactible);
        QVERIFY(!servicio.ultimoResultado().presentable);
        QVERIFY(!servicio.ultimoResultado().datosAnteriores);
        // H3 mínimo (RF-2): se conserva el texto del motor, nunca el mensaje genérico.
        QCOMPARE(servicio.ultimoResultado().mensaje, r.error);
        QVERIFY2(!servicio.ultimoResultado().mensaje.contains(
                     QStringLiteral("No existe una solución factible")),
                 qPrintable(servicio.ultimoResultado().mensaje));
    }

    /// RF-2: sin diagnóstico del motor, se declara que no hay causa atribuible (no un texto
    /// genérico de «no existe solución»).
    void noFactibleSinTexto_informaQueNoHayCausaAtribuible()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r;
        r.desenlace = DesenlaceSolver::Infactible;
        r.ok = true;
        r.factible = false;
        puerto.responder(r);

        QCOMPARE(servicio.ultimoResultado().estado, Estado::NoFactible);
        QVERIFY2(servicio.ultimoResultado().mensaje.contains(QStringLiteral("causa atribuible")),
                 qPrintable(servicio.ultimoResultado().mensaje));
    }

    /// RF-1: el NoFactible informa de los docentes sin capacidad, tanto con `horas_aula = 0`
    /// declarado como con el campo ausente, para que no sea un deshabilitado silencioso.
    void docenteSinHorasAula_apareceEnElMensaje()
    {
        for (const bool quitarElCampo : {false, true})
        {
            PuertoFalso puerto;
            ServicioGeneracion servicio(puerto, 0);
            servicio.generar(entradaSinHorasAula(quitarElCampo), QStringLiteral("h"));
            QVERIFY2(servicio.enCurso(),
                     "la falta de capacidad no invalida la entrada (V13 intacta)");

            RespuestaSolver r;
            r.desenlace = DesenlaceSolver::Infactible;
            r.ok = true;
            r.factible = false;
            puerto.responder(r);

            const QString mensaje = servicio.ultimoResultado().mensaje;
            QCOMPARE(servicio.ultimoResultado().estado, Estado::NoFactible);
            QVERIFY2(mensaje.contains(QStringLiteral("causa atribuible")), qPrintable(mensaje));
            QVERIFY2(mensaje.contains(QStringLiteral("sin capacidad")), qPrintable(mensaje));
            QVERIFY2(mensaje.contains(QStringLiteral("Ana")), qPrintable(mensaje));
        }
    }

    /// RF-3: un presupuesto agotado sin prueba es «no concluyente», no NoFactible, y conserva
    /// el texto del motor.
    void noConcluyente_esDistintoDeNoFactible()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r;
        r.desenlace = DesenlaceSolver::NoConcluyente;
        r.ok = true;
        r.factible = false;
        r.error = QStringLiteral("El solver no determinó la factibilidad (UNKNOWN).");
        puerto.responder(r);

        QCOMPARE(servicio.ultimoResultado().estado, Estado::NoConcluyente);
        QVERIFY(!servicio.ultimoResultado().presentable);
        QVERIFY(servicio.ultimoResultado().horario.horarios.isEmpty());
        QCOMPARE(servicio.ultimoResultado().mensaje, r.error);
        QVERIFY2(!servicio.ultimoResultado().mensaje.contains(QStringLiteral("No existe una solución")),
                 qPrintable(servicio.ultimoResultado().mensaje));
    }

    /// RF-3: la pérdida de comunicación es «sin respuesta», no una respuesta inválida ni
    /// NoFactible, y no presenta horario.
    void sinRespuesta_noEsContratoNiNoFactible()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r;
        r.desenlace = DesenlaceSolver::SinRespuesta;
        r.error = QStringLiteral("Se perdió la comunicación con el proceso de cálculo.");
        puerto.responder(r);

        const ResultadoGeneracion& resultado = servicio.ultimoResultado();
        QCOMPARE(resultado.estado, Estado::SinRespuesta);
        QVERIFY(!resultado.presentable);
        QVERIFY(resultado.mensaje.contains(QStringLiteral("perdió la comunicación")));
        QVERIFY2(!resultado.mensaje.contains(QStringLiteral("contrato")),
                 qPrintable(resultado.mensaje));
    }

    /// RF-3: un modelo inválido o una respuesta fuera de contrato se informa como error de
    /// entrada/contrato, con su causa.
    void errorDeContrato_conservaSuCausa()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r;
        r.desenlace = DesenlaceSolver::ErrorContrato;
        r.error = QStringLiteral("El modelo CP-SAT no es válido (MODEL_INVALID).");
        puerto.responder(r);

        QCOMPARE(servicio.ultimoResultado().estado, Estado::ContratoInvalido);
        QVERIFY(!servicio.ultimoResultado().presentable);
        QCOMPARE(servicio.ultimoResultado().mensaje, r.error);
    }

    void contratoInvalido()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        RespuestaSolver r;
        r.desenlace = DesenlaceSolver::Exito;
        r.ok = true;
        r.factible = true;
        r.salida = QJsonObject{};
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

    /// 020 (RF-5): P4 (solapamientos) y P5 (exceso de horas de aula) son fallos distintos; el
    /// mensaje de solapamiento no se usa para un exceso de tope.
    void calculoFallidoPorSolapamiento_noSeConfundeConTope()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        puerto.responder(respuestaConSolapamiento());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.estado, Estado::CalculoFallido);
        QVERIFY(r.analisis.hayConflictoSolapamiento());
        QVERIFY(r.analisis.excesosTopeHoras.isEmpty());
        QVERIFY2(r.mensaje.contains(QStringLiteral("solapamiento")), qPrintable(r.mensaje));
        QVERIFY2(!r.mensaje.contains(QStringLiteral("tope")), qPrintable(r.mensaje));
    }

    /// 020 (RF-5): un exceso de tope de horas de aula no se presenta como horario y su mensaje
    /// es distinto del de solapamiento.
    void calculoFallidoPorTopeDeHorasAula_mensajeDistinto()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        // El docente tiene 0 h de aula declaradas y la salida le asigna 2 clases.
        servicio.generar(entradaSinHorasAula(false), QStringLiteral("h"));
        QVERIFY(servicio.enCurso());

        puerto.responder(respuestaFactible());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.estado, Estado::CalculoFallido);
        QVERIFY(!r.presentable);
        QVERIFY(!r.analisis.excesosTopeHoras.isEmpty());
        QVERIFY(!r.analisis.hayConflictoSolapamiento());
        QVERIFY2(r.mensaje.contains(QStringLiteral("tope")), qPrintable(r.mensaje));
        QVERIFY2(!r.mensaje.contains(QStringLiteral("solapamiento")), qPrintable(r.mensaje));
    }

    /// 020 (RF-5): el resultado lleva el reparto (una línea por docente y totales) para que la
    /// vista lo presente sin recalcular nada.
    void reparto_enElResultado()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        puerto.responder(respuestaFactible());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.analisis.reparto.size(), 1);
        QCOMPARE(r.analisis.reparto[0].horasAsignadas, 2);
        QCOMPARE(r.analisis.reparto[0].horasAula, 8);
        QCOMPARE(r.analisis.totalHorasAsignadas, 2);
        QCOMPARE(r.analisis.totalHorasAula, 8);
        const QString reparto = r.analisis.repartoTexto();
        QVERIFY2(reparto.contains(QStringLiteral("Ana: 2/8 h")), qPrintable(reparto));
        QVERIFY2(reparto.contains(QStringLiteral("Total del centro: 2/8 h")), qPrintable(reparto));
    }

    /// 020 (RF-3, D8): la optimalidad por fase llega en la metadata de la salida y se expone en
    /// el resultado; sin fases marcadas no se califica de no óptima la asignación.
    void fasesNoOptimas_enElResultado()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));
        puerto.responder(respuestaFactibleConFases({QStringLiteral("referencia")}));

        const ResultadoGeneracion& conFases = servicio.ultimoResultado();
        QCOMPARE(conFases.estado, Estado::Listo);
        QVERIFY(conFases.presentable);
        QCOMPARE(conFases.fasesNoOptimas, QStringList{QStringLiteral("referencia")});

        PuertoFalso puerto2;
        ServicioGeneracion servicio2(puerto2, 0);
        servicio2.generar(entradaValida(), QStringLiteral("h"));
        puerto2.responder(respuestaFactible());

        const ResultadoGeneracion& optimo = servicio2.ultimoResultado();
        QCOMPARE(optimo.estado, Estado::Listo);
        QVERIFY(optimo.fasesNoOptimas.isEmpty());
    }

    void ok_presentable()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));

        puerto.responder(respuestaFactible());

        QCOMPARE(servicio.ultimoResultado().estado, Estado::Listo);
        QVERIFY(servicio.ultimoResultado().presentable);

        // 014, RF-12: el informe de bloques es aditivo y se calcula sobre la salida.
        const AnalisisBloques& bloques = servicio.ultimoResultado().bloques;
        QCOMPARE(bloques.bloques.size(), 1);
        QCOMPARE(bloques.bloquesSimples, 1);
        QCOMPARE(bloques.bloquesCompartidos, 0);
        QCOMPARE(bloques.bloquesDeUno, 0);
        QCOMPARE(bloques.desvio, 0);
        QCOMPARE(bloques.materias.size(), 1);
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

    /// 020 (RF-3/RNF-2): con un plazo corto inyectado el desenlace es «plazo agotado» (no
    /// NoFactible) y una respuesta tardía se ignora. No se espera la hora real.
    void plazoCortoInyectado_informaPlazoAgotado()
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
        QVERIFY2(!servicio.ultimoResultado().mensaje.contains(
                     QStringLiteral("No existe una solución")),
                 qPrintable(servicio.ultimoResultado().mensaje));

        // Una respuesta posterior se ignora: el estado no cambia.
        puerto.responder(respuestaFactible());
        QTest::qWait(20);
        QCOMPARE(servicio.ultimoResultado().estado, Estado::TiempoAgotado);
        QCOMPARE(spy.count(), 1);
    }

    /// 020 (RF-3): el plazo por defecto del proceso es de una hora de reloj (60 min).
    void plazoPorDefecto_esUnaHora()
    {
        QCOMPARE(PLAZO_MS_POR_DEFECTO, 60 * 60 * 1000);
        QCOMPARE(PLAZO_MS_POR_DEFECTO, 3600000);
    }

    void huellaDatos_esDeterministaYcambiaConLosDatos()
    {
        const QString h1 = huellaDatos(dominio(), 35, {});
        QCOMPARE(h1, huellaDatos(dominio(), 35, {}));

        DatosDominio modificado = dominio();
        modificado.cursos[0].seccion = QStringLiteral("B");
        QVERIFY(huellaDatos(modificado, 35, {}) != h1);
    }

    void huellaDatos_cambiaConElTipoDeAula()
    {
        const QString h1 = huellaDatos(dominio(), 35, {});

        DatosDominio otraAula = dominio();
        otraAula.aulas[0].idTipoAula = 7;
        QVERIFY(huellaDatos(otraAula, 35, {}) != h1);

        DatosDominio otraMateria = dominio();
        otraMateria.materias[0].idTipoAula = 7;
        QVERIFY(huellaDatos(otraMateria, 35, {}) != h1);
    }

    void huellaDatos_noIncluyeElTipoGeneralNiSuNombreVisible()
    {
        // El nombre visible del tipo no entra en la huella (solo su identidad, `idTipoAula`).
        // Dos dominios que solo difieren en `tipoAula` dan la misma huella; el control
        // `huellaDatos_cambiaConElTipoDeAula` fija que el id sí entra.
        const QString h1 = huellaDatos(dominio(), 35, {});

        DatosDominio otroNombre = dominio();
        otroNombre.aulas[0].tipoAula = QStringLiteral("Otro nombre visible");
        otroNombre.materias[0].tipoAula = QStringLiteral("Otro nombre visible");
        QCOMPARE(huellaDatos(otroNombre, 35, {}), h1);

        // Tampoco entra el nombre por separado en cada colección.
        DatosDominio soloAula = dominio();
        soloAula.aulas[0].tipoAula = QStringLiteral("Aula común");
        QCOMPARE(huellaDatos(soloAula, 35, {}), h1);

        DatosDominio soloMateria = dominio();
        soloMateria.materias[0].tipoAula = QStringLiteral("Aula común");
        QCOMPARE(huellaDatos(soloMateria, 35, {}), h1);
    }

    void huellaDatos_cambiaConLasMateriasSuplentes()
    {
        const QString h1 = huellaDatos(dominio(), 35, {});

        DatosDominio conSuplente = dominio();
        QVERIFY(!conSuplente.docentes.isEmpty());
        MateriaAsignadaDTO suplente;
        suplente.id = 99;
        suplente.nombre = QStringLiteral("Materia suplente");
        conSuplente.docentes[0].materiasSuplentes.append(suplente);

        QVERIFY(huellaDatos(conSuplente, 35, {}) != h1);
    }

    void huellaDatos_cambiaConLosEventos()
    {
        EventoDTO evento;
        evento.id = 3;
        evento.nombre = QStringLiteral("Receso");
        evento.despuesDeSlot = 2;
        evento.orden = 1;
        evento.duracion = 10;
        evento.inicio = QTime(8, 20);
        evento.fin = QTime(8, 30);

        DatosDominio sinEventos = dominio();
        const QString h1 = huellaDatos(sinEventos, 35, {});

        DatosDominio conEvento = dominio();
        conEvento.turnos[0].eventos = {evento};
        const QString h2 = huellaDatos(conEvento, 35, {});
        QVERIFY(h2 != h1);

        // El orden persistente del evento también entra en la huella.
        DatosDominio otroOrden = conEvento;
        otroOrden.turnos[0].eventos[0].orden = 2;
        QVERIFY(huellaDatos(otroOrden, 35, {}) != h2);
    }

    /// La huella incluye la hora académica (RF-6): es estable y distinta con otra D.
    void huellaDatos_incluyeLaDuracion()
    {
        const QString con35 = huellaDatos(dominio(), 35, {});
        QCOMPARE(con35, huellaDatos(dominio(), 35, {}));
        QVERIFY(huellaDatos(dominio(), 45, {}) != con35);
    }

    // ─── Preferencias intervinientes en la huella (013, RF-6) ─────────────

    /// RNF-4 (013): sin intervinientes la huella vale EXACTAMENTE lo mismo que antes de la 013
    /// (golden del algoritmo anterior con el mismo `dominio()` y D = 35).
    void huellaDatos_sinIntervinientes_conservaElValorDeHoy()
    {
        QCOMPARE(huellaDatos(dominio(), 35, {}),
                 QStringLiteral("3c50f3abc8e7c294815cb32bf6e80b8837265950341f6a07fbbe576fd0ed1330"));
    }

    /// RF-6: una preferencia interviniente (ya proyectada) cambia la huella; el sentido y las
    /// franjas también. Es el caso de punta a punta: datos → proyección → huella.
    void huellaDatos_unaPreferenciaIntervinienteCambiaLaHuella()
    {
        const QString base = huellaDatos(dominio(), 35, {});

        DatosDominio conLunes = dominio();
        // Lunes 07:00–09:00 con D = 35 → franjas {0, 1, 2} y día de generación 0.
        agregarPreferenciaAlDominio(conLunes, 0, 1, QTime(7, 0), QTime(9, 0),
                                    SentidoPreferencia::Evitada);

        const auto construida = construirEntradaCompleta(
            conLunes, parametros(), 35, QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0)));
        QVERIFY2(construida.ok, qPrintable(construida.mensajeError));
        QCOMPARE(construida.valor.intervinientes.size(), 1);

        const QString conInterviniente =
            huellaDatos(conLunes, 35, construida.valor.intervinientes);
        QVERIFY(conInterviniente != base);

        // El sentido y las franjas forman parte de la huella.
        const QString preferida = huellaDatos(
            conLunes, 35, {interviniente(0, 0, {0, 1, 2}, SentidoPreferencia::Preferida)});
        QVERIFY(preferida != conInterviniente);
        QVERIFY(huellaDatos(conLunes, 35,
                            {interviniente(0, 0, {0, 1}, SentidoPreferencia::Evitada)}) !=
                conInterviniente);
    }

    /// RF-6: una preferencia que no interviene no entra en la huella. La de domingo se descarta
    /// en la proyección y el llamador pasa la lista vacía (no cambia la huella).
    void huellaDatos_unaPreferenciaNoIntervinienteNoCambiaLaHuella()
    {
        const QString base = huellaDatos(dominio(), 35, {});

        DatosDominio conDomingo = dominio();
        agregarPreferenciaAlDominio(conDomingo, 0, 0, QTime(7, 0), QTime(9, 0),
                                    SentidoPreferencia::Evitada);

        const auto construida = construirEntradaCompleta(
            conDomingo, parametros(), 35, QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0)));
        QVERIFY2(construida.ok, qPrintable(construida.mensajeError));
        QVERIFY(construida.valor.intervinientes.isEmpty());

        QCOMPARE(huellaDatos(conDomingo, 35, construida.valor.intervinientes), base);
    }

    /// RNF-2/RF-6: el orden de las preferencias y el de las franjas dentro de una preferencia
    /// no alteran la huella (se canoniza antes de hashear).
    void huellaDatos_elOrdenDePreferenciasYfranjasNoAlteraLaHuella()
    {
        const PreferenciaProyectada a =
            interviniente(0, 1, {0, 3}, SentidoPreferencia::Evitada);
        const PreferenciaProyectada b =
            interviniente(1, 2, {5}, SentidoPreferencia::Preferida);

        const QString h1 = huellaDatos(dominio(), 35, {a, b});
        QCOMPARE(huellaDatos(dominio(), 35, {b, a}), h1);
        QVERIFY(h1 != huellaDatos(dominio(), 35, {}));

        const PreferenciaProyectada desordenada =
            interviniente(0, 1, {3, 0}, SentidoPreferencia::Evitada);
        QCOMPARE(huellaDatos(dominio(), 35, {desordenada, b}), h1);
    }

    /// RF-6: un cambio de una preferencia INTERVINIENTE entre el envío y la respuesta marca
    /// «datos anteriores».
    void datosAnteriores_siCambiaUnaPreferenciaInterviniente()
    {
        const DatosDominio d = dominio();
        const QVector<PreferenciaProyectada> intervinientes{
            interviniente(0, 0, {0, 1}, SentidoPreferencia::Evitada)};

        PuertoFalso puerto;
        ServicioGeneracion servicio(
            puerto, 0, [d, intervinientes]() { return huellaDatos(d, 35, intervinientes); });
        servicio.generar(entradaValida(), huellaDatos(d, 35, {}));

        puerto.responder(respuestaFactible());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.estado, Estado::DatosAnteriores);
        QVERIFY(r.datosAnteriores);
    }

    // ─── Estado por preferencia y cumplimiento (013, RF-8) ─────────────────

    void cumplimiento_evitadaRespetada()
    {
        SolverConfig config;
        config.preferencias = {preferencia(0, 0, {2, 3}, QStringLiteral("evitada"))};
        const HorarioSalida horario = horarioDosClases();   // clases en (día 0, slots 0 y 1)

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(config, horario, {});

        QCOMPARE(cumplimiento.porPreferencia.size(), 1);
        const PreferenciaEstado& estado = cumplimiento.porPreferencia[0];
        QCOMPARE(estado.estado, EstadoPreferencia::Respetada);
        QCOMPARE(estado.motivo, MotivoPreferencia::Ninguno);
        QCOMPARE(estado.docente, 0);
        QCOMPARE(estado.dia, 1);   // generación 0 = lunes → dominio 1
        QCOMPARE(estado.slot, (QVector<int>{2, 3}));
        QCOMPARE(estado.sentido, QStringLiteral("evitada"));
        QVERIFY(estado.clasesIncumplidas.isEmpty());
        QCOMPARE(cumplimiento.respetadas, 1);
        QCOMPARE(cumplimiento.incumplidas, 0);
        QCOMPARE(cumplimiento.noAplicadas, 0);
    }

    void cumplimiento_evitadaIncumplidaListaLasClasesOcupadas()
    {
        SolverConfig config;
        config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("evitada"))};
        const HorarioSalida horario = horarioDosClases();

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(config, horario, {});

        QCOMPARE(cumplimiento.porPreferencia.size(), 1);
        const PreferenciaEstado& estado = cumplimiento.porPreferencia[0];
        QCOMPARE(estado.estado, EstadoPreferencia::Incumplida);
        QCOMPARE(estado.motivo, MotivoPreferencia::Ninguno);   // solo las no aplicadas llevan motivo
        QCOMPARE(estado.clasesIncumplidas.size(), 2);
        // Lista de clases incumplidas: curso ("anio|seccion"), materia y slot, en orden.
        QCOMPARE(estado.clasesIncumplidas[0].curso, claveCurso(1, QStringLiteral("A")));
        QCOMPARE(estado.clasesIncumplidas[0].materia, 0);
        QCOMPARE(estado.clasesIncumplidas[0].slot, 0);
        QCOMPARE(estado.clasesIncumplidas[1].slot, 1);
        QCOMPARE(cumplimiento.incumplidas, 1);
        QCOMPARE(cumplimiento.respetadas, 0);
    }

    void cumplimiento_evitadaParcialmenteOcupadaEsIncumplida()
    {
        // Se evitan {0, 1}; solo la franja 0 está ocupada: sigue siendo incumplida.
        SolverConfig config;
        config.preferencias = {preferencia(0, 0, {0, 5}, QStringLiteral("evitada"))};

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(config, horarioDosClases(), {});

        QCOMPARE(cumplimiento.porPreferencia[0].estado, EstadoPreferencia::Incumplida);
        QCOMPARE(cumplimiento.porPreferencia[0].clasesIncumplidas.size(), 1);
        QCOMPARE(cumplimiento.porPreferencia[0].clasesIncumplidas[0].slot, 0);
    }

    void cumplimiento_preferidaSinClaseEsIncumplida()
    {
        SolverConfig config;
        config.preferencias = {preferencia(0, 0, {4, 5}, QStringLiteral("preferida"))};

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(config, horarioDosClases(), {});

        QCOMPARE(cumplimiento.porPreferencia[0].estado, EstadoPreferencia::Incumplida);
        QCOMPARE(cumplimiento.porPreferencia[0].motivo, MotivoPreferencia::Ninguno);
        // Una preferida sin clases no tiene clases que listar.
        QVERIFY(cumplimiento.porPreferencia[0].clasesIncumplidas.isEmpty());
        QCOMPARE(cumplimiento.incumplidas, 1);
    }

    void cumplimiento_preferidaConClaseEsRespetada()
    {
        SolverConfig config;
        config.preferencias = {preferencia(0, 0, {1}, QStringLiteral("preferida"))};

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(config, horarioDosClases(), {});

        QCOMPARE(cumplimiento.porPreferencia[0].estado, EstadoPreferencia::Respetada);
        QVERIFY(cumplimiento.porPreferencia[0].clasesIncumplidas.isEmpty());
        QCOMPARE(cumplimiento.respetadas, 1);
    }

    void cumplimiento_soloCuentanLasClasesDelPropioDocente()
    {
        // El docente 1 no da clase: su evitada sobre la franja ocupada por el docente 0 se respeta.
        SolverConfig config;
        config.preferencias = {preferencia(1, 0, {0, 1}, QStringLiteral("evitada"))};

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(config, horarioDosClases(), {});

        QCOMPARE(cumplimiento.porPreferencia[0].estado, EstadoPreferencia::Respetada);
    }

    void cumplimiento_noAplicadasConservanSuMotivo()
    {
        const QVector<PreferenciaNoAplicada> noAplicadas = {
            noAplicada(0, 0, SentidoPreferencia::Evitada, MotivoPreferencia::DiaFueraDeSemana),
            noAplicada(0, 3, SentidoPreferencia::Preferida, MotivoPreferencia::SinFranjaDelTurno),
            noAplicada(1, 2, SentidoPreferencia::Evitada, MotivoPreferencia::DocenteNoParticipa)};

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(SolverConfig{}, HorarioSalida{}, noAplicadas);

        QCOMPARE(cumplimiento.porPreferencia.size(), 3);
        // Orden canónico por (docente, día, sentido, franjas).
        QCOMPARE(cumplimiento.porPreferencia[0].docente, 0);
        QCOMPARE(cumplimiento.porPreferencia[0].dia, 0);   // ya es día de DOMINIO
        QCOMPARE(cumplimiento.porPreferencia[0].motivo, MotivoPreferencia::DiaFueraDeSemana);
        QCOMPARE(cumplimiento.porPreferencia[0].sentido, QStringLiteral("evitada"));
        QVERIFY(cumplimiento.porPreferencia[0].slot.isEmpty());
        QCOMPARE(cumplimiento.porPreferencia[1].dia, 3);
        QCOMPARE(cumplimiento.porPreferencia[1].motivo, MotivoPreferencia::SinFranjaDelTurno);
        QCOMPARE(cumplimiento.porPreferencia[2].docente, 1);
        QCOMPARE(cumplimiento.porPreferencia[2].motivo, MotivoPreferencia::DocenteNoParticipa);
        QCOMPARE(cumplimiento.noAplicadas, 3);
        QCOMPARE(cumplimiento.respetadas, 0);
        QCOMPARE(cumplimiento.incumplidas, 0);
    }

    void cumplimiento_totalesYordenCanonico()
    {
        SolverConfig config;
        // Declaradas fuera de orden: el resultado se canoniza por (docente, dia, sentido, slot).
        config.preferencias = {
            preferencia(0, 2, {4}, QStringLiteral("preferida")),   // incumplida (sin clase)
            preferencia(0, 0, {0}, QStringLiteral("evitada")),     // incumplida (clase)
            preferencia(0, 1, {2}, QStringLiteral("evitada"))};    // respetada

        const QVector<PreferenciaNoAplicada> noAplicadas = {
            noAplicada(0, 4, SentidoPreferencia::Evitada, MotivoPreferencia::SinFranjaDelTurno)};

        const CumplimientoPreferencias cumplimiento =
            analizarPreferencias(config, horarioDosClases(), noAplicadas);

        QCOMPARE(cumplimiento.porPreferencia.size(), 4);
        QCOMPARE(cumplimiento.respetadas, 1);
        QCOMPARE(cumplimiento.incumplidas, 2);
        QCOMPARE(cumplimiento.noAplicadas, 1);
        // Orden: día 0 evitada, día 1 evitada, día 2 preferida, día 4 (no aplicada).
        QCOMPARE(cumplimiento.porPreferencia[0].dia, 1);   // generación 0 → dominio 1
        QCOMPARE(cumplimiento.porPreferencia[0].sentido, QStringLiteral("evitada"));
        QCOMPARE(cumplimiento.porPreferencia[1].dia, 2);   // generación 1 → dominio 2
        QCOMPARE(cumplimiento.porPreferencia[2].dia, 3);   // generación 2 → dominio 3
        QCOMPARE(cumplimiento.porPreferencia[3].dia, 4);   // no aplicada (dominio)
        QCOMPARE(cumplimiento.porPreferencia[3].estado, EstadoPreferencia::NoAplicada);
    }

    /// RF-8: el servicio expone el cumplimiento en su resultado, con las no aplicadas que recibe.
    void servicio_cumplimientoEnElResultado()
    {
        QJsonObject entrada = entradaValida();
        // El horario de prueba da clases del docente 0 en (día 0, slots 0 y 1).
        entrada[QStringLiteral("preferencias")] = QJsonArray{
            QJsonObject{{QStringLiteral("docente"), 0},
                        {QStringLiteral("dia"), 0},
                        {QStringLiteral("slot"), QJsonArray{0}},
                        {QStringLiteral("sentido"), QStringLiteral("evitada")}}};

        const QVector<PreferenciaNoAplicada> noAplicadas = {
            noAplicada(0, 0, SentidoPreferencia::Evitada, MotivoPreferencia::DiaFueraDeSemana)};

        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entrada, QStringLiteral("h"), noAplicadas);
        puerto.responder(respuestaFactible());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.estado, Estado::Listo);
        QCOMPARE(r.cumplimiento.porPreferencia.size(), 2);
        QCOMPARE(r.cumplimiento.incumplidas, 1);
        QCOMPARE(r.cumplimiento.noAplicadas, 1);
        QCOMPARE(r.cumplimiento.respetadas, 0);
        // Orden canónico por día: la no aplicada (dominio 0) va antes que la interviniente
        // (generación 0 → dominio 1).
        QCOMPARE(r.cumplimiento.porPreferencia[0].estado, EstadoPreferencia::NoAplicada);
        QCOMPARE(r.cumplimiento.porPreferencia[0].dia, 0);
        QCOMPARE(r.cumplimiento.porPreferencia[1].estado, EstadoPreferencia::Incumplida);
        QCOMPARE(r.cumplimiento.porPreferencia[1].dia, 1);
        QCOMPARE(r.cumplimiento.porPreferencia[1].clasesIncumplidas.size(), 1);
        QCOMPARE(r.cumplimiento.porPreferencia[1].clasesIncumplidas[0].slot, 0);
        QCOMPARE(r.cumplimiento.porPreferencia[1].clasesIncumplidas[0].curso,
                 claveCurso(1, QStringLiteral("A")));
    }

    /// Sin preferencias ni diagnóstico, el cumplimiento queda vacío (aditivo, sin ruido).
    void servicio_sinPreferencias_cumplimientoVacio()
    {
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0);
        servicio.generar(entradaValida(), QStringLiteral("h"));
        puerto.responder(respuestaFactible());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.estado, Estado::Listo);
        QVERIFY(r.cumplimiento.porPreferencia.isEmpty());
        QCOMPARE(r.cumplimiento.respetadas, 0);
        QCOMPARE(r.cumplimiento.incumplidas, 0);
        QCOMPARE(r.cumplimiento.noAplicadas, 0);
    }

    /// Un cambio de D entre el envío y la respuesta marca «datos anteriores» (RF-6).
    void datosAnteriores_siCambiaLaDuracionEntreEnvioYRespuesta()
    {
        const DatosDominio d = dominio();
        PuertoFalso puerto;
        ServicioGeneracion servicio(puerto, 0, [d]() { return huellaDatos(d, 45, {}); });
        servicio.generar(entradaValida(), huellaDatos(d, 35, {}));

        puerto.responder(respuestaFactible());

        const ResultadoGeneracion& r = servicio.ultimoResultado();
        QCOMPARE(r.estado, Estado::DatosAnteriores);
        QVERIFY(r.datosAnteriores);
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
