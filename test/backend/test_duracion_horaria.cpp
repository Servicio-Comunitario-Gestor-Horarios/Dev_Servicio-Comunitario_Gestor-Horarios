#include <gtest/gtest.h>

#include <QString>

#include "backend/services/duracion_horaria.hpp"

/// Pruebas del punto único de la duración de la hora académica (RF-1, RF-3,
/// RNF-4): rango 30–90, conversión horas académicas → minutos y encaje entero
/// del turno. El módulo es puro: sin base de datos y sin reloj.
namespace {

/// Mensaje esperado cuando la duración cae fuera del rango.
void esperarRechazoConRangoYAccion(const QString& mensaje, int duracion) {
    EXPECT_FALSE(mensaje.isEmpty());
    EXPECT_NE(mensaje.indexOf(QStringLiteral("30")), -1) << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("90")), -1) << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QString::number(duracion)), -1) << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("Ajuste la duración a un valor del rango")), -1)
        << mensaje.toStdString();
}

} // namespace

// ─── errorDeRango ───────────────────────────────────────────────────────────

TEST(TestDuracionHoraria, errorDeRango_aceptaElValorPorDefectoYLosExtremos) {
    EXPECT_TRUE(duracion_horaria::errorDeRango(30).isEmpty());
    EXPECT_TRUE(duracion_horaria::errorDeRango(35).isEmpty());
    EXPECT_TRUE(duracion_horaria::errorDeRango(90).isEmpty());
}

TEST(TestDuracionHoraria, errorDeRango_rechazaBajoElMinimoConRangoValorYAccion) {
    esperarRechazoConRangoYAccion(duracion_horaria::errorDeRango(29), 29);
    esperarRechazoConRangoYAccion(duracion_horaria::errorDeRango(5), 5);
}

TEST(TestDuracionHoraria, errorDeRango_rechazaSobreElMaximoConRangoValorYAccion) {
    esperarRechazoConRangoYAccion(duracion_horaria::errorDeRango(91), 91);
}

// ─── minutosDeHorasAcademicas ───────────────────────────────────────────────

TEST(TestDuracionHoraria, minutosDeHorasAcademicas_multiplicaPorLaDuracion) {
    EXPECT_EQ(duracion_horaria::minutosDeHorasAcademicas(40, 35), 1400);
    EXPECT_EQ(duracion_horaria::minutosDeHorasAcademicas(40, 30), 1200);
}

TEST(TestDuracionHoraria, minutosDeHorasAcademicas_sinHorasEsCero) {
    EXPECT_EQ(duracion_horaria::minutosDeHorasAcademicas(0, 35), 0);
}

TEST(TestDuracionHoraria, minutosDeHorasAcademicas_horasNegativasEsCero) {
    EXPECT_EQ(duracion_horaria::minutosDeHorasAcademicas(-1, 35), 0);
}

TEST(TestDuracionHoraria, minutosDeHorasAcademicas_duracionFueraDeRangoEsCero) {
    EXPECT_EQ(duracion_horaria::minutosDeHorasAcademicas(40, 5), 0);
    EXPECT_EQ(duracion_horaria::minutosDeHorasAcademicas(40, 0), 0);
}

// ─── franjasUtilizables ─────────────────────────────────────────────────────

TEST(TestDuracionHoraria, franjasUtilizables_floorDeLaVentanaMenosEventosEntreLaDuracion) {
    // (290 − 10) / 45 = 6,22 → 6; (290 − 10) / 35 = 8; (150 − 10) / 35 = 4.
    EXPECT_EQ(duracion_horaria::franjasUtilizables(290, 10, 45), 6);
    EXPECT_EQ(duracion_horaria::franjasUtilizables(290, 10, 35), 8);
    EXPECT_EQ(duracion_horaria::franjasUtilizables(150, 10, 35), 4);
}

TEST(TestDuracionHoraria, franjasUtilizables_sinFranjaCompletaEsCero) {
    // (40 − 10) / 35 = 0; el turno no admite generación.
    EXPECT_EQ(duracion_horaria::franjasUtilizables(40, 10, 35), 0);
}

TEST(TestDuracionHoraria, franjasUtilizables_ventanaNoPositivaEsCero) {
    EXPECT_EQ(duracion_horaria::franjasUtilizables(0, 10, 35), 0);
    EXPECT_EQ(duracion_horaria::franjasUtilizables(-290, 10, 35), 0);
}

TEST(TestDuracionHoraria, franjasUtilizables_eventosQueSuperanLaVentanaEsCero) {
    EXPECT_EQ(duracion_horaria::franjasUtilizables(10, 20, 35), 0);
}

TEST(TestDuracionHoraria, franjasUtilizables_duracionNoPositivaEsCero) {
    EXPECT_EQ(duracion_horaria::franjasUtilizables(290, 10, 0), 0);
    EXPECT_EQ(duracion_horaria::franjasUtilizables(290, 10, -35), 0);
}

// ─── deficitDeEncaje ────────────────────────────────────────────────────────

TEST(TestDuracionHoraria, deficitDeEncaje_casoDeLaSpecCon45) {
    // 8 × 45 + 10 = 370 > 290 → faltan 80.
    EXPECT_EQ(duracion_horaria::deficitDeEncaje(8, 290, 10, 45), 80);
}

TEST(TestDuracionHoraria, deficitDeEncaje_turnoCortoCon35) {
    // 5 × 35 + 10 = 185 > 150 → faltan 35.
    EXPECT_EQ(duracion_horaria::deficitDeEncaje(5, 150, 10, 35), 35);
}

TEST(TestDuracionHoraria, deficitDeEncaje_siCabeNoHayDeficit) {
    // 8 × 35 + 10 = 290 ≤ 370; el sobrante no se reparte.
    EXPECT_EQ(duracion_horaria::deficitDeEncaje(8, 370, 10, 35), 0);
}

TEST(TestDuracionHoraria, deficitDeEncaje_ventanaNoPositivaEsCero) {
    EXPECT_EQ(duracion_horaria::deficitDeEncaje(5, 0, 10, 35), 0);
}

TEST(TestDuracionHoraria, deficitDeEncaje_duracionNoPositivaEsCero) {
    EXPECT_EQ(duracion_horaria::deficitDeEncaje(5, 150, 0, 0), 0);
}
