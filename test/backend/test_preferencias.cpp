#include <gtest/gtest.h>

#include <QString>
#include <QTime>

#include "backend/data/franja_horaria.hpp"
#include "backend/services/franja_horaria_validacion.hpp"
#include "backend/services/preferencias.hpp"

/// Pruebas del modelo puro de preferencias y de la validación compartida de franjas
/// (RF-1, RF-2, RNF-1): día 0–6, rango con inicio anterior al fin, solape y sentido.
/// Sin base de datos.
namespace {

/// Franja de conveniencia: día, hora de inicio y hora de fin.
FranjaHoraria franja(int dia, int hIni, int mIni, int hFin, int mFin)
{
    FranjaHoraria f;
    f.dia = dia;
    f.inicio = QTime(hIni, mIni);
    f.fin = QTime(hFin, mFin);
    return f;
}

} // namespace

// ─── errorDeFranja ──────────────────────────────────────────────────────────

TEST(TestPreferencias, errorDeFranja_aceptaLosExtremosDelDia)
{
    EXPECT_TRUE(franja_horaria::errorDeFranja(franja(0, 8, 0, 10, 0)).isEmpty());
    EXPECT_TRUE(franja_horaria::errorDeFranja(franja(6, 8, 0, 10, 0)).isEmpty());
    EXPECT_TRUE(franja_horaria::errorDeFranja(franja(3, 8, 0, 10, 0)).isEmpty());
}

TEST(TestPreferencias, errorDeFranja_rechazaDiaFueraDeRangoConValorYAccion)
{
    for (int dia : {-1, 7}) {
        const QString error = franja_horaria::errorDeFranja(franja(dia, 8, 0, 10, 0));

        EXPECT_FALSE(error.isEmpty());
        EXPECT_NE(error.indexOf(QStringLiteral("0")), -1) << error.toStdString();
        EXPECT_NE(error.indexOf(QStringLiteral("6")), -1) << error.toStdString();
        EXPECT_NE(error.indexOf(QString::number(dia)), -1) << error.toStdString();
        EXPECT_NE(error.indexOf(QStringLiteral("Corrija el día")), -1) << error.toStdString();
    }
}

TEST(TestPreferencias, errorDeFranja_rechazaHorasInvalidas)
{
    FranjaHoraria sinInicio = franja(1, 8, 0, 10, 0);
    sinInicio.inicio = QTime();
    FranjaHoraria sinFin = franja(1, 8, 0, 10, 0);
    sinFin.fin = QTime();

    for (const FranjaHoraria& f : {sinInicio, sinFin}) {
        const QString error = franja_horaria::errorDeFranja(f);

        EXPECT_FALSE(error.isEmpty());
        EXPECT_NE(error.indexOf(QStringLiteral("HH:mm")), -1) << error.toStdString();
        EXPECT_NE(error.indexOf(QStringLiteral("Corrija el rango")), -1) << error.toStdString();
    }
}

TEST(TestPreferencias, errorDeFranja_rechazaInicioNoAnteriorAlFin)
{
    // Iguales e invertidas: el inicio debe ser estrictamente anterior al fin.
    EXPECT_FALSE(franja_horaria::errorDeFranja(franja(1, 10, 0, 10, 0)).isEmpty());
    EXPECT_FALSE(franja_horaria::errorDeFranja(franja(1, 12, 0, 8, 0)).isEmpty());

    const QString error = franja_horaria::errorDeFranja(franja(1, 12, 0, 8, 0));
    EXPECT_NE(error.indexOf(QStringLiteral("anterior")), -1) << error.toStdString();
    EXPECT_NE(error.indexOf(QStringLiteral("Corrija el rango")), -1) << error.toStdString();
}

// ─── haySolape ──────────────────────────────────────────────────────────────

TEST(TestPreferencias, haySolape_solapados_verdadero)
{
    EXPECT_TRUE(haySolape(franja(1, 8, 0, 10, 0), franja(1, 9, 0, 11, 0)));
    EXPECT_TRUE(haySolape(franja(1, 9, 0, 11, 0), franja(1, 8, 0, 10, 0)));
    // Contención completa: también se solapan.
    EXPECT_TRUE(haySolape(franja(1, 8, 0, 12, 0), franja(1, 9, 0, 10, 0)));
}

TEST(TestPreferencias, haySolape_contiguos_falso)
{
    // El fin de uno es el inicio del otro: no se solapan (RF-2).
    EXPECT_FALSE(haySolape(franja(1, 8, 0, 10, 0), franja(1, 10, 0, 12, 0)));
    EXPECT_FALSE(haySolape(franja(1, 10, 0, 12, 0), franja(1, 8, 0, 10, 0)));
}

// ─── sentido ────────────────────────────────────────────────────────────────

TEST(TestPreferencias, textoDeSentido_formaPersistida)
{
    EXPECT_EQ(textoDeSentido(SentidoPreferencia::Preferida), QStringLiteral("preferida"));
    EXPECT_EQ(textoDeSentido(SentidoPreferencia::Evitada), QStringLiteral("evitada"));
}

TEST(TestPreferencias, sentidoDesdeTexto_leeLaFormaPersistida)
{
    SentidoPreferencia sentido = SentidoPreferencia::Preferida;

    ASSERT_TRUE(sentidoDesdeTexto(QStringLiteral("evitada"), &sentido));
    EXPECT_EQ(sentido, SentidoPreferencia::Evitada);

    ASSERT_TRUE(sentidoDesdeTexto(QStringLiteral("preferida"), &sentido));
    EXPECT_EQ(sentido, SentidoPreferencia::Preferida);

    // Un texto desconocido no es un sentido y no toca el valor de salida.
    EXPECT_FALSE(sentidoDesdeTexto(QStringLiteral("neutra"), &sentido));
    EXPECT_EQ(sentido, SentidoPreferencia::Preferida);
    EXPECT_FALSE(sentidoDesdeTexto(QString(), &sentido));
}

TEST(TestPreferencias, sentidoDesdeTexto_idaYVuelta)
{
    for (SentidoPreferencia original :
         {SentidoPreferencia::Preferida, SentidoPreferencia::Evitada}) {
        SentidoPreferencia leido = SentidoPreferencia::Preferida;
        ASSERT_TRUE(sentidoDesdeTexto(textoDeSentido(original), &leido));
        EXPECT_EQ(leido, original);
    }
}

TEST(TestPreferencias, sentidoValido_rechazaValorCasteado)
{
    EXPECT_TRUE(sentidoValido(SentidoPreferencia::Preferida));
    EXPECT_TRUE(sentidoValido(SentidoPreferencia::Evitada));
    EXPECT_FALSE(sentidoValido(static_cast<SentidoPreferencia>(99)));
}
