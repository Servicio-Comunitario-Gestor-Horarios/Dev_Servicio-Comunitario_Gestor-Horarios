#include <QJsonArray>
#include <QJsonObject>
#include <QTest>
#include <QString>

#include <backend/solver/config/contrato_solver.hpp>
#include <backend/solver/config/solver_config.hpp>

/// Tests de las funciones puras, las constantes y la forma del contrato 2.0 (RF-1, RF-3, RF-5).
class TestContratoSolver : public QObject
{
    Q_OBJECT

private:
    /// Configuración 2.0 mínima y válida: tipos declarados en aulas y materias, ventana de turno
    /// y dos eventos en la misma posición (sin identidad).
    QJsonObject config2_0() const
    {
        QJsonObject o;
        o["version"]   = QStringLiteral("2.0");
        o["aula_fija"] = true;

        QJsonObject dim;
        dim["num_profesores"] = 1;
        dim["num_materias"]   = 1;
        dim["num_aulas"]      = 1;
        dim["num_cursos"]     = 1;
        dim["num_dias"]       = 5;
        dim["num_slots_dia"]  = 6;
        o["dimensiones"] = dim;

        QJsonObject franja;
        franja["duracion_minutos"] = 45;
        franja["slots_por_turno"]  = 6;
        o["franja_horaria"] = franja;

        QJsonObject turno;
        turno["nombre"] = QStringLiteral("manana");
        turno["slot"]   = QJsonArray{0, 1, 2, 3, 4, 5};
        turno["inicio"] = QStringLiteral("07:00");
        turno["fin"]    = QStringLiteral("12:00");
        QJsonObject turnos;
        turnos["turnos"] = QJsonArray{turno};
        o["turnos"] = turnos;

        QJsonObject receso;
        receso["nombre"]           = QStringLiteral("Receso");
        receso["turno"]            = QStringLiteral("manana");
        receso["despues_de_slot"]  = 2;
        receso["orden"]            = 1;
        receso["duracion"]         = 10;
        // Ventana de la posición 2 con la franja de 45 min: 07:00 + 2 × 45 = 08:30, y la
        // suma de duraciones (15 min) la cierra a las 08:45; el receso va de 08:34 a 08:44.
        receso["inicio"]           = QStringLiteral("08:34");
        receso["fin"]              = QStringLiteral("08:44");
        QJsonObject acto;
        acto["nombre"]          = QStringLiteral("Acto cívico");
        acto["turno"]           = QStringLiteral("manana");
        acto["despues_de_slot"] = 2;
        acto["orden"]           = 2;
        acto["duracion"]        = 5;
        o["eventos"] = QJsonArray{receso, acto};

        QJsonObject materiaPlan;
        materiaPlan["materia_idx"]     = 0;
        materiaPlan["curso"]           = 1;
        materiaPlan["horas_semanales"] = 2;
        QJsonObject plan;
        plan["codigo"]   = QStringLiteral("Ciencias");
        plan["materias"] = QJsonArray{materiaPlan};
        o["planes"] = QJsonArray{plan};

        QJsonObject curso;
        curso["anio"]            = 1;
        curso["seccion"]         = QStringLiteral("A");
        curso["turno"]           = QStringLiteral("manana");
        curso["plan"]            = QStringLiteral("Ciencias");
        curso["num_estudiantes"] = 20;
        curso["aula"]            = 0;
        o["cursos"] = QJsonArray{curso};

        QJsonObject profesor;
        profesor["nombre"]             = QStringLiteral("Ana");
        profesor["horas_aula"]         = 2;
        profesor["anios"]              = QJsonArray{1};
        profesor["materias_asignadas"] = QJsonArray{0};
        profesor["materias_suplente"]  = QJsonArray{};
        QJsonObject disp;
        disp["dia"]  = 0;
        disp["slot"] = QJsonArray{0, 1, 2, 3, 4, 5};
        profesor["disponibilidad"] = QJsonArray{disp};
        o["profesores"] = QJsonArray{profesor};

        QJsonObject materia;
        materia["nombre"] = QStringLiteral("Matemática");
        materia["tipo"]   = QStringLiteral("Aula de estudio");
        o["materias"] = QJsonArray{materia};

        QJsonObject aula;
        aula["nombre"]    = QStringLiteral("A1");
        aula["tipo"]      = QStringLiteral("Aula de estudio");
        aula["capacidad"] = 30;
        o["aulas"] = QJsonArray{aula};

        return o;
    }

private slots:

    void bloques_esIdentidad()
    {
        // Rectificación (2026-10-17): 1 hora académica = 1 bloque. `duracion_minutos` no
        // interviene en la conversión; no hay multiplicación por 60 ni redondeo.
        QCOMPARE(bloques(3, 45), 3);   // antes 4 (ceil(180/45))
        QCOMPARE(bloques(2, 45), 2);   // antes 3 (ceil(120/45))
        QCOMPARE(bloques(4, 40), 4);
        QCOMPARE(bloques(6, 60), 6);
    }

    void bloques_duracionInvalida_devuelveCero()
    {
        QCOMPARE(bloques(2, 0), 0);
        QCOMPARE(bloques(2, -30), 0);
    }

    void bloquesAula_esIdentidad()
    {
        // El tope del docente se mide en bloques = horas_aula (S1), sin ×60 ni redondeo.
        QCOMPARE(bloquesAula(8, 45), 8);
        QCOMPARE(bloquesAula(8, 0), 0);
        QCOMPARE(bloquesAula(8, 45), bloques(8, 45));
    }

    void claveCurso_canonicaYDeterminista()
    {
        QCOMPARE(claveCurso(1, QStringLiteral("A")), claveCurso(1, QStringLiteral("A")));
        QVERIFY(claveCurso(1, QStringLiteral("A")) != claveCurso(2, QStringLiteral("A")));
        QVERIFY(claveCurso(1, QStringLiteral("A")) != claveCurso(1, QStringLiteral("B")));
        QVERIFY(!claveCurso(1, QStringLiteral("A")).isEmpty());
    }

    void constantes_jerarquiaDelObjetivo()
    {
        QVERIFY(ContratoSolver::PESO_HORAS > ContratoSolver::COSTE_SUPLENTE);
        QVERIFY(ContratoSolver::COSTE_SUPLENTE > 0);
    }

    /// RF-1: el contrato se declara en la versión 2.0, también al serializar una config por defecto.
    void contratoDeclaraLaVersion2_0()
    {
        const SolverConfig config;
        QCOMPARE(config.version, QStringLiteral("2.0"));
        QCOMPARE(config.toJson()[QStringLiteral("version")].toString(), QStringLiteral("2.0"));
    }

    /// RF-1: la forma 2.0 trae el tipo de cada materia y de cada aula, la ventana del turno y los
    /// eventos (sin identidad).
    void forma2_0_parseaTiposEventosYventanaDeTurno()
    {
        const auto config = SolverConfig::fromJson(config2_0());
        QVERIFY2(config.ok, qPrintable(config.mensajeError));

        QCOMPARE(config.valor.materias.size(), 1);
        QCOMPARE(config.valor.materias[0].tipo, QStringLiteral("Aula de estudio"));
        QCOMPARE(config.valor.aulas.size(), 1);
        QCOMPARE(config.valor.aulas[0].tipo, QStringLiteral("Aula de estudio"));

        QCOMPARE(config.valor.turnos.turnos.size(), 1);
        QCOMPARE(config.valor.turnos.turnos[0].inicio, QStringLiteral("07:00"));
        QCOMPARE(config.valor.turnos.turnos[0].fin, QStringLiteral("12:00"));

        QCOMPARE(config.valor.eventos.size(), 2);
        QCOMPARE(config.valor.eventos[0].nombre, QStringLiteral("Receso"));
        QCOMPARE(config.valor.eventos[0].turno, QStringLiteral("manana"));
        QCOMPARE(config.valor.eventos[0].despues_de_slot, 2);
        QCOMPARE(config.valor.eventos[0].orden, 1);
        QCOMPARE(config.valor.eventos[0].duracion, 10);
        QCOMPARE(config.valor.eventos[0].inicio, QStringLiteral("08:34"));
        QCOMPARE(config.valor.eventos[0].fin, QStringLiteral("08:44"));
        // El horario es opcional: el segundo evento no lo declara.
        QCOMPARE(config.valor.eventos[1].nombre, QStringLiteral("Acto cívico"));
        QCOMPARE(config.valor.eventos[1].orden, 2);
        QVERIFY(config.valor.eventos[1].inicio.isEmpty());
        QVERIFY(config.valor.eventos[1].fin.isEmpty());
    }

    /// RF-1: `toJson` sella 2.0 siempre, serializa los tipos y los eventos y no emite `recesos`;
    /// la identidad del evento no viaja.
    void toJson_sella2_0_yEmiteTiposYeventosSinRecesos()
    {
        const auto config = SolverConfig::fromJson(config2_0());
        QVERIFY2(config.ok, qPrintable(config.mensajeError));

        const QJsonObject json = config.valor.toJson();
        QCOMPARE(json[QStringLiteral("version")].toString(), QStringLiteral("2.0"));
        QVERIFY(json.contains(QStringLiteral("eventos")));
        QVERIFY(!json.contains(QStringLiteral("recesos")));

        const QJsonObject materia = json[QStringLiteral("materias")].toArray()[0].toObject();
        QCOMPARE(materia[QStringLiteral("tipo")].toString(), QStringLiteral("Aula de estudio"));

        const QJsonObject aula = json[QStringLiteral("aulas")].toArray()[0].toObject();
        QCOMPARE(aula[QStringLiteral("tipo")].toString(), QStringLiteral("Aula de estudio"));

        const QJsonObject turno =
            json[QStringLiteral("turnos")].toObject()[QStringLiteral("turnos")].toArray()[0]
                .toObject();
        QCOMPARE(turno[QStringLiteral("inicio")].toString(), QStringLiteral("07:00"));
        QCOMPARE(turno[QStringLiteral("fin")].toString(), QStringLiteral("12:00"));

        const QJsonArray eventos = json[QStringLiteral("eventos")].toArray();
        QCOMPARE(eventos.size(), 2);
        const QJsonObject evento = eventos[0].toObject();
        QVERIFY(!evento.contains(QStringLiteral("id")));
        QCOMPARE(evento[QStringLiteral("nombre")].toString(), QStringLiteral("Receso"));
        QCOMPARE(evento[QStringLiteral("turno")].toString(), QStringLiteral("manana"));
        QCOMPARE(evento[QStringLiteral("despues_de_slot")].toInt(), 2);
        QCOMPARE(evento[QStringLiteral("orden")].toInt(), 1);
        QCOMPARE(evento[QStringLiteral("duracion")].toInt(), 10);
        QCOMPARE(evento[QStringLiteral("inicio")].toString(), QStringLiteral("08:34"));
        QCOMPARE(evento[QStringLiteral("fin")].toString(), QStringLiteral("08:44"));
    }

    /// RF-1: guardar y volver a cargar reproduce tipos, eventos y ventana de turno sin pérdida.
    void idaYVuelta2_0_sinPerdida()
    {
        const auto primera = SolverConfig::fromJson(config2_0());
        QVERIFY2(primera.ok, qPrintable(primera.mensajeError));

        const QJsonObject serializado = primera.valor.toJson();
        const auto segunda = SolverConfig::fromJson(serializado);
        QVERIFY2(segunda.ok, qPrintable(segunda.mensajeError));

        QCOMPARE(segunda.valor.toJson(), serializado);
        QCOMPARE(segunda.valor.materias[0].tipo, QStringLiteral("Aula de estudio"));
        QCOMPARE(segunda.valor.aulas[0].tipo, QStringLiteral("Aula de estudio"));
        QCOMPARE(segunda.valor.eventos.size(), 2);
        QCOMPARE(segunda.valor.eventos[1].nombre, QStringLiteral("Acto cívico"));
        QCOMPARE(segunda.valor.turnos.turnos[0].inicio, QStringLiteral("07:00"));
    }

    /// RF-5: la clave de tipo normaliza recorte y plegado Unicode; solo sirve para comparar.
    void claveTipo_normalizaRecorteYPlegado()
    {
        QCOMPARE(claveTipo(QStringLiteral("  Laboratorio  ")), QStringLiteral("laboratorio"));
        QCOMPARE(claveTipo(QStringLiteral("laboratorio")), QStringLiteral("laboratorio"));
        QCOMPARE(claveTipo(QStringLiteral("Física")), claveTipo(QStringLiteral("FÍSICA")));
        QCOMPARE(claveTipo(QStringLiteral("ÁULA de Estudio ")),
                 claveTipo(QStringLiteral("áula de estudio")));
        QCOMPARE(claveTipo(QStringLiteral("   ")), QString());
        QCOMPARE(claveTipo(QString()), QString());
    }

    /// RF-1: la normalización de tipos es solo para comparar; el nombre visible se conserva tal cual.
    void tipoSeConservaTalCualSeDeclaro()
    {
        QJsonObject o = config2_0();
        QJsonObject materia = o[QStringLiteral("materias")].toArray()[0].toObject();
        materia[QStringLiteral("tipo")] = QStringLiteral("  Laboratorio  ");
        o[QStringLiteral("materias")] = QJsonArray{materia};

        // El aula declara el mismo tipo con otra caja y otros espacios: V17 compara por clave.
        QJsonObject aula = o[QStringLiteral("aulas")].toArray()[0].toObject();
        aula[QStringLiteral("tipo")] = QStringLiteral(" LABORATORIO ");

        // V18: la referencia del curso es del tipo general, así que el laboratorio no puede serlo.
        QJsonObject general;
        general[QStringLiteral("nombre")]    = QStringLiteral("A2");
        general[QStringLiteral("tipo")]      = QStringLiteral("Aula de estudio");
        general[QStringLiteral("capacidad")] = 30;
        o[QStringLiteral("aulas")] = QJsonArray{aula, general};

        QJsonObject dim = o[QStringLiteral("dimensiones")].toObject();
        dim[QStringLiteral("num_aulas")] = 2;
        o[QStringLiteral("dimensiones")] = dim;

        QJsonObject curso = o[QStringLiteral("cursos")].toArray()[0].toObject();
        curso[QStringLiteral("aula")] = 1;
        o[QStringLiteral("cursos")] = QJsonArray{curso};

        const auto config = SolverConfig::fromJson(o);
        QVERIFY2(config.ok, qPrintable(config.mensajeError));
        QCOMPARE(config.valor.materias[0].tipo, QStringLiteral("  Laboratorio  "));
        QCOMPARE(config.valor.aulas[0].tipo, QStringLiteral(" LABORATORIO "));
        QCOMPARE(config.valor.toJson()[QStringLiteral("materias")].toArray()[0].toObject()
                     [QStringLiteral("tipo")].toString(),
                 QStringLiteral("  Laboratorio  "));
    }
};

QTEST_MAIN(TestContratoSolver)
#include "test_contrato_solver.moc"
