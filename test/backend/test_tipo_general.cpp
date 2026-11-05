#include <gtest/gtest.h>

#include <QString>
#include <QVector>

#include "backend/database/tipo_general.hpp"
#include "common/tipo_general.hpp"

/// Pruebas del predicado único de validez de la fila del tipo general (RF-2/RF-6) y de
/// la traducción pura del rechazo del almacenamiento (P4). Sin base de datos y sin reloj.
namespace {

tipo_general::Fila fila(int id, const QString& nombre, const QString& clave) {
    return tipo_general::Fila{id, nombre, clave};
}

tipo_general::Fila filaGeneral() {
    return fila(1, tipo_general::nombre(), tipo_general::clave());
}

/// Comprueba que el motivo nombra el conteo y el valor esperado.
void exigirMotivoConConteoYValorEsperado(const QString& motivo, const QString& conteo) {
    EXPECT_FALSE(motivo.isEmpty());
    EXPECT_NE(motivo.indexOf(conteo), -1) << motivo.toStdString();
    EXPECT_NE(motivo.indexOf(tipo_general::nombre()), -1) << motivo.toStdString();
    EXPECT_NE(motivo.indexOf(tipo_general::clave()), -1) << motivo.toStdString();
}

} // namespace

// ─── evaluar ────────────────────────────────────────────────────────────────

TEST(TestTipoGeneral, evaluar_sinFilas_esSinTipoGeneral) {
    QString motivo;
    EXPECT_EQ(tipo_general::evaluar({}, &motivo), tipo_general::EstadoFila::SinTipoGeneral);
    exigirMotivoConConteoYValorEsperado(motivo, QStringLiteral("0"));
}

TEST(TestTipoGeneral, evaluar_unaFilaCanonica_esValida) {
    QString motivo;
    EXPECT_EQ(tipo_general::evaluar({filaGeneral()}, &motivo),
              tipo_general::EstadoFila::Valida);
    EXPECT_TRUE(motivo.isEmpty()) << motivo.toStdString();
}

TEST(TestTipoGeneral, evaluar_dosFilasConNombreCanonico_esVariasFilas) {
    // Una fila canónica correcta y otra con el mismo nombre visible normalizado y la
    // clave en mayúsculas: la clave almacenada difiere, así que el índice único no la
    // frena, pero la validez exige exactamente una fila (falla la fila única).
    const QVector<tipo_general::Fila> filas = {
        filaGeneral(),
        fila(2, QStringLiteral("AULA DE ESTUDIO"), QStringLiteral("AULA DE ESTUDIO")),
    };

    QString motivo;
    EXPECT_EQ(tipo_general::evaluar(filas, &motivo),
              tipo_general::EstadoFila::VariasFilas);
    exigirMotivoConConteoYValorEsperado(motivo, QStringLiteral("2"));
}

TEST(TestTipoGeneral, evaluar_unaFilaConClaveDistinta_esClaveDistinta) {
    const QVector<tipo_general::Fila> filas = {
        fila(1, tipo_general::nombre(), QStringLiteral("AULA DE ESTUDIO")),
    };

    QString motivo;
    EXPECT_EQ(tipo_general::evaluar(filas, &motivo),
              tipo_general::EstadoFila::ClaveDistinta);
    exigirMotivoConConteoYValorEsperado(motivo, QStringLiteral("1"));
    // El motivo nombra la clave almacenada que se encontró.
    EXPECT_NE(motivo.indexOf(QStringLiteral("AULA DE ESTUDIO")), -1) << motivo.toStdString();
}

TEST(TestTipoGeneral, evaluar_ignoraElOrdenDeEntrada) {
    const QVector<tipo_general::Fila> ascendente = {
        filaGeneral(),
        fila(2, QStringLiteral("Laboratorio"), QStringLiteral("laboratorio")),
        fila(3, QStringLiteral("Taller"), QStringLiteral("taller")),
    };
    const QVector<tipo_general::Fila> descendente = {
        ascendente[2],
        ascendente[1],
        ascendente[0],
    };

    EXPECT_EQ(tipo_general::evaluar(ascendente), tipo_general::EstadoFila::Valida);
    EXPECT_EQ(tipo_general::evaluar(descendente), tipo_general::EstadoFila::Valida);
}

// ─── traducirRechazo ────────────────────────────────────────────────────────

TEST(TestTipoGeneral, traducirRechazo_sinFallo_devuelveVacio) {
    const tipo_general::EstadoFilaProtegida general{true, true};
    EXPECT_TRUE(tipo_general::traducirRechazo(tipo_general::OperacionProtegida::Borrado,
                                              false, general, QString())
                    .isEmpty());
}

TEST(TestTipoGeneral, traducirRechazo_falloDeFilaInexistente_devuelveVacio) {
    const tipo_general::EstadoFilaProtegida inexistente{false, false};
    EXPECT_TRUE(tipo_general::traducirRechazo(tipo_general::OperacionProtegida::Borrado,
                                              true, inexistente, QString())
                    .isEmpty());
}

TEST(TestTipoGeneral, traducirRechazo_falloDeOtraFila_devuelveVacio) {
    const tipo_general::EstadoFilaProtegida otra{true, false};
    EXPECT_TRUE(tipo_general::traducirRechazo(tipo_general::OperacionProtegida::Renombrado,
                                              true, otra, QStringLiteral("Laboratorio"))
                    .isEmpty());
}

TEST(TestTipoGeneral, traducirRechazo_falloDelGeneral_borrado_devuelveElMensajeDelCatalogo) {
    const tipo_general::EstadoFilaProtegida general{true, true};
    const QString mensaje = tipo_general::traducirRechazo(
        tipo_general::OperacionProtegida::Borrado, true, general, QString());

    EXPECT_FALSE(mensaje.isEmpty());
    EXPECT_EQ(mensaje, tipo_general::mensajeProteccionServicio(
                           tipo_general::OperacionProtegida::Borrado, QString()));
    EXPECT_NE(mensaje.indexOf(QStringLiteral("general")), -1) << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("no admite borrado")), -1)
        << mensaje.toStdString();
}

TEST(TestTipoGeneral, traducirRechazo_falloDelGeneral_renombrado_incluyeElValorRecibido) {
    const tipo_general::EstadoFilaProtegida general{true, true};
    const QString recibido = QStringLiteral("Aula común");
    const QString mensaje = tipo_general::traducirRechazo(
        tipo_general::OperacionProtegida::Renombrado, true, general, recibido);

    EXPECT_FALSE(mensaje.isEmpty());
    EXPECT_NE(mensaje.indexOf(QStringLiteral("general")), -1) << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(QStringLiteral("no admite cambios")), -1)
        << mensaje.toStdString();
    EXPECT_NE(mensaje.indexOf(recibido), -1) << mensaje.toStdString();
}
