#include <QSet>
#include <QTest>
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

} // namespace

class TestSeedPerfil : public QObject {
    Q_OBJECT

private slots:
    void bloquesEsLaIdentidad()
    {
        QCOMPARE(bloques(0), 0);
        QCOMPARE(bloques(1), 1);
        QCOMPARE(bloques(2), 2);
        QCOMPARE(bloques(3), 3);
        QCOMPARE(bloques(4), 4);
        QCOMPARE(bloques(5), 5);
        QCOMPARE(bloques(6), 6);
        QCOMPARE(bloques(40), 40);
        QCOMPARE(bloques(46), 46);
    }

    void catalogoTiene57MateriasY4Planes()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(perfil.materias.size(), 57);
        QCOMPARE(perfil.planes.size(), 4);
    }

    void planGeneralReproduceTablaAdaptada()
    {
        const Perfil& perfil = perfilPorDefecto();
        const PlanDef& general = planPorCodigo(perfil, QStringLiteral("GEN"));

        QVERIFY(!general.nombre.isEmpty());
        QCOMPARE(general.nombre, QString("General"));
        QCOMPARE(general.cargas.size(), 51);

        const int materiasPorAnio[5] = {9, 9, 10, 11, 12};
        const int horasPorAnio[5] = {40, 40, 40, 40, 40};
        for (int anio = 1; anio <= 5; ++anio) {
            QCOMPARE(materiasDeAnio(general, anio), materiasPorAnio[anio - 1]);
            QCOMPARE(horasDeAnio(general, anio), horasPorAnio[anio - 1]);
        }
    }

    void horasYBloquesDeAnioCoherentes()
    {
        const Perfil& perfil = perfilPorDefecto();
        const PlanDef& general = planPorCodigo(perfil, QStringLiteral("GEN"));

        for (int anio = 1; anio <= 5; ++anio) {
            QCOMPARE(bloquesDeAnio(general, anio), horasDeAnio(general, anio));
            QCOMPARE(bloquesDeAnio(general, anio), 40);
        }
    }

    void horasDeAnioSinAnioDevuelveMenosUno()
    {
        PlanDef plan;
        plan.cargas = {CargaPlan{QStringLiteral("Materia 1º"), 1, 4}};

        QCOMPARE(horasDeAnio(plan, 1), 4);
        QCOMPARE(horasDeAnio(plan, 2), -1);
        QCOMPARE(bloquesDeAnio(plan, 2), -1);
    }

    void seccionesDeAnioValores()
    {
        QCOMPARE(seccionesDeAnio(1), 3);
        QCOMPARE(seccionesDeAnio(2), 3);
        QCOMPARE(seccionesDeAnio(3), 3);
        QCOMPARE(seccionesDeAnio(4), 2);
        QCOMPARE(seccionesDeAnio(5), 2);
    }

    void cursosAulasYTurnos()
    {
        const Perfil& perfil = perfilPorDefecto();

        // ── Turnos (RF-4): 2, mañana y tarde, 8 franjas y receso de 10 min tras la franja 2.
        QCOMPARE(perfil.turnos.size(), 2);
        const TurnoDef& manana = perfil.turnos[0];
        const TurnoDef& tarde = perfil.turnos[1];

        QCOMPARE(manana.nombre, QString("Mañana"));
        QCOMPARE(manana.inicio, QTime(7, 0));
        QCOMPARE(manana.fin, QTime(12, 30));
        QCOMPARE(manana.numSlots, 8);
        QCOMPARE(manana.recesoDespuesDeSlot, 2);
        QCOMPARE(manana.recesoInicio, QTime(8, 20));
        QCOMPARE(manana.recesoFin, QTime(8, 30));

        QCOMPARE(tarde.nombre, QString("Tarde"));
        QCOMPARE(tarde.inicio, QTime(12, 40));
        QCOMPARE(tarde.fin, QTime(18, 10));
        QCOMPARE(tarde.numSlots, 8);
        QCOMPARE(tarde.recesoDespuesDeSlot, 2);
        QCOMPARE(tarde.recesoInicio, QTime(14, 0));
        QCOMPARE(tarde.recesoFin, QTime(14, 10));

        // ── Aulas (RF-5): 16, nombre único, con laboratorio o taller.
        QCOMPARE(perfil.aulas.size(), 16);
        QSet<QString> nombresAula;
        bool hayLaboratorio = false;
        bool hayTaller = false;
        for (const AulaDef& aula : perfil.aulas) {
            QVERIFY2(!nombresAula.contains(aula.nombre), qPrintable(aula.nombre));
            nombresAula.insert(aula.nombre);
            QVERIFY(aula.capacidad > 0);
            QVERIFY(!aula.edificio.isEmpty());
            QVERIFY(!aula.piso.isEmpty());
            if (aula.nombre.contains(QStringLiteral("Laboratorio"))) {
                hayLaboratorio = true;
            }
            if (aula.nombre.contains(QStringLiteral("Taller"))) {
                hayTaller = true;
            }
        }
        QVERIFY(hayLaboratorio || hayTaller);

        // ── Cursos (RF-4, RF-5): 13, sin turno (el tipo no lo declara), secciones de un
        //    carácter en mayúscula, pareja año+sección única y aula existente y distinta.
        QCOMPARE(perfil.cursos.size(), 13);
        QSet<QString> parejas;
        QSet<QString> aulasUsadas;
        for (const CursoDef& curso : perfil.cursos) {
            QVERIFY(curso.anio >= 1 && curso.anio <= 5);
            QCOMPARE(curso.seccion.size(), 1);
            QVERIFY(curso.seccion.at(0).isUpper());
            const QString pareja =
                QStringLiteral("%1%2").arg(curso.anio).arg(curso.seccion);
            QVERIFY2(!parejas.contains(pareja), qPrintable(pareja));
            parejas.insert(pareja);
            QVERIFY2(nombresAula.contains(curso.aula), qPrintable(curso.aula));
            QVERIFY2(!aulasUsadas.contains(curso.aula), qPrintable(curso.aula));
            aulasUsadas.insert(curso.aula);
        }

        // Secciones por año: 3 en 1.º–3.º y 2 en 4.º–5.º (RF-4).
        QCOMPARE(aulasUsadas.size(), 13);
    }

    void validarPerfilRechazaTurnosAulasYCursos()
    {
        const Perfil& base = perfilPorDefecto();
        // Guarda para la fase roja: si el perfil aún no se rellena, aborta sin indexar.
        QCOMPARE(base.turnos.size(), 2);
        QCOMPARE(base.aulas.size(), 16);
        QCOMPARE(base.cursos.size(), 13);

        // Un turno con el receso fuera de la franja 2.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.turnos[0].recesoDespuesDeSlot = 1;
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("franja 2")), qPrintable(error));
        }

        // Un aula duplicada.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.aulas.append(perfil.aulas[0]);
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("duplicada")), qPrintable(error));
        }

        // Un curso con un aula que no existe.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[0].aula = QStringLiteral("Aula Fantasma");
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("inexistente")), qPrintable(error));
        }

        // Una pareja año+sección repetida (1ºA dos veces).
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos[1].seccion = QStringLiteral("A");   // 1ºA duplicado
            perfil.cursos[1].aula = QStringLiteral("Aula 14"); // aula libre para no disparar antes
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("año+sección")), qPrintable(error));
        }
    }

    void planesDerivadosCumplenReglas()
    {
        const Perfil& perfil = perfilPorDefecto();
        const QString derivados[] = {QStringLiteral("EXA"), QStringLiteral("NAT"),
                                     QStringLiteral("SOC")};

        for (const QString& codigo : derivados) {
            const PlanDef& plan = planPorCodigo(perfil, codigo);
            QVERIFY2(!plan.nombre.isEmpty(), qPrintable(codigo));
            QCOMPARE(plan.cargas.size(), 53);

            // Σ horas = 40 en cada año (RF-3): la carga cabe en las 40 franjas del turno.
            for (int anio = 1; anio <= 5; ++anio) {
                QCOMPARE(horasDeAnio(plan, anio), 40);
            }

            // ≥2 materias nuevas propias del área, todas en 4.º–5.º.
            int nuevas = 0;
            for (const CargaPlan& carga : plan.cargas) {
                if (esMateriaNueva(carga.materia)) {
                    QVERIFY2(carga.anio >= 4 && carga.anio <= 5,
                             qPrintable(carga.materia));
                    ++nuevas;
                }
            }
            QVERIFY2(nuevas >= 2, qPrintable(codigo));
        }
    }

    void catalogoSinHuerfanas()
    {
        const Perfil& perfil = perfilPorDefecto();

        QSet<QString> nombres;
        for (const MateriaAnio& materia : perfil.materias) {
            QVERIFY2(!nombres.contains(materia.nombre), qPrintable(materia.nombre));
            nombres.insert(materia.nombre);
        }

        // Cada materia usada por ≥1 plan y como máximo una vez por plan.
        for (const MateriaAnio& materia : perfil.materias) {
            int usos = 0;
            for (const PlanDef& plan : perfil.planes) {
                int enEstePlan = 0;
                for (const CargaPlan& carga : plan.cargas) {
                    if (carga.materia == materia.nombre) {
                        ++enEstePlan;
                    }
                }
                QVERIFY(enEstePlan <= 1);
                usos += enEstePlan;
            }
            QVERIFY2(usos >= 1, qPrintable(materia.nombre));
        }
    }

    void progresionDel31059()
    {
        const Perfil& perfil = perfilPorDefecto();

        struct Regla {
            const char* area;
            int anioMin;
            int anioMax;
        };
        const Regla reglas[] = {
            {"Arte y Patrimonio", 1, 2},
            {"Ciencias Naturales", 1, 2},
            {"Física", 3, 5},
            {"Química", 3, 5},
            {"Biología", 3, 5},
            {"Ciencias de la Tierra", 5, 5},
            {"Formación para la Soberanía Nacional", 4, 5},
        };

        for (const PlanDef& plan : perfil.planes) {
            for (const CargaPlan& carga : plan.cargas) {
                if (esMateriaNueva(carga.materia)) {
                    continue;
                }
                for (const Regla& regla : reglas) {
                    const QString prefijo =
                        QString::fromUtf8(regla.area) + QStringLiteral(" ");
                    if (carga.materia.startsWith(prefijo)) {
                        QVERIFY2(carga.anio >= regla.anioMin && carga.anio <= regla.anioMax,
                                 qPrintable(carga.materia));
                    }
                }
            }
        }
    }

    void catalogoOrdenFijo()
    {
        const Perfil& perfil = perfilPorDefecto();

        QCOMPARE(perfil.materias.size(), 57);
        QCOMPARE(perfil.materias[22].nombre, QString("Física 3º"));

        // Materias nuevas al final de su año, en el orden fijo del plan §4.2 (RF-11).
        QCOMPARE(perfil.materias[39].nombre, QString("Matemática Aplicada"));
        QCOMPARE(perfil.materias[40].nombre, QString("Química Aplicada"));
        QCOMPARE(perfil.materias[41].nombre, QString("Geografía Económica"));
        QCOMPARE(perfil.materias[54].nombre, QString("Física Aplicada"));
        QCOMPARE(perfil.materias[55].nombre, QString("Ciencias de la Tierra Aplicadas"));
        QCOMPARE(perfil.materias[56].nombre, QString("Historia Contemporánea"));
    }

    void validarPerfilAceptaPorDefecto()
    {
        QString error;
        QVERIFY2(validarPerfil(perfilPorDefecto(), &error), qPrintable(error));
    }

    void validarPerfilRechazaCorrupciones()
    {
        // Un plan con un año por encima de 40 horas académicas.
        {
            Perfil perfil = perfilPorDefecto();
            for (PlanDef& plan : perfil.planes) {
                if (plan.codigo != QStringLiteral("EXA")) {
                    continue;
                }
                for (CargaPlan& carga : plan.cargas) {
                    if (carga.anio == 4 && carga.materia == QStringLiteral("Matemática Aplicada")) {
                        carga.horas = 4;  // 4.º pasa de 40 a 41.
                    }
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("40 horas académicas")), qPrintable(error));
        }

        // Una materia del catálogo sin ningún plan (huérfana).
        {
            Perfil perfil = perfilPorDefecto();
            perfil.materias.append(MateriaAnio{QStringLiteral("Materia Huérfana 5º"), 5});
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("ningún plan")), qPrintable(error));
        }

        // Un aula repetida entre dos cursos.
        {
            Perfil perfil = perfilPorDefecto();
            perfil.cursos.append(CursoDef{1, QStringLiteral("A"), QStringLiteral("Aula 01")});
            perfil.cursos.append(CursoDef{1, QStringLiteral("B"), QStringLiteral("Aula 01")});
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("Aula 01")), qPrintable(error));
        }
    }

    void validarPerfilRechazaGeneralConHorasDesviadas()
    {
        // Quitar 1 h a «Castellano 1º»: el año 1 suma 39 (se esperaban 40) y el mensaje
        // debe indicar el año y la diferencia (RF-2, plan §5.3).
        {
            Perfil perfil = perfilPorDefecto();
            for (PlanDef& plan : perfil.planes) {
                if (plan.codigo != QStringLiteral("GEN")) {
                    continue;
                }
                for (CargaPlan& carga : plan.cargas) {
                    if (carga.materia == QStringLiteral("Castellano 1º")) {
                        carga.horas = 3;
                    }
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("año 1")), qPrintable(error));
            QVERIFY2(error.contains(QStringLiteral("39")), qPrintable(error));
            QVERIFY2(error.contains(QStringLiteral("diferencia -1")), qPrintable(error));
        }

        // Horas distintas de la tabla aunque el año siga sumando 40 (compensado en otra
        // materia del mismo año): debe rechazarse por la tabla canónica.
        {
            Perfil perfil = perfilPorDefecto();
            for (PlanDef& plan : perfil.planes) {
                if (plan.codigo != QStringLiteral("GEN")) {
                    continue;
                }
                for (CargaPlan& carga : plan.cargas) {
                    if (carga.materia == QStringLiteral("Castellano 1º")) {
                        carga.horas = 3;
                    } else if (carga.materia ==
                               QStringLiteral("Inglés y otras lenguas extranjeras 1º")) {
                        carga.horas = 6;
                    }
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("Castellano 1º")), qPrintable(error));
        }
    }

    void validarPerfilRechazaGeneralConMateriasDesviadas()
    {
        // Quitar una materia de un año del plan General cambia el recuento 9/9/10/11/12.
        Perfil perfil = perfilPorDefecto();
        for (PlanDef& plan : perfil.planes) {
            if (plan.codigo != QStringLiteral("GEN")) {
                continue;
            }
            for (int i = 0; i < plan.cargas.size(); ++i) {
                if (plan.cargas[i].materia == QStringLiteral("Orientación y Convivencia 1º")) {
                    plan.cargas.removeAt(i);
                    break;
                }
            }
        }
        QString error;
        QVERIFY(!validarPerfil(perfil, &error));
        QVERIFY2(error.contains(QStringLiteral("año 1")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("8 materias")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("se esperaban 9")), qPrintable(error));
    }

    void validarPerfilRechazaRecuentos()
    {
        // Falta una carga en un derivado → 209 materias de plan (se esperaban 210).
        {
            Perfil perfil = perfilPorDefecto();
            for (PlanDef& plan : perfil.planes) {
                if (plan.codigo != QStringLiteral("EXA")) {
                    continue;
                }
                for (int i = 0; i < plan.cargas.size(); ++i) {
                    if (plan.cargas[i].materia == QStringLiteral("Educación Física 5º")) {
                        plan.cargas.removeAt(i);
                        break;
                    }
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("210")), qPrintable(error));
        }

        // Falta una materia del catálogo → 56 materias (se esperaban 57).
        {
            Perfil perfil = perfilPorDefecto();
            for (int i = 0; i < perfil.materias.size(); ++i) {
                if (perfil.materias[i].nombre == QStringLiteral("Castellano 5º")) {
                    perfil.materias.removeAt(i);
                    break;
                }
            }
            QString error;
            QVERIFY(!validarPerfil(perfil, &error));
            QVERIFY2(error.contains(QStringLiteral("57")), qPrintable(error));
        }
    }

    void cargaMateriaYDemanda()
    {
        const Perfil& perfil = perfilPorDefecto();
        QCOMPARE(cargaMateria(perfil, QStringLiteral("Castellano 1º")), 12);
        QCOMPARE(cargaMateria(perfil, QStringLiteral("Matemáticas 3º")), 12);
        QCOMPARE(cargaMateria(perfil, QStringLiteral("Ciencias de la Tierra 5º")), 4);
        QCOMPARE(cargaMateria(perfil, QStringLiteral("Matemática Aplicada")), 0);  // nueva: no añade demanda
        QCOMPARE(demandaTotal(perfil), 520);

        // Σ cargaMateria del catálogo = 520 y las cargas por materia van de 0 a 18.
        int suma = 0;
        int minimo = 9999;
        int maximo = 0;
        for (const MateriaAnio& materia : perfil.materias) {
            const int carga = cargaMateria(perfil, materia.nombre);
            suma += carga;
            minimo = qMin(minimo, carga);
            maximo = qMax(maximo, carga);
        }
        QCOMPARE(suma, 520);
        QCOMPARE(minimo, 0);
        QCOMPARE(maximo, 18);
    }

    void variantesValidasCubrenAmbosTurnos()
    {
        const Perfil& perfil = perfilPorDefecto();
        QString error;
        QVERIFY2(coberturaDeVariante(perfil, VarianteDocentes::Holgada, &error), qPrintable(error));
        QVERIFY2(coberturaDeVariante(perfil, VarianteDocentes::Suplentes, &error), qPrintable(error));
    }

    void varianteSinProfesoradoNoCubre()
    {
        const Perfil& perfil = perfilPorDefecto();
        QString error;
        QVERIFY(!coberturaDeVariante(perfil, VarianteDocentes::SinProfesorado, &error));
        QVERIFY2(error.contains(QStringLiteral("Física 3º")), qPrintable(error));
    }

    void varianteSinDisponibilidadNoCubre()
    {
        const Perfil& perfil = perfilPorDefecto();
        QString error;
        QVERIFY(!coberturaDeVariante(perfil, VarianteDocentes::SinDisponibilidad, &error));
        QVERIFY2(error.contains(QStringLiteral("Física 3º")), qPrintable(error));
    }

    void recuentosEsperados()
    {
        const Perfil& perfil = perfilPorDefecto();

        const QVector<DocenteDef> holgada = docentesDeVariante(perfil, VarianteDocentes::Holgada);
        QCOMPARE(holgada.size(), 65);
        QCOMPARE(asignacionesDe(holgada), 65);
        QCOMPARE(disponibilidadesDe(holgada), 325);

        const QVector<DocenteDef> suplentes = docentesDeVariante(perfil, VarianteDocentes::Suplentes);
        QCOMPARE(suplentes.size(), 43);
        QCOMPARE(asignacionesDe(suplentes), 57);
        QCOMPARE(disponibilidadesDe(suplentes), 215);

        const QVector<DocenteDef> sinProfesorado =
            docentesDeVariante(perfil, VarianteDocentes::SinProfesorado);
        QCOMPARE(sinProfesorado.size(), 64);
        QCOMPARE(asignacionesDe(sinProfesorado), 64);
        QCOMPARE(disponibilidadesDe(sinProfesorado), 320);

        const QVector<DocenteDef> sinDisponibilidad =
            docentesDeVariante(perfil, VarianteDocentes::SinDisponibilidad);
        QCOMPARE(sinDisponibilidad.size(), 65);
        QCOMPARE(asignacionesDe(sinDisponibilidad), 65);
        QCOMPARE(disponibilidadesDe(sinDisponibilidad), 325);
    }

    void holgadaCumpleTope()
    {
        const Perfil& perfil = perfilPorDefecto();
        const QVector<DocenteDef> docentes = docentesDeVariante(perfil, VarianteDocentes::Holgada);
        QCOMPARE(docentes.size(), 65);

        int maximo = 0;
        for (const DocenteDef& docente : docentes) {
            const int carga = cargaAsignada(perfil, docente.materias);
            QVERIFY2(carga * 10 <= 6 * docente.horasAula, qPrintable(docente.id));
            maximo = qMax(maximo, carga);
        }
        QCOMPARE(maximo, 18);   // «GHC 1.º/2.º» (6 h × 3 secciones) es el 60 % exacto del tope 30.
        QVERIFY(docentesCumplenCarga(perfil, VarianteDocentes::Holgada, nullptr));
    }

    void suplentesCumplenTopeYCapacidad()
    {
        const Perfil& perfil = perfilPorDefecto();
        const QVector<DocenteDef> docentes = docentesDeVariante(perfil, VarianteDocentes::Suplentes);
        QCOMPARE(docentes.size(), 43);

        int titulares = 0;
        int suplentes = 0;
        int sumaTopes = 0;
        for (const DocenteDef& docente : docentes) {
            if (docente.materias.isEmpty()) {
                ++suplentes;
                continue;
            }
            ++titulares;
            const int carga = cargaAsignada(perfil, docente.materias);
            QVERIFY2(carga * 10 >= 8 * docente.horasAula, qPrintable(docente.id));
            QVERIFY2(carga >= docente.horasAula, qPrintable(docente.id));
            QVERIFY2(carga * 100 <= 115 * docente.horasAula, qPrintable(docente.id));
            sumaTopes += docente.horasAula;
        }
        QCOMPARE(titulares, 19);
        QCOMPARE(suplentes, 24);
        QCOMPARE(sumaTopes, 461);

        const int demanda = demandaTotal(perfil);
        QVERIFY2(sumaTopes * 100 >= 85 * demanda, "Σ topes por debajo del 85 % de la demanda");
        QVERIFY2(sumaTopes * 100 <= 90 * demanda, "Σ topes por encima del 90 % de la demanda");
        QVERIFY(docentesCumplenCarga(perfil, VarianteDocentes::Suplentes, nullptr));
    }

    void varianteNombresYTextos()
    {
        QCOMPARE(nombreVariante(VarianteDocentes::Holgada), QString("holgada"));
        QCOMPARE(nombreVariante(VarianteDocentes::Suplentes), QString("suplentes"));
        QCOMPARE(nombreVariante(VarianteDocentes::SinProfesorado), QString("sin profesorado"));
        QCOMPARE(nombreVariante(VarianteDocentes::SinDisponibilidad),
                 QString("sin disponibilidad"));

        const auto holgada = varianteDesdeTexto(QStringLiteral("holgada"));
        QVERIFY(holgada.has_value() && *holgada == VarianteDocentes::Holgada);
        const auto suplentes = varianteDesdeTexto(QStringLiteral("SUPLENTES"));
        QVERIFY(suplentes.has_value() && *suplentes == VarianteDocentes::Suplentes);
        const auto sinProfesorado = varianteDesdeTexto(QStringLiteral("sin-profesorado"));
        QVERIFY(sinProfesorado.has_value() && *sinProfesorado == VarianteDocentes::SinProfesorado);
        const auto sinDisponibilidad = varianteDesdeTexto(QStringLiteral("sin-disponibilidad"));
        QVERIFY(sinDisponibilidad.has_value()
                && *sinDisponibilidad == VarianteDocentes::SinDisponibilidad);
        QVERIFY(!varianteDesdeTexto(QStringLiteral("inexistente")).has_value());
    }
};

QTEST_MAIN(TestSeedPerfil)
#include "test_seed_perfil.moc"
