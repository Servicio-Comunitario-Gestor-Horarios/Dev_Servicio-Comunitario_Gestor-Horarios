#include <QMap>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QVector>

#include <gtest/gtest.h>

#include <memory>

#include <backend/services/NucleoDatos.hpp>
#include <backend/services/ServicioCapacidadHoraria.hpp>
#include <backend/services/ServicioCursos.hpp>
#include <seed/perfil.hpp>
#include <seed/sembrador.hpp>

namespace {

/// Aplicación Qt mínima: el controlador SQLite necesita una instancia de `QCoreApplication`
/// y Google Test no la crea (Qt Test sí lo hace en `QTEST_MAIN`).
QCoreApplication& aplicacionQt()
{
    static int argc = 1;
    static char nombre[] = "test_servicio_capacidad_horaria";
    static char* argv[] = {nombre, nullptr};
    static QCoreApplication aplicacion(argc, argv);
    return aplicacion;
}

/// Avisos del resumen unidos en un texto para localizar uno concreto.
QString avisosComoTexto(const ResumenCapacidadHorariaDTO& resumen)
{
    return resumen.avisos.join(QStringLiteral(" | "));
}

/// Mapa curso→plan del perfil del centro, con la clave de presentación de cada curso.
QMap<QString, QString> planesDelPerfil()
{
    QMap<QString, QString> planes;
    for (const seed::CursoDef& curso : seed::perfilPorDefecto().cursos) {
        planes.insert(nombreCurso(curso.anio, curso.seccion), curso.plan);
    }
    return planes;
}

/// Σ de horas del detalle para un plan y un año.
int horasDePlanYAnio(const QVector<HorasPlanAnioMateriaDTO>& detalle, const QString& codigoPlan,
                     int anio)
{
    int total = 0;
    for (const HorasPlanAnioMateriaDTO& fila : detalle) {
        if (fila.codigoPlan == codigoPlan && fila.anio == anio) {
            total += fila.horas;
        }
    }
    return total;
}

/// Nombre del plan `codigo` dentro del detalle (vacío si no aparece).
QString nombreDePlan(const QVector<HorasPlanAnioMateriaDTO>& detalle, const QString& codigoPlan)
{
    for (const HorasPlanAnioMateriaDTO& fila : detalle) {
        if (fila.codigoPlan == codigoPlan) {
            return fila.nombrePlan;
        }
    }
    return QString();
}

} // namespace

/// Servicio de capacidad horaria sobre la base sembrada del centro: horas disponibles entre
/// docentes, horas necesarias para cubrir los cursos y detalle de horas por plan/año/materia.
class ServicioCapacidadHorariaTest : public ::testing::Test
{
protected:
    std::unique_ptr<QTemporaryDir> m_dir;
    QSqlDatabase m_db;
    QString m_conexion;
    std::unique_ptr<ServicioCapacidadHoraria> m_servicio;
    std::unique_ptr<NucleoDatos> m_nucleo;

    void SetUp() override
    {
        aplicacionQt();
        m_dir.reset(new QTemporaryDir());
        ASSERT_TRUE(m_dir->isValid());

        // Base sembrada con el conjunto del centro: 33 docentes, 24 cursos, 2 planes y 95 cargas.
        const QString ruta = m_dir->filePath(QStringLiteral("centro.db"));
        seed::OpcionesAplicacion siembra;
        siembra.rutaDb = ruta;
        const seed::ResultadoAplicacion sembrado = seed::aplicar(siembra);
        ASSERT_TRUE(sembrado.ok()) << sembrado.detalle.toStdString();

        m_conexion = QStringLiteral("test_servicio_capacidad_horaria");
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_conexion);
        m_db.setDatabaseName(ruta);
        ASSERT_TRUE(m_db.open()) << m_db.lastError().text().toStdString();

        m_servicio.reset(new ServicioCapacidadHoraria(m_db));
        m_nucleo.reset(new NucleoDatos(m_db));
    }

    void TearDown() override
    {
        m_nucleo.reset();
        m_servicio.reset();
        if (m_db.isOpen()) {
            m_db.close();
        }
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_conexion);
        m_dir.reset();
    }
};

// ─── Detalle por plan, año y materia ─────────────────────────────────────────

TEST_F(ServicioCapacidadHorariaTest, DetalleContieneTodasLasCargasDeLosDosPlanes)
{
    const auto resultado = m_servicio->horasPorPlanAnioMateria();
    ASSERT_TRUE(resultado.ok) << resultado.mensajeError.toStdString();

    const QVector<HorasPlanAnioMateriaDTO>& detalle = resultado.valor;
    EXPECT_EQ(detalle.size(), 95);

    EXPECT_EQ(nombreDePlan(detalle, QStringLiteral("31059")), QStringLiteral("Media General"));
    EXPECT_EQ(nombreDePlan(detalle, QStringLiteral("31060")),
              QStringLiteral("Ciencias y Tecnología"));

    // 31059: 34/34/34/36/34 horas por año; 31060: 40 horas cada año.
    EXPECT_EQ(horasDePlanYAnio(detalle, QStringLiteral("31059"), 1), 34);
    EXPECT_EQ(horasDePlanYAnio(detalle, QStringLiteral("31059"), 2), 34);
    EXPECT_EQ(horasDePlanYAnio(detalle, QStringLiteral("31059"), 3), 34);
    EXPECT_EQ(horasDePlanYAnio(detalle, QStringLiteral("31059"), 4), 36);
    EXPECT_EQ(horasDePlanYAnio(detalle, QStringLiteral("31059"), 5), 34);
    for (int anio = 1; anio <= 5; ++anio) {
        EXPECT_EQ(horasDePlanYAnio(detalle, QStringLiteral("31060"), anio), 40) << anio;
    }

    // Orden: plan, año y materia; cada fila trae materia, id y horas.
    for (int i = 1; i < detalle.size(); ++i) {
        const HorasPlanAnioMateriaDTO& anterior = detalle.at(i - 1);
        const HorasPlanAnioMateriaDTO& actual = detalle.at(i);
        const bool ordenado =
            anterior.codigoPlan < actual.codigoPlan
            || (anterior.codigoPlan == actual.codigoPlan
                && (anterior.anio < actual.anio
                    || (anterior.anio == actual.anio
                        && anterior.nombreMateria <= actual.nombreMateria)));
        EXPECT_TRUE(ordenado) << anterior.codigoPlan.toStdString() << " y "
                              << actual.codigoPlan.toStdString();
        EXPECT_GT(actual.idMateria, 0);
        EXPECT_FALSE(actual.nombreMateria.isEmpty());
        EXPECT_GT(actual.horas, 0);
    }
}

// ─── Resumen sin mapas del frontend ──────────────────────────────────────────

TEST_F(ServicioCapacidadHorariaTest, ResumenSinMapasUsaTreintaYSeisHorasYNoCalculaNecesarias)
{
    const auto resultado = m_servicio->resumen();
    ASSERT_TRUE(resultado.ok) << resultado.mensajeError.toStdString();

    const ResumenCapacidadHorariaDTO& resumen = resultado.valor;
    EXPECT_EQ(resumen.docentes, 33);
    EXPECT_EQ(resumen.cursos, 24);
    EXPECT_EQ(resumen.horasDisponibles, 1188); // 33 × 36
    EXPECT_EQ(resumen.horasNecesarias, 0);
    EXPECT_EQ(resumen.detalle.size(), 95);

    const QString avisos = avisosComoTexto(resumen);
    EXPECT_TRUE(avisos.contains(QStringLiteral("36"))) << avisos.toStdString();
    EXPECT_TRUE(avisos.contains(QStringLiteral("plan"))) << avisos.toStdString();
}

// ─── Resumen con los planes por curso del frontend ───────────────────────────

TEST_F(ServicioCapacidadHorariaTest, ResumenConPlanesDelPerfilCalculaLasHorasNecesarias)
{
    const auto resultado = m_servicio->resumen({}, planesDelPerfil());
    ASSERT_TRUE(resultado.ok) << resultado.mensajeError.toStdString();

    const ResumenCapacidadHorariaDTO& resumen = resultado.valor;
    EXPECT_EQ(resumen.cursos, 24);
    EXPECT_EQ(resumen.horasNecesarias, 876);
    EXPECT_EQ(resumen.horasDisponibles, 1188);
    EXPECT_FALSE(avisosComoTexto(resumen).contains(QStringLiteral("No se recibieron planes")));
}

// ─── Resumen con las horas de aula por docente del frontend ──────────────────

TEST_F(ServicioCapacidadHorariaTest, ResumenConHorasAulaPorDocenteSumaElMapa)
{
    const auto docentes = m_nucleo->listarDocentes();
    ASSERT_TRUE(docentes.ok) << docentes.mensajeError.toStdString();
    ASSERT_EQ(docentes.valor.size(), 33);

    QMap<QString, int> horasAula;
    for (const ProfesorDTO& docente : docentes.valor) {
        horasAula.insert(docente.nombre, 30);
    }

    const auto resultado = m_servicio->resumen(horasAula, planesDelPerfil());
    ASSERT_TRUE(resultado.ok) << resultado.mensajeError.toStdString();
    EXPECT_EQ(resultado.valor.horasDisponibles, 990); // 33 × 30
    EXPECT_EQ(resultado.valor.horasNecesarias, 876);
    EXPECT_FALSE(avisosComoTexto(resultado.valor).contains(QStringLiteral("36")));
}

TEST_F(ServicioCapacidadHorariaTest, ResumenConDocenteInexistenteAvisaYLoIgnora)
{
    QMap<QString, int> horasAula;
    horasAula.insert(QStringLiteral("Docente Que No Existe"), 20);

    const auto resultado = m_servicio->resumen(horasAula, planesDelPerfil());
    ASSERT_TRUE(resultado.ok) << resultado.mensajeError.toStdString();
    EXPECT_EQ(resultado.valor.horasDisponibles, 0);
    EXPECT_TRUE(avisosComoTexto(resultado.valor)
                    .contains(QStringLiteral("Docente Que No Existe")))
        << avisosComoTexto(resultado.valor).toStdString();
}

// ─── Datos incompletos: aviso, no error ─────────────────────────────────────

TEST_F(ServicioCapacidadHorariaTest, ResumenCursoSinPlanYPlanInexistenteAvisaYOmite)
{
    QMap<QString, QString> planes = planesDelPerfil();
    ASSERT_GT(planes.remove(QStringLiteral("1ro A")), 0);
    planes.insert(QStringLiteral("1ro B"), QStringLiteral("99999"));

    const auto resultado = m_servicio->resumen({}, planes);
    ASSERT_TRUE(resultado.ok) << resultado.mensajeError.toStdString();

    // Se omiten las 34 horas de «1ro A» (sin plan) y las 34 de «1ro B» (plan inexistente).
    EXPECT_EQ(resultado.valor.horasNecesarias, 876 - 34 - 34);

    const QString avisos = avisosComoTexto(resultado.valor);
    EXPECT_TRUE(avisos.contains(QStringLiteral("1ro A"))) << avisos.toStdString();
    EXPECT_TRUE(avisos.contains(QStringLiteral("1ro B"))) << avisos.toStdString();
    EXPECT_TRUE(avisos.contains(QStringLiteral("99999"))) << avisos.toStdString();
}

// ─── Fachada NucleoDatos ────────────────────────────────────────────────────

TEST_F(ServicioCapacidadHorariaTest, NucleoExponeElDetalleYElResumen)
{
    const auto detalle = m_nucleo->horasPorPlanAnioMateria();
    ASSERT_TRUE(detalle.ok) << detalle.mensajeError.toStdString();
    EXPECT_EQ(detalle.valor.size(), 95);

    const auto resumen = m_nucleo->resumenCapacidadHoraria({}, planesDelPerfil());
    ASSERT_TRUE(resumen.ok) << resumen.mensajeError.toStdString();
    EXPECT_EQ(resumen.valor.docentes, 33);
    EXPECT_EQ(resumen.valor.horasNecesarias, 876);
    EXPECT_EQ(resumen.valor.horasDisponibles, 1188);
}
