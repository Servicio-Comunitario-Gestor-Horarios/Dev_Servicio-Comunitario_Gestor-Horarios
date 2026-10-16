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
        d.materias = {mat};

        AulaDTO aula; aula.id = 10; aula.nombre = QStringLiteral("A1"); aula.capacidad = 30;
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
        p.version = QStringLiteral("1.0");
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
        return construirJsonEntrada(dominio(), parametros(),
                                    QDateTime(QDate(2026, 10, 10), QTime(12, 0, 0)));
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
        // num_estudiantes es opcional; solo un valor negativo se rechaza.
        QJsonObject j = conCurso(jsonValido(), [](QJsonObject& c) { c["num_estudiantes"] = -1; });
        QVERIFY(!validarEntradaSolver(j).ok);
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

    void jsonVacio_falla()
    {
        const auto resultado = validarEntradaSolver(QJsonObject{});
        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }
};

QTEST_MAIN(TestValidadorEntrada)
#include "test_validador_entrada.moc"
