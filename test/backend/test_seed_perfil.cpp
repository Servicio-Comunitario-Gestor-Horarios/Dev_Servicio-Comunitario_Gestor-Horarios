#include <QSet>
#include <QTest>

#include <backend/services/duracion_horaria.hpp>
#include <seed/docentes.hpp>
#include <seed/perfil.hpp>

using namespace seed;

namespace {

/// Devuelve el plan con `codigo`, o un plan vacío si no existe.
const PlanDef& planPorCodigo(const Perfil& perfil, const QString& codigo)
{
    for (const PlanDef& plan : perfil.planes) {
        if (plan.codigo == codigo) {
            return plan;
        }
    }
    static const PlanDef vacio;
    return vacio;
}

/// Número de materias-año de un plan en un año concreto.
int materiasDeAnio(const PlanDef& plan, int anio)
{
    int n = 0;
    for (const CargaPlan& carga : plan.cargas) {
        if (carga.anio == anio) {
            ++n;
        }
    }
    return n;
}

/// Horas académicas de un área del plan en un año; -1 si no se cursa.
int horasDeArea(const PlanDef& plan, const Perfil& perfil, const QString& area, int anio)
{
    for (const CargaPlan& carga : plan.cargas) {
        if (carga.anio != anio) {
            continue;
        }
        for (const MateriaAnio& materia : perfil.materias) {
            if (materia.nombre == carga.materia && materia.area == area) {
                return carga.horas;
            }
        }
    }
    return -1;
}

/// Σ materias asignadas de una plantilla de docentes.
int asignacionesDe(const QVector<DocenteDef>& docentes)
{
    int n = 0;
    for (const DocenteDef& docente : docentes) {
        n += docente.materias.size();
    }
    return n;
}

/// Σ filas de disponibilidad de una plantilla de docentes.
int disponibilidadesDe(const QVector<DocenteDef>& docentes)
{
    int n = 0;
    for (const DocenteDef& docente : docentes) {
        n += docente.disponibilidad.size();
    }
    return n;
}

/// Conjunto de tipos de aula que usan las aulas del perfil.
QSet<QString> tiposDeAulasDe(const Perfil& perfil)
{
    QSet<QString> tipos;
    for (const AulaDef& aula : perfil.aulas) {
        tipos.insert(aula.tipo);
    }
    return tipos;
}

/// Σ eventos de todos los turnos del perfil.
int eventosDe(const Perfil& perfil)
{
    int n = 0;
    for (const TurnoDef& turno : perfil.turnos) {
        n += turno.eventos.size();
    }
    return n;
}

/// Número de cursos de un turno.
int cursosEnTurno(const Perfil& perfil, const QString& turno)
{
    int n = 0;
    for (const CursoDef& curso : perfil.cursos) {
        if (curso.turno == turno) {
            ++n;
        }
    }
    return n;
}

/// Σ matrícula de los cursos de un plan.
int matriculaDePlan(const Perfil& perfil, const QString& plan)
{
    int n = 0;
    for (const CursoDef& curso : perfil.cursos) {
        if (curso.plan == plan) {
            n += curso.matricula;
        }
    }
    return n;
}

/// Materias-año de un área de formación.
QSet<QString> materiasDelArea(const Perfil& perfil, const QString& area)
{
    QSet<QString> materias;
    for (const MateriaAnio& materia : perfil.materias) {
        if (materia.area == area) {
            materias.insert(materia.nombre);
        }
    }
    return materias;
}

/// Σ asignaciones de una plantilla a las materias de un área.
int asignacionesDelArea(const Perfil& perfil, const QVector<DocenteDef>& docentes,
                        const QString& area)
{
    const QSet<QString> materias = materiasDelArea(perfil, area);
    int n = 0;
    for (const DocenteDef& docente : docentes) {
        for (const QString& materia : docente.materias) {
            if (materias.contains(materia)) {
                ++n;
            }
        }
    }
    return n;
}

/// Σ asignaciones por turno del docente.
int asignacionesEnTurno(const QVector<DocenteDef>& docentes, const QString& turno)
{
    int n = 0;
    for (const DocenteDef& docente : docentes) {
        if (docente.turno == turno) {
            n += docente.materias.size();
        }
    }
    return n;
}

/// ¿Tiene el perfil un aula con ese nombre?
bool existeAula(const Perfil& perfil, const QString& nombre)
{
    for (const AulaDef& aula : perfil.aulas) {
        if (aula.nombre == nombre) {
            return true;
        }
    }
    return false;
}

/// Aula del perfil con ese nombre, o nullptr.
const AulaDef* aulaPorNombre(const Perfil& perfil, const QString& nombre)
{
    for (const AulaDef& aula : perfil.aulas) {
        if (aula.nombre == nombre) {
            return &aula;
        }
    }
    return nullptr;
}

} // namespace

class TestSeedPerfil : public QObject {
    Q_OBJECT

private slots:
    void bloquesEsLaIdentidad()
    {
        QCOMPARE(bloques(0), 0);
        QCOMPARE(bloques(1), 1);
        QCOMPARE(bloques(4), 4);
        QCOMPARE(bloques(34), 34);
        QCOMPARE(bloques(40), 40);
    }

    // ── Planes (RF-2) ─────────────────────────────────────────────────────
    void planesDelCentro()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(perfil.planes.size(), 2);

        const PlanDef& media = planPorCodigo(perfil, QStringLiteral("31059"));
        QCOMPARE(media.nombre, QStringLiteral("Media General"));
        QCOMPARE(media.cargas.size(), 45);
        const int horas31059[5] = {34, 34, 34, 36, 34};
        const int areas31059[5] = {8, 8, 9, 10, 10};
        for (int anio = 1; anio <= 5; ++anio) {
            QCOMPARE(horasDeAnio(media, anio), horas31059[anio - 1]);
            QCOMPARE(bloquesDeAnio(media, anio), horas31059[anio - 1]);
            QCOMPARE(materiasDeAnio(media, anio), areas31059[anio - 1]);
            QVERIFY(horasDeAnio(media, anio) % 2 == 0);
        }

        const PlanDef& ciencias = planPorCodigo(perfil, QStringLiteral("31060"));
        QCOMPARE(ciencias.nombre, QStringLiteral("Ciencias y Tecnología"));
        QCOMPARE(ciencias.cargas.size(), 50);
        for (int anio = 1; anio <= 5; ++anio) {
            QCOMPARE(horasDeAnio(ciencias, anio), 40);
            QCOMPARE(bloquesDeAnio(ciencias, anio), 40);
            QCOMPARE(materiasDeAnio(ciencias, anio), 10);
        }

        // Tablas por área de cada plan (spot checks de las dos tablas de la spec).
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Castellano"), 1), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Castellano"), 5), 3);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Inglés y otras lenguas extranjeras"), 5), 3);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Matemáticas"), 5), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Educación Física"), 1), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Arte y Patrimonio"), 1), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Arte y Patrimonio"), 3), -1);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Ciencias Naturales"), 2), 6);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Ciencias Naturales"), 3), -1);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Física"), 2), -1);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Física"), 3), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Química"), 3), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Biología"), 3), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Geografía, Historia y Ciudadanía"), 1), 6);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Geografía, Historia y Ciudadanía"), 5), 4);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Formación para la Soberanía Nacional"), 3), -1);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Formación para la Soberanía Nacional"), 4), 2);
        QCOMPARE(horasDeArea(media, perfil, QStringLiteral("Orientación y Convivencia"), 1), 2);

        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Lengua y Literatura"), 5), 4);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Idiomas"), 1), 4);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Biología, Ambiente y Tecnología"), 1), 4);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Física"), 1), 2);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Física"), 3), 4);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Química"), 2), 2);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Química"), 5), 4);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Geografía, Historia y Soberanía Nacional"), 1), 6);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Orientación Vocacional"), 1), 4);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Orientación Vocacional"), 3), 2);
        QCOMPARE(horasDeArea(ciencias, perfil, QStringLiteral("Innovación Tecnológica y Productividad"), 5), 6);
    }

    void horasDeAnioSinAnioDevuelveMenosUno()
    {
        PlanDef plan;
        plan.cargas = {CargaPlan{QStringLiteral("Materia 1º"), 1, 4}};

        QCOMPARE(horasDeAnio(plan, 1), 4);
        QCOMPARE(horasDeAnio(plan, 2), -1);
        QCOMPARE(bloquesDeAnio(plan, 2), -1);
    }

    // ── Catálogo y cargas (RF-3) ──────────────────────────────────────────
    void catalogoDelCentro()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(perfil.materias.size(), 85);

        QSet<QString> nombres;
        int sinCarga = 0;
        int conCarga = 0;
        int compartidas = 0;
        int compartidasQuinto = 0;
        for (const MateriaAnio& materia : perfil.materias) {
            QVERIFY2(!nombres.contains(materia.nombre), qPrintable(materia.nombre));
            nombres.insert(materia.nombre);
            QVERIFY2(!materia.area.isEmpty(), qPrintable(materia.nombre));
            QVERIFY(materia.anio >= 1 && materia.anio <= 5);
            if (materia.sinCarga) {
                ++sinCarga;
                continue;
            }
            ++conCarga;

            int enlaces = 0;
            for (const PlanDef& plan : perfil.planes) {
                for (const CargaPlan& carga : plan.cargas) {
                    if (carga.materia == materia.nombre) {
                        ++enlaces;
                    }
                }
            }
            QVERIFY2(enlaces >= 1, qPrintable(materia.nombre));
            if (enlaces == 2) {
                ++compartidas;
                if (materia.anio == 5) {
                    ++compartidasQuinto;
                }
            }
        }
        QCOMPARE(conCarga, 79);
        QCOMPARE(sinCarga, 6);
        QCOMPARE(compartidas, 16);

        // Las 4 compartidas de 5.º: Matemáticas, Educación Física, Física y Química.
        QCOMPARE(compartidasQuinto, 4);
        for (const char* area : {"Matemáticas", "Educación Física", "Física", "Química"}) {
            const QString nombre = QStringLiteral("%1 5º").arg(QString::fromUtf8(area));
            int enlaces = 0;
            for (const PlanDef& plan : perfil.planes) {
                for (const CargaPlan& carga : plan.cargas) {
                    if (carga.materia == nombre) {
                        ++enlaces;
                    }
                }
            }
            QCOMPARE(enlaces, 2);
        }

        // Las 6 materias sin carga son las de D-4 y van al final del catálogo.
        const QStringList sinCargaEsperadas = {
            QStringLiteral("Participación en los Grupos de Creación, Recreación y Producción 1º"),
            QStringLiteral("Participación en los Grupos de Creación, Recreación y Producción 2º"),
            QStringLiteral("Participación en los Grupos de Creación, Recreación y Producción 3º"),
            QStringLiteral("Participación en los Grupos de Creación, Recreación y Producción 4º"),
            QStringLiteral("Participación en los Grupos de Creación, Recreación y Producción 5º"),
            QStringLiteral("Ciencias de la Tierra 5º"),
        };
        for (int i = 0; i < sinCargaEsperadas.size(); ++i) {
            const MateriaAnio& materia = perfil.materias[perfil.materias.size() - 6 + i];
            QCOMPARE(materia.nombre, sinCargaEsperadas[i]);
            QVERIFY(materia.sinCarga);
        }

        // Orden por año: primero las áreas del 31059 y después las del 31060, sin duplicar
        // las compartidas.
        QCOMPARE(perfil.materias[0].nombre, QStringLiteral("Castellano 1º"));
        QCOMPARE(perfil.materias[7].nombre, QStringLiteral("Orientación y Convivencia 1º"));
        QCOMPARE(perfil.materias[8].nombre, QStringLiteral("Lengua y Literatura 1º"));
        QCOMPARE(perfil.materias[8].area, QStringLiteral("Lengua y Literatura"));
        // 1.º de 31060: 10 áreas, 2 compartidas con 31059 → 8 materias nuevas.
        QCOMPARE(materiasDeAnio(perfil.planes[0], 1), 8);
        QCOMPARE(materiasDeAnio(perfil.planes[1], 1), 10);
    }

    void cargasPorPlanYTipos()
    {
        const Perfil& perfil = perfilPorDefecto();
        int total = 0;
        for (const PlanDef& plan : perfil.planes) {
            total += plan.cargas.size();
        }
        QCOMPARE(total, 95);
        QCOMPARE(perfil.planes[0].cargas.size(), 45);
        QCOMPARE(perfil.planes[1].cargas.size(), 50);

        // Tipos de aula (RF-3, RF-5): 1 declarado (cancha) y 2 en las aulas (general + cancha).
        QCOMPARE(perfil.tiposAula.size(), 1);
        QCOMPARE(perfil.tiposAula.at(0), QStringLiteral("Cancha"));
        QCOMPARE(tiposDeAulasDe(perfil).size(), 2);
        QVERIFY(tiposDeAulasDe(perfil).contains(QStringLiteral("Aula de estudio")));
        QVERIFY(tiposDeAulasDe(perfil).contains(QStringLiteral("Cancha")));
    }

    void tiposDeMateria()
    {
        // Educación Física se imparte en la cancha; el resto, en el aula de estudio (RF-3).
        QCOMPARE(tipoEspecializadoDeMateria(QStringLiteral("Educación Física 1º")),
                 QStringLiteral("Cancha"));
        QCOMPARE(tipoEspecializadoDeMateria(QStringLiteral("Castellano 1º")),
                 QStringLiteral("Aula de estudio"));
        QCOMPARE(tipoEspecializadoDeMateria(QStringLiteral("Física 3º")),
                 QStringLiteral("Aula de estudio"));
        QCOMPARE(tipoEspecializadoDeMateria(QStringLiteral("Biología, Ambiente y Tecnología 1º")),
                 QStringLiteral("Aula de estudio"));

        const Perfil& perfil = perfilPorDefecto();
        const QSet<QString> tipos = tiposDeAulasDe(perfil);
        QCOMPARE(tipoFinalDeMateria(QStringLiteral("Educación Física 1º"), tipos),
                 QStringLiteral("Cancha"));
        QCOMPARE(tipoFinalDeMateria(QStringLiteral("Física 3º"), tipos),
                 QStringLiteral("Aula de estudio"));

        // Sin cancha sembrada, Educación Física cae al tipo general.
        QSet<QString> sinCancha = tipos;
        sinCancha.remove(QStringLiteral("Cancha"));
        QCOMPARE(tipoFinalDeMateria(QStringLiteral("Educación Física 1º"), sinCancha),
                 QStringLiteral("Aula de estudio"));

        // Solo las 5 materias de Educación Física exigen cancha.
        int conCancha = 0;
        for (const MateriaAnio& materia : perfil.materias) {
            if (tipoFinalDeMateria(materia.nombre, tipos) == QStringLiteral("Cancha")) {
                ++conCancha;
            }
        }
        QCOMPARE(conCancha, 5);
    }

    // ── Cursos, aulas y turnos (RF-4, RF-5, RF-7) ─────────────────────────
    void cursosDelCentro()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(perfil.cursos.size(), 24);
        QCOMPARE(cursosEnTurno(perfil, QStringLiteral("Mañana")), 12);
        QCOMPARE(cursosEnTurno(perfil, QStringLiteral("Tarde")), 12);

        QSet<QString> parejas;
        QSet<QString> aulasEnManana;
        QSet<QString> aulasEnTarde;
        int totalMatricula = 0;
        for (const CursoDef& curso : perfil.cursos) {
            const QString nombre = QStringLiteral("%1º%2").arg(curso.anio).arg(curso.seccion);
            QVERIFY(curso.anio >= 1 && curso.anio <= 5);
            QCOMPARE(curso.seccion.size(), 1);
            QVERIFY(curso.seccion.at(0).isUpper());
            QVERIFY2(!parejas.contains(nombre), qPrintable(nombre));
            parejas.insert(nombre);

            QVERIFY2(curso.plan == QStringLiteral("31059") ||
                         curso.plan == QStringLiteral("31060"),
                     qPrintable(nombre));
            QVERIFY2(curso.turno == QStringLiteral("Mañana") ||
                         curso.turno == QStringLiteral("Tarde"),
                     qPrintable(nombre));
            QVERIFY2(horasDeAnio(planPorCodigo(perfil, curso.plan), curso.anio) > 0,
                     qPrintable(nombre));

            const AulaDef* aula = aulaPorNombre(perfil, curso.aula);
            QVERIFY2(aula != nullptr, qPrintable(curso.aula));
            QCOMPARE(aula->tipo, QStringLiteral("Aula de estudio"));

            QSet<QString>& aulas =
                curso.turno == QStringLiteral("Mañana") ? aulasEnManana : aulasEnTarde;
            QVERIFY2(!aulas.contains(curso.aula), qPrintable(curso.aula));
            aulas.insert(curso.aula);

            QVERIFY2(curso.matricula >= 32 && curso.matricula <= 44, qPrintable(nombre));
            totalMatricula += curso.matricula;
        }
        QCOMPARE(parejas.size(), 24);
        QCOMPARE(aulasEnManana.size(), 12);
        QCOMPARE(aulasEnTarde.size(), 12);

        // Matrícula: 889 = 579 (31059) + 310 (31060); 44 en 4.º A y 4.º B.
        QCOMPARE(totalMatricula, 889);
        QCOMPARE(matriculaDePlan(perfil, QStringLiteral("31059")), 579);
        QCOMPARE(matriculaDePlan(perfil, QStringLiteral("31060")), 310);

        const struct {
            int anio;
            const char* seccion;
            const char* plan;
            const char* turno;
            const char* aula;
            int matricula;
        } kEsperados[] = {
            {1, "A", "31059", "Mañana", "Aula 01", 39},
            {1, "B", "31059", "Mañana", "Aula 02", 39},
            {3, "A", "31059", "Mañana", "Aula 03", 39},
            {4, "A", "31059", "Mañana", "Aula 12", 44},
            {4, "B", "31059", "Mañana", "Aula 11", 44},
            {4, "C", "31059", "Mañana", "Aula 10", 41},
            {5, "D", "31059", "Mañana", "Aula 06", 39},
            {2, "A", "31059", "Tarde", "Aula 01", 37},
            {2, "C", "31059", "Tarde", "Aula 03", 34},
            {1, "C", "31060", "Tarde", "Aula 10", 37},
            {1, "E", "31060", "Tarde", "Aula 12", 36},
            {2, "D", "31060", "Tarde", "Aula 04", 32},
            {3, "D", "31060", "Tarde", "Aula 07", 37},
            {4, "E", "31060", "Tarde", "Aula 08", 32},
        };
        for (const auto& esperado : kEsperados) {
            bool encontrado = false;
            for (const CursoDef& curso : perfil.cursos) {
                if (curso.anio == esperado.anio &&
                    curso.seccion == QString::fromUtf8(esperado.seccion)) {
                    encontrado = true;
                    QCOMPARE(curso.plan, QString::fromUtf8(esperado.plan));
                    QCOMPARE(curso.turno, QString::fromUtf8(esperado.turno));
                    QCOMPARE(curso.aula, QString::fromUtf8(esperado.aula));
                    QCOMPARE(curso.matricula, esperado.matricula);
                }
            }
            QVERIFY2(encontrado, esperado.seccion);
        }
    }

    void aulasYCancha()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(perfil.aulas.size(), 13);

        QSet<QString> nombres;
        int generales = 0;
        int canchas = 0;
        for (const AulaDef& aula : perfil.aulas) {
            QVERIFY2(!nombres.contains(aula.nombre), qPrintable(aula.nombre));
            nombres.insert(aula.nombre);
            QVERIFY(aula.capacidad > 0);
            QVERIFY(!aula.edificio.isEmpty());
            QVERIFY(!aula.piso.isEmpty());
            if (aula.tipo == QStringLiteral("Aula de estudio")) {
                ++generales;
                QCOMPARE(aula.capacidad, 44);
            } else {
                ++canchas;
                QCOMPARE(aula.tipo, QStringLiteral("Cancha"));
                QCOMPARE(aula.capacidad, 88);
                QCOMPARE(aula.piso, QStringLiteral("Descubierto"));
            }
        }
        QCOMPARE(generales, 12);
        QCOMPARE(canchas, 1);

        // La cancha no es aula de referencia de ningún curso (RF-5).
        for (const CursoDef& curso : perfil.cursos) {
            QVERIFY2(curso.aula != QStringLiteral("Cancha"), qPrintable(curso.aula));
        }
        QVERIFY(existeAula(perfil, QStringLiteral("Aula 01")));
        QVERIFY(existeAula(perfil, QStringLiteral("Aula 12")));
    }

    void turnosYEventos()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(perfil.turnos.size(), 2);
        QCOMPARE(eventosDe(perfil), 3);

        const TurnoDef& manana = perfil.turnos[0];
        QCOMPARE(manana.nombre, QStringLiteral("Mañana"));
        QCOMPARE(manana.inicio, QTime(7, 0));
        QCOMPARE(manana.fin, QTime(12, 40));
        QCOMPARE(manana.numSlots, 8);
        QCOMPARE(manana.eventos.size(), 2);
        QCOMPARE(manana.eventos[0].nombre, QStringLiteral("Receso"));
        QCOMPARE(manana.eventos[0].despuesDeSlot, 2);
        QCOMPARE(manana.eventos[0].orden, 1);
        QCOMPARE(manana.eventos[0].duracion, 10);
        QCOMPARE(manana.eventos[0].inicio, QTime(8, 20));
        QCOMPARE(manana.eventos[0].fin, QTime(8, 30));
        QCOMPARE(manana.eventos[1].nombre, QStringLiteral("Acto cívico"));
        QCOMPARE(manana.eventos[1].despuesDeSlot, 2);
        QCOMPARE(manana.eventos[1].orden, 2);
        QCOMPARE(manana.eventos[1].duracion, 10);
        QCOMPARE(manana.eventos[1].inicio, QTime(8, 30));
        QCOMPARE(manana.eventos[1].fin, QTime(8, 40));

        const TurnoDef& tarde = perfil.turnos[1];
        QCOMPARE(tarde.nombre, QStringLiteral("Tarde"));
        QCOMPARE(tarde.inicio, QTime(12, 40));
        QCOMPARE(tarde.fin, QTime(18, 10));
        QCOMPARE(tarde.numSlots, 8);
        QCOMPARE(tarde.eventos.size(), 1);
        QCOMPARE(tarde.eventos[0].nombre, QStringLiteral("Receso"));
        QCOMPARE(tarde.eventos[0].despuesDeSlot, 2);
        QCOMPARE(tarde.eventos[0].orden, 1);
        QCOMPARE(tarde.eventos[0].duracion, 10);
        QCOMPARE(tarde.eventos[0].inicio, QTime(14, 0));
        QCOMPARE(tarde.eventos[0].fin, QTime(14, 10));

        // Encaje con la duración del perfil (40): mañana 8 × 40 + 20 = 340 y tarde
        // 8 × 40 + 10 = 330 (plan §4.5).
        QCOMPARE(perfil.duracionMinutos, 40);
        QCOMPARE(8 * perfil.duracionMinutos + 20, 340);
        QCOMPARE(8 * perfil.duracionMinutos + 10, 330);
        QCOMPARE(manana.inicio.secsTo(manana.fin) / 60, 340);
        QCOMPARE(tarde.inicio.secsTo(tarde.fin) / 60, 330);
    }

    void duracionYRecuentosEsperados()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(perfil.duracionMinutos, 40);
        QVERIFY(perfil.duracionMinutos >= duracion_horaria::MINIMA);
        QVERIFY(perfil.duracionMinutos <= duracion_horaria::MAXIMA);

        const Recuentos esperados = recuentosEsperados(perfil);
        QCOMPARE(esperados.turnos, 2);
        QCOMPARE(esperados.eventos, 3);
        QCOMPARE(esperados.tiposAula, 2);
        QCOMPARE(esperados.aulas, 13);
        QCOMPARE(esperados.materias, 85);
        QCOMPARE(esperados.planes, 2);
        QCOMPARE(esperados.planMaterias, 95);
        QCOMPARE(esperados.cursos, 24);
        QCOMPARE(esperados.docentes, 32);
        QCOMPARE(esperados.asignaciones, 272);
        QCOMPARE(esperados.disponibilidades, 160);
        QCOMPARE(esperados.preferencias, 0);
        QCOMPARE(esperados.duracionMinutos, 40);
        QCOMPARE(esperados.configuraciones, 1);
    }

    // ── Demanda y áreas con cursos (RF-8, plan §5) ────────────────────────
    void horasDeCursoYDemanda()
    {
        const Perfil& perfil = perfilPorDefecto();

        const int horasPorAnio31059[5] = {34, 34, 34, 36, 34};
        for (const CursoDef& curso : perfil.cursos) {
            const int esperadas = curso.plan == QStringLiteral("31059")
                                      ? horasPorAnio31059[curso.anio - 1]
                                      : 40;
            QCOMPARE(horasDeCurso(perfil, curso), esperadas);
        }

        CursoDef desconocido;
        desconocido.anio = 1;
        desconocido.plan = QStringLiteral("99999");
        QCOMPARE(horasDeCurso(perfil, desconocido), -1);

        // 876 clases = mañana 414 (31059) + tarde 462 (31059 2.º y 31060).
        QCOMPARE(demandaTotal(perfil), 876);
        QCOMPARE(demandaPorTurno(perfil, QStringLiteral("Mañana")), 414);
        QCOMPARE(demandaPorTurno(perfil, QStringLiteral("Tarde")), 462);
        QCOMPARE(demandaPorTurno(perfil, QStringLiteral("Noche")), -1);
    }

    void areasConCursosEnTurno()
    {
        const Perfil& perfil = perfilPorDefecto();

        // «Biología» solo tiene cursos de 31059 en la mañana; «Biología, Ambiente y
        // Tecnología» solo tiene cursos de 31060 en la tarde.
        QVERIFY(areaConCursosEnTurno(perfil, QStringLiteral("Biología"), QStringLiteral("Mañana")));
        QVERIFY(!areaConCursosEnTurno(perfil, QStringLiteral("Biología"), QStringLiteral("Tarde")));
        QVERIFY(!areaConCursosEnTurno(perfil, QStringLiteral("Biología, Ambiente y Tecnología"),
                                      QStringLiteral("Mañana")));
        QVERIFY(areaConCursosEnTurno(perfil, QStringLiteral("Biología, Ambiente y Tecnología"),
                                     QStringLiteral("Tarde")));

        // Ciencias Naturales y Arte existen en 31059 1.º, 2.º y 4.º: hay cursos en ambos turnos.
        QVERIFY(areaConCursosEnTurno(perfil, QStringLiteral("Ciencias Naturales"),
                                     QStringLiteral("Mañana")));
        QVERIFY(areaConCursosEnTurno(perfil, QStringLiteral("Ciencias Naturales"),
                                     QStringLiteral("Tarde")));
        QVERIFY(areaConCursosEnTurno(perfil, QStringLiteral("Arte y Patrimonio"),
                                     QStringLiteral("Tarde")));

        // Las áreas excluidas de los planes no tienen cursos en ningún turno.
        for (const char* area :
             {"Ciencias de la Tierra",
              "Participación en los Grupos de Creación, Recreación y Producción"}) {
            for (const TurnoDef& turno : perfil.turnos) {
                QVERIFY2(!areaConCursosEnTurno(perfil, QString::fromUtf8(area), turno.nombre),
                         area);
            }
        }
    }

    void motivoNoGenerableExplicaLaCancha()
    {
        const Perfil& perfil = perfilPorDefecto();

        // La Educación Física de cada turno son 48 clases (4 h × 12 cursos).
        QCOMPARE(demandaDeAreaPorTurno(perfil, QStringLiteral("Educación Física"),
                                       QStringLiteral("Mañana")), 48);
        QCOMPARE(demandaDeAreaPorTurno(perfil, QStringLiteral("Educación Física"),
                                       QStringLiteral("Tarde")), 48);
        QCOMPARE(demandaDeAreaPorTurno(perfil, QStringLiteral("Educación Física"),
                                       QStringLiteral("Noche")), -1);

        // El motivo cita las 48 clases frente a las 40 franjas de cancha de un turno y la
        // acción pendiente (el aforo simultáneo de su spec).
        const QString motivo = motivoNoGenerable(perfil);
        QVERIFY2(!motivo.isEmpty(), "el centro real no debe declararse generable");
        QVERIFY2(motivo.contains(QStringLiteral("48")), qPrintable(motivo));
        QVERIFY2(motivo.contains(QStringLiteral("40")), qPrintable(motivo));
        QVERIFY2(motivo.contains(QStringLiteral("Educación Física")), qPrintable(motivo));
        QVERIFY2(motivo.contains(QStringLiteral("cancha")), qPrintable(motivo));
        QVERIFY2(motivo.contains(QStringLiteral("aforo simultáneo")), qPrintable(motivo));
        QVERIFY2(motivo.contains(perfil.turnos[0].nombre), qPrintable(motivo));

        // Sin cancha el motivo identifica la causa.
        Perfil sinCancha = perfil;
        for (AulaDef& aula : sinCancha.aulas) {
            if (aula.tipo == QStringLiteral("Cancha")) {
                aula.tipo = QStringLiteral("Aula de estudio");
            }
        }
        const QString sinCanchaMotivo = motivoNoGenerable(sinCancha);
        QVERIFY2(sinCanchaMotivo.contains(QStringLiteral("Cancha")), qPrintable(sinCanchaMotivo));
        QVERIFY2(sinCanchaMotivo.contains(QStringLiteral("no tiene")), qPrintable(sinCanchaMotivo));

        // Sin cursos no hay demanda de Educación Física: el conjunto cabría y no hay motivo.
        Perfil sinCursos = perfil;
        sinCursos.cursos.clear();
        QVERIFY2(motivoNoGenerable(sinCursos).isEmpty(), "sin cursos no debe haber motivo");
    }

    // ── Docentes (RF-6) ───────────────────────────────────────────────────
    void docentesDelCentroYAsignaciones()
    {
        const Perfil& perfil = perfilPorDefecto();
        const QVector<DocenteDef> docentes = docentesDelCentro(perfil);

        QCOMPARE(docentes.size(), 32);
        QCOMPARE(asignacionesDe(docentes), 272);
        QCOMPARE(disponibilidadesDe(docentes), 160);
        QCOMPARE(asignacionesEnTurno(docentes, QStringLiteral("Mañana")), 147);
        QCOMPARE(asignacionesEnTurno(docentes, QStringLiteral("Tarde")), 125);

        QCOMPARE(docentes[0].id, QStringLiteral("P001"));
        QCOMPARE(docentes[31].id, QStringLiteral("P032"));
        QCOMPARE(docentes[31].email, QStringLiteral("p032@centro-de-prueba.edu.ve"));

        QSet<QString> nombres;
        QSet<QString> emails;
        int manana = 0;
        int tarde = 0;
        for (const DocenteDef& docente : docentes) {
            QVERIFY2(docente.id.startsWith(QStringLiteral("P0")), qPrintable(docente.id));
            QVERIFY2(!nombres.contains(docente.nombre), qPrintable(docente.nombre));
            nombres.insert(docente.nombre);
            QVERIFY2(!emails.contains(docente.email), qPrintable(docente.email));
            emails.insert(docente.email);
            QCOMPARE(docente.horasAula, 36);
            if (docente.turno == QStringLiteral("Mañana")) {
                ++manana;
            } else {
                ++tarde;
            }
        }
        QCOMPARE(manana, 16);
        QCOMPARE(tarde, 16);

        // Tabla por área de la spec (plan §4.7).
        const struct {
            const char* area;
            int asignaciones;
        } kPorArea[] = {
            {"Castellano", 25},
            {"Inglés y otras lenguas extranjeras", 20},
            {"Matemáticas", 20},
            {"Educación Física", 20},
            {"Arte y Patrimonio", 4},
            {"Ciencias Naturales", 6},
            {"Física", 20},
            {"Química", 15},
            {"Biología", 9},
            {"Biología, Ambiente y Tecnología", 5},
            {"Geografía, Historia y Ciudadanía", 30},
            {"Formación para la Soberanía Nacional", 2},
            {"Orientación y Convivencia", 40},
            {"Lengua y Literatura", 10},
            {"Idiomas", 5},
            {"Geografía, Historia y Soberanía Nacional", 10},
            {"Orientación Vocacional", 15},
            {"Innovación Tecnológica y Productividad", 10},
            {"Participación en los Grupos de Creación, Recreación y Producción", 5},
            {"Ciencias de la Tierra", 1},
        };
        int suma = 0;
        for (const auto& fila : kPorArea) {
            const int n = asignacionesDelArea(perfil, docentes, QString::fromUtf8(fila.area));
            QVERIFY2(n == fila.asignaciones,
                     qPrintable(QStringLiteral("%1: %2 (se esperaban %3)")
                                    .arg(QString::fromUtf8(fila.area))
                                    .arg(n)
                                    .arg(fila.asignaciones)));
            suma += n;
        }
        QCOMPARE(suma, 272);

        // Anclas de la spec: P029 solo «Biología, Ambiente y Tecnología» (5, no Biología),
        // P012/P013/P015 reciben Biología (3 cada una) y P028 suma 10.
        const QSet<QString> bat =
            materiasDelArea(perfil, QStringLiteral("Biología, Ambiente y Tecnología"));
        const QSet<QString> biologia = materiasDelArea(perfil, QStringLiteral("Biología"));
        QCOMPARE(docentes[28].id, QStringLiteral("P029"));
        QCOMPARE(docentes[28].materias.size(), 5);
        for (const QString& materia : docentes[28].materias) {
            QVERIFY2(bat.contains(materia), qPrintable(materia));
            QVERIFY2(!biologia.contains(materia), qPrintable(materia));
        }
        const int conBiologia[3] = {11, 12, 14};
        for (int indice : conBiologia) {
            int n = 0;
            for (const QString& materia : docentes[indice].materias) {
                if (biologia.contains(materia)) {
                    ++n;
                }
            }
            QCOMPARE(n, 3);
        }
        QCOMPARE(docentes[27].id, QStringLiteral("P028"));
        QCOMPARE(docentes[27].materias.size(), 10);

        // Los 2 añadidos (P031 y P032): 5 materias de Participación en la mañana y 1 de
        // Ciencias de la Tierra en la tarde, sin carga en ningún plan.
        QCOMPARE(docentes[30].id, QStringLiteral("P031"));
        QCOMPARE(docentes[30].turno, QStringLiteral("Mañana"));
        QCOMPARE(docentes[30].materias.size(), 5);
        QCOMPARE(docentes[31].id, QStringLiteral("P032"));
        QCOMPARE(docentes[31].turno, QStringLiteral("Tarde"));
        QCOMPARE(docentes[31].materias.size(), 1);
        QCOMPARE(docentes[31].materias.first(), QStringLiteral("Ciencias de la Tierra 5º"));

        // Disponibilidad (RF-6, RF-7): 5 filas de lunes a viernes con la ventana del turno.
        for (const DocenteDef& docente : docentes) {
            QCOMPARE(docente.disponibilidad.size(), 5);
            const QTime esperadoInicio =
                docente.turno == QStringLiteral("Mañana") ? QTime(7, 0) : QTime(12, 40);
            const QTime esperadoFin =
                docente.turno == QStringLiteral("Mañana") ? QTime(12, 40) : QTime(18, 10);
            QSet<int> dias;
            for (const FranjaDef& franja : docente.disponibilidad) {
                QVERIFY(franja.dia >= 1 && franja.dia <= 5);
                QVERIFY2(!dias.contains(franja.dia), qPrintable(docente.id));
                dias.insert(franja.dia);
                QCOMPARE(franja.inicio, esperadoInicio);
                QCOMPARE(franja.fin, esperadoFin);
            }
            QCOMPARE(dias.size(), 5);
        }
    }

    void asignacionesDeQuintoDe31060()
    {
        const Perfil& perfil = perfilPorDefecto();
        const QVector<DocenteDef> docentes = docentesDelCentro(perfil);

        // Las 10 materias del 5.º de 31060 (año sin cursos: no aporta clases).
        const PlanDef& ciencias = planPorCodigo(perfil, QStringLiteral("31060"));
        QSet<QString> materiasQuinto;
        for (const CargaPlan& carga : ciencias.cargas) {
            if (carga.anio == 5) {
                materiasQuinto.insert(carga.materia);
            }
        }
        QCOMPARE(materiasQuinto.size(), 10);

        // 18 asignaciones de esas materias a docentes de la tarde (los que no tienen cursos
        // de 5.º de 31060). Las compartidas con 31059 5.º también van a la mañana, que sí los
        // usa; por eso el total de la materia no es 18.
        int enTarde = 0;
        for (const DocenteDef& docente : docentes) {
            if (docente.turno != QStringLiteral("Tarde")) {
                continue;
            }
            for (const QString& materia : docente.materias) {
                if (materiasQuinto.contains(materia)) {
                    ++enTarde;
                }
            }
        }
        QCOMPARE(enTarde, 18);
    }

    void coberturaYValidacionDeDocentes()
    {
        const Perfil& perfil = perfilPorDefecto();
        const QVector<DocenteDef> docentes = docentesDelCentro(perfil);

        QString error;
        QVERIFY2(validarDocentes(perfil, docentes, &error), qPrintable(error));
        QVERIFY2(coberturaDelCentro(perfil, docentes, &error), qPrintable(error));

        // 31 docentes.
        {
            QVector<DocenteDef> recortados = docentes;
            recortados.removeLast();
            QVERIFY(!validarDocentes(perfil, recortados, &error));
            QVERIFY2(error.contains(QStringLiteral("32")), qPrintable(error));
        }

        // Identificador duplicado.
        {
            QVector<DocenteDef> copia = docentes;
            copia[1].id = copia[0].id;
            QVERIFY(!validarDocentes(perfil, copia, &error));
            QVERIFY2(error.contains(QStringLiteral("duplicado")), qPrintable(error));
        }

        // Tope de horas de aula distinto de 36.
        {
            QVector<DocenteDef> copia = docentes;
            copia[0].horasAula = 40;
            QVERIFY(!validarDocentes(perfil, copia, &error));
            QVERIFY2(error.contains(QStringLiteral("36")), qPrintable(error));
        }

        // Una fila de disponibilidad de menos.
        {
            QVector<DocenteDef> copia = docentes;
            copia[0].disponibilidad.removeLast();
            QVERIFY(!validarDocentes(perfil, copia, &error));
            QVERIFY2(error.contains(QStringLiteral("5")), qPrintable(error));
        }

        // Disponibilidad fuera de la ventana del turno (19:00–21:00).
        {
            QVector<DocenteDef> copia = docentes;
            for (FranjaDef& franja : copia[0].disponibilidad) {
                franja.inicio = QTime(19, 0);
                franja.fin = QTime(21, 0);
            }
            QVERIFY(!validarDocentes(perfil, copia, &error));
            QVERIFY2(error.contains(QStringLiteral("ventana")), qPrintable(error));
        }

        // Materia sin docente: se quita «Ciencias de la Tierra 5º» a P032.
        {
            QVector<DocenteDef> copia = docentes;
            copia[31].materias.clear();
            QVERIFY(!coberturaDelCentro(perfil, copia, &error));
            QVERIFY2(error.contains(QStringLiteral("Ciencias de la Tierra 5º")), qPrintable(error));
        }

        // «Biología, Ambiente y Tecnología» sin docente en la tarde: se mueve su asignación
        // de P029 a un docente de la mañana (el recuento total no cambia, la cobertura falla).
        {
            QVector<DocenteDef> copia = docentes;
            const QSet<QString> bat =
                materiasDelArea(perfil, QStringLiteral("Biología, Ambiente y Tecnología"));
            QStringList traspasadas;
            for (const QString& materia : copia[28].materias) {
                if (bat.contains(materia)) {
                    traspasadas << materia;
                }
            }
            copia[28].materias.removeAll(traspasadas.first());
            copia[0].materias << traspasadas.first();
            QVERIFY2(validarDocentes(perfil, copia, &error), qPrintable(error));
            QVERIFY(!coberturaDelCentro(perfil, copia, &error));
            QVERIFY2(error.contains(QStringLiteral("Biología, Ambiente y Tecnología 1º")),
                     qPrintable(error));
        }
    }

    // ── Validación del perfil (plan §6) ───────────────────────────────────
    void validarPerfilAceptaPorDefecto()
    {
        QString error;
        QVERIFY2(validarPerfil(perfilPorDefecto(), &error), qPrintable(error));
    }

    void validarPerfilRechazaDuracionYTurnos()
    {
        const Perfil& base = perfilPorDefecto();
        QCOMPARE(base.duracionMinutos, 40);
        QCOMPARE(base.turnos.size(), 2);

        // Duración fuera del rango 30–90.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.duracionMinutos = 20;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("30")), qPrintable(error));
        }

        // La ventana de la mañana debe cerrar a las 12:40.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.turnos[0].fin = QTime(12, 30);
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("12:40")), qPrintable(error));
        }

        // Un evento fuera de la ventana de su posición con la duración de 40.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.turnos[0].eventos[0].inicio = QTime(8, 0);
            perfil.turnos[0].eventos[0].fin = QTime(8, 10);
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("Receso")), qPrintable(error));
            QVERIFY2(error.contains(QStringLiteral("ventana")), qPrintable(error));
        }

        // Duración que no coincide con la diferencia entre las horas.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.turnos[0].eventos[0].duracion = 15;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("duración")), qPrintable(error));
        }

        // Un tercer evento de 20 min rompe el encaje con 40: 8 × 40 + 40 = 360 > 340.
        {
            Perfil perfil = perfilPorDefecto();
            EventoDef extra{QStringLiteral("Formación"), 4, 1, 20, QTime(), QTime()};
            perfil.turnos[0].eventos.append(extra);
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("Mañana")), qPrintable(error));
            QVERIFY2(error.contains(QStringLiteral("déficit 20")), qPrintable(error));
        }
    }

    void validarPerfilRechazaAulas()
    {
        // Un aula duplicada.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.aulas.append(perfil.aulas[0]);
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("duplicada")), qPrintable(error));
        }

        // Un aula general con capacidad menor que el grupo mayor (44).
        {
            Perfil perfil = perfilPorDefecto();
            perfil.aulas[0].capacidad = 40;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("44")), qPrintable(error));
        }

        // Una cancha que no cubre dos grupos.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.aulas[12].capacidad = 80;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("88")), qPrintable(error));
        }

        // Un curso con la cancha como aula de referencia.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[0].aula = QStringLiteral("Cancha");
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("uso general")), qPrintable(error));
        }

        // Una materia que exige un tipo sin aulas no es alcanzable con el conjunto real: la
        // única exigencia especial es la cancha y el perfil la siembra. Se cubren los rechazos
        // del catálogo de tipos: vacío y duplicado.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.tiposAula.clear();
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("tipos de aula")), qPrintable(error));
        }
        {
            Perfil perfil = perfilPorDefecto();
            perfil.tiposAula.append(QStringLiteral("Cancha"));
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("duplicado en el catálogo")), qPrintable(error));
        }
    }

    void validarPerfilRechazaCursos()
    {
        const Perfil& base = perfilPorDefecto();
        QCOMPARE(base.cursos.size(), 24);

        // Un aula que no existe.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[0].aula = QStringLiteral("Aula Fantasma");
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("inexistente")), qPrintable(error));
        }

        // Una pareja año+sección repetida.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[1].seccion = QStringLiteral("A");
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("año+sección")), qPrintable(error));
        }

        // Dos cursos del mismo turno con el mismo aula de referencia.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[1].aula = perfil.cursos[0].aula;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("se repite entre cursos del turno")),
                     qPrintable(error));
        }

        // Matrícula fuera de 32–44.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[0].matricula = 31;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("32")), qPrintable(error));
        }

        // Plan inexistente.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[0].plan = QStringLiteral("99999");
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("99999")), qPrintable(error));
        }

        // Turno inexistente.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[0].turno = QStringLiteral("Noche");
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("Noche")), qPrintable(error));
        }

        // Un curso de menos.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos.removeLast();
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("24")), qPrintable(error));
        }
    }

    void validarPerfilRechazaCatalogo()
    {
        // Una materia de menos: al quitar una sin carga quedan 5 y la validación de D-4 lo
        // rechaza (el recuento de 85 materias es defensivo y lo cubren las tablas canónicas).
        {
            Perfil perfil = perfilPorDefecto();
            perfil.materias.removeLast();
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("6 materias sin carga")), qPrintable(error));
        }

        // Una materia con carga marcada como sin carga.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.materias[0].sinCarga = true;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("aparece en un plan")), qPrintable(error));
        }

        // Una materia sin carga de más.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.materias.append(MateriaAnio{QStringLiteral("Otra 3º"), 3,
                                               QStringLiteral("Ciencias de la Tierra"), true});
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("6 materias sin carga")), qPrintable(error));
        }

        // Una materia con carga que no aparece en ningún plan.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.materias[0].sinCarga = false;
            perfil.materias.append(MateriaAnio{QStringLiteral("Huérfana 3º"), 3,
                                               QStringLiteral("Castellano"), false});
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("ningún plan")), qPrintable(error));
        }

        // Una materia duplicada.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.materias.append(perfil.materias[0]);
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("duplicada")), qPrintable(error));
        }
    }

    void validarPerfilRechazaPlanes()
    {
        // Un plan desconocido (los derivados EXA/NAT/SOC ya no existen).
        {
            Perfil perfil = perfilPorDefecto();
            PlanDef derivado = perfil.planes[0];
            derivado.codigo = QStringLiteral("EXA");
            perfil.planes.append(derivado);
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("EXA")), qPrintable(error));
        }

        // Un año con menos materias de las canónicas: se quita una compartida de 31059, que
        // sigue en 31060, para que el catálogo no quede huérfano.
        {
            Perfil perfil = perfilPorDefecto();
            for (int i = 0; i < perfil.planes[0].cargas.size(); ++i) {
                if (perfil.planes[0].cargas[i].materia == QStringLiteral("Matemáticas 1º")) {
                    perfil.planes[0].cargas.removeAt(i);
                    break;
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("año 1")), qPrintable(error));
            QVERIFY2(error.contains(QStringLiteral("7 materias")), qPrintable(error));
        }

        // Un año que deja de ser par (4.º: 36 → 35).
        {
            Perfil perfil = perfilPorDefecto();
            for (CargaPlan& carga : perfil.planes[0].cargas) {
                if (carga.anio == 4 && carga.materia == QStringLiteral("Castellano 4º")) {
                    carga.horas = 3;
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("par")), qPrintable(error));
        }

        // Horas distintas de la tabla aunque el año siga sumando 34.
        {
            Perfil perfil = perfilPorDefecto();
            for (CargaPlan& carga : perfil.planes[0].cargas) {
                if (carga.anio != 1) {
                    continue;
                }
                if (carga.materia == QStringLiteral("Castellano 1º")) {
                    carga.horas = 5;
                } else if (carga.materia == QStringLiteral("Geografía, Historia y Ciudadanía 1º")) {
                    carga.horas = 5;
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("Castellano 1º")), qPrintable(error));
            QVERIFY2(error.contains(QStringLiteral("se esperaban 4")), qPrintable(error));
        }

        // Una carga de un área que el plan no incluye: se cambia una compartida por un área
        // exclusiva del 31060 (la materia compartida sigue en el otro plan).
        {
            Perfil perfil = perfilPorDefecto();
            for (CargaPlan& carga : perfil.planes[0].cargas) {
                if (carga.materia == QStringLiteral("Matemáticas 1º")) {
                    carga.materia = QStringLiteral("Lengua y Literatura 1º");
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("no incluye el área")), qPrintable(error));
        }
    }
};

QTEST_MAIN(TestSeedPerfil)
#include "test_seed_perfil.moc"
