#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTime>
#include <QTest>

#include <backend/data/materia.hpp>
#include <backend/data/plan_estudio.hpp>
#include <backend/services/ConstructorEntradaSolver.hpp>
#include <backend/services/ValidadorEntradaSolver.hpp>

/// Tests de la validación previa del JSON de entrada (V1–V12, RF-1).
///
/// El JSON válido se construye con `construirJsonEntrada` (T2) y luego se muta
/// una sección por caso para provocar cada regla.
class TestValidadorEntrada : public QObject
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
        MateriaEnPlan mp; mp.curso = 1; mp.horas = 5; mp.materia = mPlan;
        PlanEstudio plan; plan.codigo = QStringLiteral("Ciencias"); plan.nombre = QStringLiteral("Ciencias");
        plan.materias = {mp};
        d.planes = {plan};

        FranjaHoraria franja; franja.dia = 1; franja.inicio = QTime(7, 0); franja.fin = QTime(8, 0);
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

    QJsonObject jsonValido() const
    {
        const auto resultado = construirJsonEntrada(dominio(), parametros(),
                                                    QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0)));
        const bool ok = QTest::qVerify(resultado.ok, "construirJsonEntrada(dominio())",
                                       qPrintable(resultado.mensajeError), __FILE__, __LINE__);
        Q_UNUSED(ok);
        return resultado.valor;
    }

    /// Fija el materia_idx de la primera materia del primer plan embebido.
    void ponerMateriaIdxPlan(QJsonObject& j, int valor) const
    {
        QJsonArray planes = j[QStringLiteral("planes")].toArray();
        QJsonObject plan = planes[0].toObject();
        QJsonArray mats = plan[QStringLiteral("materias")].toArray();
        QJsonObject m = mats[0].toObject();
        m[QStringLiteral("materia_idx")] = valor;
        mats[0] = m;
        plan[QStringLiteral("materias")] = mats;
        planes[0] = plan;
        j[QStringLiteral("planes")] = planes;
    }

    /// Fija las horas_semanales de la primera materia del primer plan embebido.
    void ponerHorasPlan(QJsonObject& j, int horas) const
    {
        QJsonArray planes = j[QStringLiteral("planes")].toArray();
        QJsonObject plan = planes[0].toObject();
        QJsonArray mats = plan[QStringLiteral("materias")].toArray();
        QJsonObject m = mats[0].toObject();
        m[QStringLiteral("horas_semanales")] = horas;
        mats[0] = m;
        plan[QStringLiteral("materias")] = mats;
        planes[0] = plan;
        j[QStringLiteral("planes")] = planes;
    }

    /// Reconfigura la entrada para que el curso tenga exactamente 40 franjas útiles
    /// (num_dias = 5 × turno de 8 franjas). Sirve de frontera para V14 (40 pasa, 41 falla).
    QJsonObject conFranjasUtiles40(QJsonObject j) const
    {
        QJsonObject dim = j[QStringLiteral("dimensiones")].toObject();
        dim[QStringLiteral("num_dias")] = 5;
        dim[QStringLiteral("num_slots_dia")] = 8;
        j[QStringLiteral("dimensiones")] = dim;

        QJsonObject turnos = j[QStringLiteral("turnos")].toObject();
        QJsonArray arrTurnos = turnos[QStringLiteral("turnos")].toArray();
        QJsonObject manana = arrTurnos[0].toObject();
        QJsonArray franjas;
        for (int s = 0; s < 8; ++s) franjas.append(s);
        manana[QStringLiteral("slot")] = franjas;
        arrTurnos[0] = manana;
        turnos[QStringLiteral("turnos")] = arrTurnos;
        j[QStringLiteral("turnos")] = turnos;
        return j;
    }

    /// Devuelve el JSON con el primer curso modificado por `f`.
    template <typename F>
    QJsonObject conCurso(QJsonObject j, F f) const
    {
        QJsonArray cursos = j["cursos"].toArray();
        QJsonObject c = cursos[0].toObject();
        f(c);
        cursos[0] = c;
        j["cursos"] = cursos;
        return j;
    }

    /// Devuelve el JSON con el primer profesor modificado por `f`.
    template <typename F>
    QJsonObject conProfesor(QJsonObject j, F f) const
    {
        QJsonArray profesores = j["profesores"].toArray();
        QJsonObject p = profesores[0].toObject();
        f(p);
        profesores[0] = p;
        j["profesores"] = profesores;
        return j;
    }

    void ponerMateriaIdxCurso(QJsonObject& curso, int valor) const
    {
        QJsonArray materias = curso["materias"].toArray();
        if (materias.isEmpty())
            materias.append(QJsonObject{{QStringLiteral("materia_idx"), 0},
                                        {QStringLiteral("horas_semanales"), 5}});
        QJsonObject m = materias[0].toObject();
        m["materia_idx"] = valor;
        materias[0] = m;
        curso["materias"] = materias;
    }

private slots:

    void jsonValido_pasa()
    {
        const auto resultado = validarEntradaSolver(jsonValido());
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    void v1_seccionRequeridaAusente_falla()
    {
        QJsonObject j = jsonValido();
        j.remove(QStringLiteral("cursos"));
        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }

    void v3_dimensionesNoCoinciden_falla()
    {
        QJsonObject j = jsonValido();
        QJsonObject dim = j["dimensiones"].toObject();
        dim["num_profesores"] = 2;   // hay 1
        j["dimensiones"] = dim;
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v4_indiceMateriaFueraDeRango_falla()
    {
        QJsonObject j = jsonValido(); ponerMateriaIdxPlan(j, 9);
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v7_turnoInvalido_falla()
    {
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) {
            c["turno"] = QStringLiteral("noche");
        });
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v8_aulaDeCursoFueraDeRango_falla()
    {
        // Modo aula fija activo: el aula del curso debe ser un indice valido.
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) { c["aula"] = 9; });
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v9_numeroEstudiantesNegativo_falla()
    {
        // num_estudiantes presente debe ser positivo. -1 es el centinela de AUSENTE,
        // asi que el valor negativo probado es distinto de -1.
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) { c["num_estudiantes"] = -5; });
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v9_numeroEstudiantesCero_falla()
    {
        // Presente pero NO positivo: la forma exigida es entero > 0 (RF-11, V9).
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) { c["num_estudiantes"] = 0; });
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, "num_estudiantes presente y no positivo debe rechazarse");
    }

    void v9_numeroEstudiantesPositivo_pasa()
    {
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) { c["num_estudiantes"] = 25; });
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    void v9_numeroEstudiantesAusente_pasa()
    {
        // Ausente (centinela -1): el chequeo de forma se omite y la entrada no se invalida.
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) {
            c.remove(QStringLiteral("num_estudiantes"));
        });
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    void coherencia_horasAulaMayorQueRequeridas_falla()
    {
        // Coherencia de datos: horas_aula <= horas_requeridas cuando horas_requeridas esta presente.
        QJsonObject j = conProfesor(jsonValido(), [](QJsonObject& p) {
            p["horas_requeridas"] = 8;
            p["horas_aula"]       = 12;   // 12 > 8
        });
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, "horas_aula > horas_requeridas debe rechazarse (RF-11)");
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("Ana")));
    }

    void coherencia_horasRequeridasAusente_pasa()
    {
        // horas_requeridas ausente (centinela -1): el chequeo se omite aunque horas_aula sea alta.
        QJsonObject j = conProfesor(jsonValido(), [](QJsonObject& p) {
            p.remove(QStringLiteral("horas_requeridas"));
            p["horas_aula"] = 30;
        });
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    void v10_profesorSinMaterias_falla()
    {
        QJsonObject j = conProfesor(jsonValido(), [](QJsonObject& p) {
            p["materias_asignadas"] = QJsonArray{};
            p["materias_suplente"]  = QJsonArray{};
        });
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v11_cursoSinPlan_falla()
    {
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) { c["plan"] = QString(); });
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v12_disponibilidadFueraDeRango_falla()
    {
        QJsonObject j = conProfesor(jsonValido(), [](QJsonObject& p) {
            QJsonArray disp = p["disponibilidad"].toArray();
            QJsonObject d0 = disp[0].toObject();
            d0["dia"] = 9;
            disp[0] = d0;
            p["disponibilidad"] = disp;
        });
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    void v13_materiaSinProfesor_falla()
    {
        QJsonObject j = conProfesor(jsonValido(), [](QJsonObject& p) {
            p["materias_asignadas"] = QJsonArray{};
            p["materias_suplente"] = QJsonArray{};
        });
        QVERIFY(!validarEntradaSolver(j).ok);
    }

    /// V13 debe cruzar la disponibilidad del docente con las franjas del turno del curso:
    /// un candidato cuyo rango no se solapa con el turno del curso no es elegible.
    void v13_sinDisponibilidadEnElTurno_falla()
    {
        QJsonObject j = jsonValido();

        // Se añade un turno 'tarde' (slots 6-11) y se amplía el día a 12 slots, de modo que
        // el único candidato pueda declarar disponibilidad FUERA del turno del curso (mañana).
        QJsonObject dim = j["dimensiones"].toObject();
        dim["num_slots_dia"] = 12;
        j["dimensiones"] = dim;

        QJsonObject turnos = j["turnos"].toObject();
        QJsonArray arrTurnos = turnos["turnos"].toArray();
        QJsonObject tarde;
        tarde["nombre"] = QStringLiteral("tarde");
        QJsonArray slotsTarde;
        for (int s = 6; s <= 11; ++s) slotsTarde.append(s);
        tarde["slot"] = slotsTarde;
        arrTurnos.append(tarde);
        turnos["turnos"] = arrTurnos;
        j["turnos"] = turnos;

        // El único candidato solo está disponible en la tarde; el curso es de mañana.
        j = conProfesor(j, [](QJsonObject& p) {
            QJsonArray disp = p["disponibilidad"].toArray();
            QJsonObject d0 = disp[0].toObject();
            d0["slot"] = QJsonArray{6, 7};
            disp[0] = d0;
            p["disponibilidad"] = disp;
        });

        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, qPrintable(resultado.mensajeError));
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("V13")));
    }

    /// V14: un curso cuyas horas académicas superan sus franjas útiles se rechaza.
    /// 1 hora académica = 1 bloque (sin ×60 ni redondeo): 41 h > 40 franjas útiles.
    void v14_masHorasQueFranjas_falla()
    {
        QJsonObject j = conFranjasUtiles40(jsonValido());
        ponerHorasPlan(j, 41);
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, "41 h académicas en 40 franjas útiles debe rechazarse (V14)");
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("V14")));
    }

    /// V14: 40 h académicas caben exactamente en 40 franjas útiles (1 hora = 1 bloque).
    /// Con la conversión antigua (ceil(40×60/45) = 54 bloques) esto se rechazaría.
    void v14_horasDentroDeFranjas_pasa()
    {
        QJsonObject j = conFranjasUtiles40(jsonValido());
        ponerHorasPlan(j, 40);
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    void jsonVacio_falla()
    {
        const auto resultado = validarEntradaSolver(QJsonObject{});
        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }
};

QTEST_MAIN(TestValidadorEntrada)
#include "test_validador_entrada.moc"
