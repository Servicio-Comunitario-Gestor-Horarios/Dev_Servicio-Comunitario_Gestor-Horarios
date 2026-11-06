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
        const auto resultado = construirJsonEntrada(dominio(), parametros(), 35,
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
        // 8 franjas × 35 min = 280: la ventana debe llegar a las 11:40 para encajar (RF-4).
        manana[QStringLiteral("inicio")] = QStringLiteral("07:00");
        manana[QStringLiteral("fin")] = QStringLiteral("11:40");
        arrTurnos[0] = manana;
        turnos[QStringLiteral("turnos")] = arrTurnos;
        j[QStringLiteral("turnos")] = turnos;
        return j;
    }

    /// Devuelve el JSON con la duración de la hora académica fijada en `minutos`.
    QJsonObject conDuracion(QJsonObject j, int minutos) const
    {
        QJsonObject franja = j[QStringLiteral("franja_horaria")].toObject();
        franja[QStringLiteral("duracion_minutos")] = minutos;
        j[QStringLiteral("franja_horaria")] = franja;
        return j;
    }

    /// Devuelve el JSON con el primer turno (`manana`) en la ventana indicada.
    QJsonObject conVentanaDelTurno(QJsonObject j, const QString& inicio,
                                   const QString& fin) const
    {
        QJsonObject turnos = j[QStringLiteral("turnos")].toObject();
        QJsonArray arrTurnos = turnos[QStringLiteral("turnos")].toArray();
        QJsonObject manana = arrTurnos[0].toObject();
        manana[QStringLiteral("inicio")] = inicio;
        manana[QStringLiteral("fin")] = fin;
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

    /// Devuelve el JSON con la sección `eventos` sustituida por `eventos`.
    QJsonObject conEventos(QJsonObject j, const QJsonArray& eventos) const
    {
        j[QStringLiteral("eventos")] = eventos;
        return j;
    }

    /// Devuelve el JSON con la sección `aulas` sustituida (y V3 coherente).
    QJsonObject conAulas(QJsonObject j, const QJsonArray& aulas) const
    {
        j[QStringLiteral("aulas")] = aulas;
        QJsonObject dim = j[QStringLiteral("dimensiones")].toObject();
        dim[QStringLiteral("num_aulas")] = aulas.size();
        j[QStringLiteral("dimensiones")] = dim;
        return j;
    }

    /// Devuelve el JSON con la sección `materias` sustituida (y V3 coherente).
    QJsonObject conMaterias(QJsonObject j, const QJsonArray& materias) const
    {
        j[QStringLiteral("materias")] = materias;
        QJsonObject dim = j[QStringLiteral("dimensiones")].toObject();
        dim[QStringLiteral("num_materias")] = materias.size();
        j[QStringLiteral("dimensiones")] = dim;
        return j;
    }

    /// Evento del turno de la mañana (`dominio()`: 07:00–12:00, 6 franjas).
    ///
    /// Las horas son opcionales: si se omiten, el evento no declara horario (par ausente).
    QJsonObject evento(const QString& nombre, int posicion, int orden, int duracion,
                       const QString& inicio = QString(), const QString& fin = QString()) const
    {
        QJsonObject e;
        e[QStringLiteral("nombre")]           = nombre;
        e[QStringLiteral("turno")]            = QStringLiteral("manana");
        e[QStringLiteral("despues_de_slot")]  = posicion;
        e[QStringLiteral("orden")]            = orden;
        e[QStringLiteral("duracion")]         = duracion;
        if (!inicio.isEmpty())
            e[QStringLiteral("inicio")] = inicio;
        if (!fin.isEmpty())
            e[QStringLiteral("fin")] = fin;
        return e;
    }

    /// Exige que la entrada se rechace por V2 y que el mensaje contenga `fragmento`.
    void exigeRechazoV2(const QJsonObject& j, const QString& fragmento) const
    {
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.startsWith(QStringLiteral("V2:")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(fragmento), qPrintable(resultado.mensajeError));
    }

    /// Exige que la entrada se rechace por V16 y que el mensaje nombre `entidad`.
    void exigeRechazoV16(const QJsonObject& j, const QString& entidad) const
    {
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.startsWith(QStringLiteral("V16:")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(entidad), qPrintable(resultado.mensajeError));
    }

    /// Exige que la entrada se rechace por V17 nombrando el tipo, la materia y el curso.
    void exigeRechazoV17(const QJsonObject& j, const QString& tipo, const QString& materia,
                         const QString& curso) const
    {
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.startsWith(QStringLiteral("V17:")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(tipo), qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(materia), qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(curso), qPrintable(resultado.mensajeError));
    }

    /// Exige que la entrada se rechace por V18 nombrando el aula, el curso y el tipo, y con la
    /// acción disponible (asignar al curso un aula del tipo general).
    void exigeRechazoV18(const QJsonObject& j, const QString& aula, const QString& curso,
                         const QString& tipo) const
    {
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(!resultado.ok, qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.startsWith(QStringLiteral("V18:")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(aula), qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(curso), qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(tipo), qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("tipo general")),
                 qPrintable(resultado.mensajeError));
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

    /// V1 (010): la sección esperada es `eventos`; si falta, se rechaza nombrándola.
    void v1_seccionEventosAusente_falla()
    {
        QJsonObject j = jsonValido();
        j.remove(QStringLiteral("eventos"));
        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.startsWith(QStringLiteral("V1:")));
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("eventos")));
    }

    /// V1 (010): `recesos` ya no es la sección esperada. Una entrada con `recesos` y sin
    /// `eventos` se rechaza por la sección de eventos, no por la de recesos.
    void v1_recesosEnVezDeEventos_falla()
    {
        QJsonObject j = jsonValido();
        j.remove(QStringLiteral("eventos"));
        QJsonObject receso;
        receso[QStringLiteral("nombre")]          = QStringLiteral("Receso");
        receso[QStringLiteral("turno")]           = QStringLiteral("manana");
        receso[QStringLiteral("despues_de_slot")] = 2;
        receso[QStringLiteral("duracion")]        = 10;
        j[QStringLiteral("recesos")] = QJsonArray{receso};

        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("eventos")));
        QVERIFY(!resultado.mensajeError.contains(QStringLiteral("recesos")));
    }

    /// V1: la versión se comprueba antes que las secciones; sin versión no se acepta.
    void v1_versionAusente_falla()
    {
        QJsonObject j = jsonValido();
        j.remove(QStringLiteral("version"));
        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("versión")));
    }

    /// V1: un 1.0 se rechaza por versión (se espera 2.0), aunque traiga `eventos`.
    void v1_version1_0_falla()
    {
        QJsonObject j = jsonValido();
        j[QStringLiteral("version")] = QStringLiteral("1.0");
        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("1.0")));
        QVERIFY(resultado.mensajeError.contains(QStringLiteral("2.0")));
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

    void v8_aulaFijaActivaSinAula_falla()
    {
        // Rama activa de V8: con aula fija, el curso sin `aula` (ausente = -1) se rechaza.
        // El caso de índice fuera de rango de arriba lo juzga V6, no V8.
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) {
            c.remove(QStringLiteral("aula"));
        });

        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("V8")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("no declara aula")),
                 qPrintable(resultado.mensajeError));
    }

    void v7_turnoAusente_falla()
    {
        // Un curso sin turno no puede ubicarse en la rejilla (V7).
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) {
            c[QStringLiteral("turno")] = QString();
        });

        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("V7")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("no declara turno")),
                 qPrintable(resultado.mensajeError));
    }

    void v8_modoMovilSinAulaDeCurso_pasa()
    {
        // Con aula fija inactiva el aula del curso se ignora: puede estar ausente.
        QJsonObject j = jsonValido();
        j[QStringLiteral("aula_fija")] = false;
        j = conCurso(j, [](QJsonObject& c) { c.remove(QStringLiteral("aula")); });

        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    void v15_movilSinAulasEnElCentro_falla()
    {
        // Modo móvil exige al menos un aula en el centro (el aula del curso no aplica: V8).
        QJsonObject j = jsonValido();
        j[QStringLiteral("aula_fija")] = false;
        j = conCurso(j, [](QJsonObject& c) { c.remove(QStringLiteral("aula")); });
        j = conAulas(j, QJsonArray{});

        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("V15")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("sin aulas")),
                 qPrintable(resultado.mensajeError));
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
    void v13_d13_docenteDeOtroAnioNoEsElegible_falla()
    {
        // D13: `anios` restringe la elegibilidad al año del curso. El único docente del
        // fixture pasa a dar solo 2.º, así que la materia de 1.º se queda sin candidato (V13).
        QJsonObject j = conProfesor(jsonValido(), [](QJsonObject& p) {
            p[QStringLiteral("anios")] = QJsonArray{2};
        });

        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("V13")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("candidato elegible")),
                 qPrintable(resultado.mensajeError));
    }

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

    // ─── V2: forma y temporalidad de los eventos (010) ─────────────────────

    /// V2: un evento con horario coherente y dentro de la ventana de su posición pasa.
    /// Turno 07:00–12:00 con 6 franjas de 35 min: la posición 2 abarca 08:10–08:20.
    void v2_eventoConHorarioValido_pasa()
    {
        const QJsonObject j = conEventos(
            jsonValido(), QJsonArray{evento(QStringLiteral("Receso"), 2, 1, 10,
                                            QStringLiteral("08:10"), QStringLiteral("08:20"))});
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    /// V2: el par inicio/fin es opcional; ausente, no hay intervalo que comprobar.
    void v2_eventoSinHorario_pasa()
    {
        const QJsonObject j = conEventos(
            jsonValido(), QJsonArray{evento(QStringLiteral("Acto cívico"), 2, 1, 15)});
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    void v2_eventoSinNombre_falla()
    {
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QString(), 2, 1, 10)}),
                       QStringLiteral("nombre"));
    }

    void v2_eventoTurnoInexistente_falla()
    {
        QJsonObject e = evento(QStringLiteral("Receso"), 2, 1, 10);
        e[QStringLiteral("turno")] = QStringLiteral("noche");
        exigeRechazoV2(conEventos(jsonValido(), QJsonArray{e}), QStringLiteral("noche"));
    }

    void v2_eventoPosicionCero_falla()
    {
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 0, 1, 10)}),
                       QStringLiteral("posición 0"));
    }

    void v2_eventoPosicionFueraDeRango_falla()
    {
        // El turno tiene 6 franjas: la posición 7 está fuera (1..6).
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 7, 1, 10)}),
                       QStringLiteral("posición 7"));
    }

    void v2_eventoOrdenCero_falla()
    {
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 2, 0, 10)}),
                       QStringLiteral("orden 0"));
    }

    void v2_eventoDuracionCero_falla()
    {
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 2, 1, 0)}),
                       QStringLiteral("duración 0"));
    }

    void v2_eventoParIncompleto_falla()
    {
        // Solo inicio: el par debe estar completo o ausente.
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 2, 1, 10,
                                                    QStringLiteral("08:10"))}),
                       QStringLiteral("inicio/fin"));
    }

    void v2_eventoFinNoPosteriorAlInicio_falla()
    {
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 2, 1, 10,
                                                    QStringLiteral("08:40"),
                                                    QStringLiteral("08:40"))}),
                       QStringLiteral("posterior"));
    }

    void v2_eventoDuracionNoCoincideConLasHoras_falla()
    {
        // 08:10–08:20 son 10 min; la duración declara 9.
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 2, 1, 9,
                                                    QStringLiteral("08:10"),
                                                    QStringLiteral("08:20"))}),
                       QStringLiteral("no coincide"));
    }

    void v2_eventoFueraDeLaVentanaDeSuPosicion_falla()
    {
        // La posición 2 abarca 08:10–08:20; el intervalo 08:00–08:10 es anterior.
        exigeRechazoV2(conEventos(jsonValido(),
                                  QJsonArray{evento(QStringLiteral("Receso"), 2, 1, 10,
                                                    QStringLiteral("08:00"),
                                                    QStringLiteral("08:10"))}),
                       QStringLiteral("ventana"));
    }

    /// V2: un evento con horario exige que su turno declare la ventana inicio/fin.
    void v2_eventoConHorarioYturnoSinVentana_falla()
    {
        QJsonObject j = conEventos(
            jsonValido(), QJsonArray{evento(QStringLiteral("Receso"), 2, 1, 10,
                                            QStringLiteral("08:10"), QStringLiteral("08:20"))});
        QJsonObject turnos = j[QStringLiteral("turnos")].toObject();
        QJsonArray arrTurnos = turnos[QStringLiteral("turnos")].toArray();
        QJsonObject manana = arrTurnos[0].toObject();
        manana.remove(QStringLiteral("inicio"));
        manana.remove(QStringLiteral("fin"));
        arrTurnos[0] = manana;
        turnos[QStringLiteral("turnos")] = arrTurnos;
        j[QStringLiteral("turnos")] = turnos;

        exigeRechazoV2(j, QStringLiteral("ventana"));
    }

    /// V2: dos eventos de la misma posición con intervalos que se pisan se rechazan.
    /// La suma de duraciones (12 min) deja la ventana de la posición en 08:10–08:22.
    void v2_eventosSolapados_fallan()
    {
        const QJsonArray eventos{
            evento(QStringLiteral("Receso"), 2, 1, 6, QStringLiteral("08:10"),
                   QStringLiteral("08:16")),
            evento(QStringLiteral("Acto"), 2, 2, 6, QStringLiteral("08:14"),
                   QStringLiteral("08:20"))};
        exigeRechazoV2(conEventos(jsonValido(), eventos), QStringLiteral("solapan"));
    }

    /// V2: en la misma posición, un evento posterior en `orden` no puede empezar antes.
    /// La ventana de la posición 2 es 08:10–08:19 (suma de duraciones 9 min).
    void v2_eventosConCronologiaInvertida_fallan()
    {
        const QJsonArray eventos{
            evento(QStringLiteral("Primero declarado"), 2, 1, 4, QStringLiteral("08:15"),
                   QStringLiteral("08:19")),
            evento(QStringLiteral("Segundo declarado"), 2, 2, 5, QStringLiteral("08:10"),
                   QStringLiteral("08:15"))};
        exigeRechazoV2(conEventos(jsonValido(), eventos),
                       QStringLiteral("orden cronológico"));
    }

    // ─── V2: duración de la hora académica (30–90) y encaje (011) ───────────

    /// La 011 estrecha V2 a 30–90 con el punto único de `duracion_horaria` (RF-1, RF-5).
    void v2_duracionMinutosFueraDeRango_falla()
    {
        for (int duracion : {1, 5, 29, 91}) {
            QJsonObject j = conDuracion(jsonValido(), duracion);
            // Con 91 la ventana de 6 franjas no encajaría; se ensancha para aislar el rango.
            if (duracion == 91) {
                j = conVentanaDelTurno(j, QStringLiteral("07:00"), QStringLiteral("16:10"));
            }
            const auto resultado = validarEntradaSolver(j);
            QVERIFY2(!resultado.ok, qPrintable(QString::number(duracion)));
            QVERIFY2(resultado.mensajeError.contains(QStringLiteral("V2")),
                     qPrintable(resultado.mensajeError));
            QVERIFY2(resultado.mensajeError.contains(QStringLiteral("30")),
                     qPrintable(resultado.mensajeError));
            QVERIFY2(resultado.mensajeError.contains(QStringLiteral("90")),
                     qPrintable(resultado.mensajeError));
            QVERIFY2(resultado.mensajeError.contains(QString::number(duracion)),
                     qPrintable(resultado.mensajeError));
        }
    }

    void v2_duracionMinutosEnRango_pasa()
    {
        // 30, 35 y 45 caben en la ventana de 07:00–12:00 con 6 franjas; 90 exige 16:00.
        for (int duracion : {30, 35, 45}) {
            const auto resultado = validarEntradaSolver(conDuracion(jsonValido(), duracion));
            QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
        }

        const QJsonObject j =
            conVentanaDelTurno(conDuracion(jsonValido(), 90), QStringLiteral("07:00"),
                               QStringLiteral("16:00"));
        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    /// V1: una duración no entera se rechaza citando el valor, antes de que el parseo la
    /// convierta en 0 (RF-5, RNF-1).
    void v1_duracionMinutosNoEntera_falla()
    {
        QJsonObject j = jsonValido();
        QJsonObject franja = j[QStringLiteral("franja_horaria")].toObject();
        franja[QStringLiteral("duracion_minutos")] = 35.5;
        j[QStringLiteral("franja_horaria")] = franja;

        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("V1")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("35.5")),
                 qPrintable(resultado.mensajeError));
    }

    /// V2: un turno sin eventos debe encajar igualmente (6 franjas × 35 > 30 de ventana).
    void v2_turnoQueNoEncaja_falla()
    {
        const QJsonObject j =
            conVentanaDelTurno(jsonValido(), QStringLiteral("07:00"), QStringLiteral("07:30"));

        const auto resultado = validarEntradaSolver(j);
        QVERIFY(!resultado.ok);
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("V2")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("manana")),
                 qPrintable(resultado.mensajeError));
        QVERIFY2(resultado.mensajeError.contains(QStringLiteral("déficit 180")),
                 qPrintable(resultado.mensajeError));
    }

    // ─── V16: toda aula y toda materia declaran tipo ───────────────────────

    void v16_aulaSinTipo_falla()
    {
        QJsonObject j = jsonValido();
        QJsonArray aulas = j[QStringLiteral("aulas")].toArray();
        QJsonObject aula = aulas[0].toObject();
        aula[QStringLiteral("tipo")] = QString();
        aulas[0] = aula;
        j[QStringLiteral("aulas")] = aulas;

        exigeRechazoV16(j, QStringLiteral("A1"));
    }

    void v16_aulaSinCampoTipo_falla()
    {
        QJsonObject j = jsonValido();
        QJsonArray aulas = j[QStringLiteral("aulas")].toArray();
        QJsonObject aula = aulas[0].toObject();
        aula.remove(QStringLiteral("tipo"));
        aulas[0] = aula;
        j[QStringLiteral("aulas")] = aulas;

        exigeRechazoV16(j, QStringLiteral("A1"));
    }

    void v16_materiaSinTipo_falla()
    {
        QJsonObject j = jsonValido();
        QJsonArray materias = j[QStringLiteral("materias")].toArray();
        QJsonObject materia = materias[0].toObject();
        materia[QStringLiteral("tipo")] = QStringLiteral("   ");
        materias[0] = materia;
        j[QStringLiteral("materias")] = materias;

        exigeRechazoV16(j, QStringLiteral("Matemática"));
    }

    // ─── V17: el tipo requerido por una materia tiene aulas ────────────────

    void v17_tipoRequeridoSinAulas_falla()
    {
        QJsonObject materia = QJsonObject{{QStringLiteral("nombre"), QStringLiteral("Matemática")},
                                          {QStringLiteral("tipo"), QStringLiteral("Laboratorio")}};
        exigeRechazoV17(conMaterias(jsonValido(), QJsonArray{materia}),
                        QStringLiteral("Laboratorio"), QStringLiteral("Matemática"),
                        QStringLiteral("1A"));
    }

    /// V17 vale en los dos modos de aula: también con aulas móviles.
    void v17_tipoRequeridoSinAulas_fallaConAulasMoviles()
    {
        QJsonObject j = conMaterias(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("Matemática")},
                                   {QStringLiteral("tipo"), QStringLiteral("Laboratorio")}}});
        j[QStringLiteral("aula_fija")] = false;
        j = conCurso(j, [](QJsonObject& c) { c[QStringLiteral("aula")] = -1; });

        exigeRechazoV17(j, QStringLiteral("Laboratorio"), QStringLiteral("Matemática"),
                        QStringLiteral("1A"));
    }

    /// Los tipos se comparan por clave normalizada: mayúsculas y espacios extremos no cuentan.
    void v17_tipoIgualSalvoMayusculasYEspacios_pasa()
    {
        QJsonObject j = conMaterias(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("Matemática")},
                                   {QStringLiteral("tipo"), QStringLiteral("  aula de ESTUDIO ")}}});
        j = conAulas(
            j, QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("A1")},
                                      {QStringLiteral("tipo"), QStringLiteral("AULA DE ESTUDIO")},
                                      {QStringLiteral("capacidad"), 30}}});

        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    /// V17 no rechaza un tipo declarado que ninguna materia requiera.
    void v17_tipoDeclaradoQueNingunaMateriaRequiere_pasa()
    {
        QJsonObject j = conAulas(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("A1")},
                                   {QStringLiteral("tipo"), QStringLiteral("Aula de estudio")},
                                   {QStringLiteral("capacidad"), 30}},
                       QJsonObject{{QStringLiteral("nombre"), QStringLiteral("Lab")},
                                   {QStringLiteral("tipo"), QStringLiteral("Laboratorio")},
                                   {QStringLiteral("capacidad"), 20}}});

        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    // ─── V18: el aula de referencia es del tipo general ────────────────────

    /// Una referencia sobre un laboratorio es entrada inválida aunque haya aula general para las
    /// materias (RF-3, RF-5).
    void v18_referenciaSobreLaboratorio_falla()
    {
        QJsonObject j = conAulas(
            jsonValido(),
            QJsonArray{
                QJsonObject{{QStringLiteral("nombre"), QStringLiteral("A1")},
                            {QStringLiteral("tipo"), QStringLiteral("Aula de estudio")},
                            {QStringLiteral("capacidad"), 30}},
                QJsonObject{{QStringLiteral("nombre"), QStringLiteral("L1")},
                            {QStringLiteral("tipo"), QStringLiteral("Laboratorio")},
                            {QStringLiteral("capacidad"), 20}}});
        j = conCurso(j, [](QJsonObject& c) { c[QStringLiteral("aula")] = 1; });

        exigeRechazoV18(j, QStringLiteral("L1"), QStringLiteral("1A"),
                        QStringLiteral("Laboratorio"));
    }

    /// V17 queda satisfecho (el laboratorio tiene aula) y quien rechaza es V18: solo la referencia
    /// es especializada.
    void v18_referenciaSobreTallerConMateriasEspecializadas_falla()
    {
        QJsonObject j = conMaterias(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("Química")},
                                   {QStringLiteral("tipo"), QStringLiteral("Laboratorio")}}});
        j = conAulas(
            j, QJsonArray{
                   QJsonObject{{QStringLiteral("nombre"), QStringLiteral("T1")},
                               {QStringLiteral("tipo"), QStringLiteral("Taller")},
                               {QStringLiteral("capacidad"), 20}},
                   QJsonObject{{QStringLiteral("nombre"), QStringLiteral("L1")},
                               {QStringLiteral("tipo"), QStringLiteral("Laboratorio")},
                               {QStringLiteral("capacidad"), 20}}});
        j = conCurso(j, [](QJsonObject& c) { c[QStringLiteral("aula")] = 0; });

        exigeRechazoV18(j, QStringLiteral("T1"), QStringLiteral("1A"), QStringLiteral("Taller"));
    }

    /// La convención del tipo general se compara por clave: mayúsculas y espacios no cuentan.
    void v18_referenciaGeneralPorClave_pasa()
    {
        const QJsonObject j = conAulas(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("A1")},
                                   {QStringLiteral("tipo"), QStringLiteral("  AULA DE ESTUDIO ")},
                                   {QStringLiteral("capacidad"), 30}}});

        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    /// Con aulas móviles la referencia se ignora (V8): V18 no aplica.
    void v18_aulasMoviles_noAplica()
    {
        QJsonObject j = conMaterias(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("Química")},
                                   {QStringLiteral("tipo"), QStringLiteral("Laboratorio")}}});
        j = conAulas(
            j, QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("L1")},
                                      {QStringLiteral("tipo"), QStringLiteral("Laboratorio")},
                                      {QStringLiteral("capacidad"), 20}}});
        j[QStringLiteral("aula_fija")] = false;
        j = conCurso(j, [](QJsonObject& c) { c[QStringLiteral("aula")] = 0; });

        const auto resultado = validarEntradaSolver(j);
        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
    }

    /// Centro sin aulas del tipo general y materias que lo requieren: rechaza V17, no V18.
    void v18_sinAulaGeneralEnModoFijo_llegaPorV17()
    {
        const QJsonObject j = conAulas(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("L1")},
                                   {QStringLiteral("tipo"), QStringLiteral("Laboratorio")},
                                   {QStringLiteral("capacidad"), 20}}});

        exigeRechazoV17(j, QStringLiteral("Aula de estudio"), QStringLiteral("Matemática"),
                        QStringLiteral("1A"));
    }

    /// Lo mismo con aulas móviles: V17 vale en los dos modos y V18 no aplica.
    void v18_sinAulaGeneralEnModoMovil_llegaPorV17()
    {
        QJsonObject j = conAulas(
            jsonValido(),
            QJsonArray{QJsonObject{{QStringLiteral("nombre"), QStringLiteral("L1")},
                                   {QStringLiteral("tipo"), QStringLiteral("Laboratorio")},
                                   {QStringLiteral("capacidad"), 20}}});
        j[QStringLiteral("aula_fija")] = false;
        j = conCurso(j, [](QJsonObject& c) { c[QStringLiteral("aula")] = -1; });

        exigeRechazoV17(j, QStringLiteral("Aula de estudio"), QStringLiteral("Matemática"),
                        QStringLiteral("1A"));
    }

    void jsonVacio_falla()
    {
        const auto resultado = validarEntradaSolver(QJsonObject{});
        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }};

QTEST_MAIN(TestValidadorEntrada)
#include "test_validador_entrada.moc"
