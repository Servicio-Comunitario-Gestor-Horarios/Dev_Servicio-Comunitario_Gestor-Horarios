#include <QTest>
#include <QString>

#include <backend/services/ServicioCursos.hpp>
#include <backend/resultado.hpp>

/// Tests de las funciones puras de identidad, nombre y orden de curso (RF-1..RF-4).
///
/// Corresponden a la tarea T1 de `specs/004-persistencia-campos-curso/tasks.md`.
class TestCamposCurso : public QObject
{
    Q_OBJECT

private slots:

    // ─── nombreCurso ────────────────────────────────────────────────────────

    void nombreCurso_ordinales()
    {
        QCOMPARE(nombreCurso(1, QStringLiteral("A")), QStringLiteral("1ro A"));
        QCOMPARE(nombreCurso(2, QStringLiteral("B")), QStringLiteral("2do B"));
        QCOMPARE(nombreCurso(3, QStringLiteral("C")), QStringLiteral("3ro C"));
        QCOMPARE(nombreCurso(4, QStringLiteral("A")), QStringLiteral("4to A"));
        QCOMPARE(nombreCurso(5, QStringLiteral("A")), QStringLiteral("5to A"));
        QCOMPARE(nombreCurso(6, QStringLiteral("A")), QStringLiteral("6to A"));
    }

    void nombreCurso_anioDeDosDigitos()
    {
        QCOMPARE(nombreCurso(10, QStringLiteral("B")), QStringLiteral("10to B"));
    }

    void nombreCurso_conServicioNoAscii()
    {
        QCOMPARE(nombreCurso(4, QString::fromUtf8("\xc3\x91")), QString::fromUtf8("4to \xc3\x91"));
    }

    // ─── normalizarSeccion ──────────────────────────────────────────────────

    void normalizarSeccion_recortaYMayusculas()
    {
        QCOMPARE(normalizarSeccion(QStringLiteral(" a ")), QStringLiteral("A"));
        QCOMPARE(normalizarSeccion(QStringLiteral("a")),   QStringLiteral("A"));
    }

    void normalizarSeccion_noAscii()
    {
        QCOMPARE(normalizarSeccion(QString::fromUtf8("\xc3\xb1")), QString::fromUtf8("\xc3\x91")); // ñ -> Ñ
        QCOMPARE(normalizarSeccion(QString::fromUtf8("\xc3\xa1")), QString::fromUtf8("\xc3\x81")); // á -> Á
    }

    // ─── validarIdentidadCurso ──────────────────────────────────────────────

    void validarIdentidad_anioCero_rechaza()
    {
        const auto r = validarIdentidadCurso(0, QStringLiteral("A"));
        QVERIFY(!r.ok);
        QVERIFY(!r.mensajeError.isEmpty());
        QVERIFY(r.mensajeError.contains(QString::fromUtf8("a\xc3\xb1o"))); // menciona el campo
        QVERIFY(r.mensajeError.contains(QStringLiteral("0")));         // y el valor
    }

    void validarIdentidad_anioNegativo_rechaza()
    {
        const auto r = validarIdentidadCurso(-1, QStringLiteral("A"));
        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains(QStringLiteral("-1")));
    }

    void validarIdentidad_seccionVacia_rechaza()
    {
        const auto r = validarIdentidadCurso(1, QString());
        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains(QString::fromUtf8("secci\xc3\xb3n")));
    }

    void validarIdentidad_seccionSoloEspacios_rechaza()
    {
        const auto r = validarIdentidadCurso(1, QStringLiteral("   "));
        QVERIFY(!r.ok);
    }

    void validarIdentidad_seccionDosCaracteres_rechaza()
    {
        const auto r = validarIdentidadCurso(1, QStringLiteral("AB"));
        QVERIFY(!r.ok);
        QVERIFY(r.mensajeError.contains(QStringLiteral("AB"))); // incluye el valor recibido
    }

    void validarIdentidad_seccionNoAscii_acepta()
    {
        const auto r = validarIdentidadCurso(1, QString::fromUtf8("\xc3\xb1")); // ñ
        QVERIFY(r.ok);
        QCOMPARE(r.valor.anio, 1);
        QCOMPARE(r.valor.seccion, QString::fromUtf8("\xc3\x91")); // Ñ normalizada
    }

    void validarIdentidad_normalizaYDevuelve()
    {
        const auto r = validarIdentidadCurso(3, QStringLiteral(" a "));
        QVERIFY(r.ok);
        QCOMPARE(r.valor.anio,    3);
        QCOMPARE(r.valor.seccion, QStringLiteral("A"));
    }

    // ─── cursoAntes ─────────────────────────────────────────────────────────

    void cursoAntes_aniosDistintos_ordenNumerico()
    {
        QVERIFY(cursoAntes(1, QStringLiteral("A"), 2,  QStringLiteral("A")));
        QVERIFY(!cursoAntes(2, QStringLiteral("A"), 1,  QStringLiteral("A")));
        QVERIFY(cursoAntes(2, QStringLiteral("A"), 10, QStringLiteral("A"))); // numérico, no textual
    }

    void cursoAntes_mismoAnio_porSeccion()
    {
        QVERIFY(cursoAntes(1, QStringLiteral("A"), 1, QStringLiteral("B")));
        QVERIFY(!cursoAntes(1, QStringLiteral("B"), 1, QStringLiteral("A")));
    }

    void cursoAntes_mismoAnio_colacionEspanola()
    {
        // La colación española ordena «A» antes que «Á» (no tras «Z»).
        QVERIFY(cursoAntes(1, QStringLiteral("A"), 1, QString::fromUtf8("\xc3\x81")));
    }

    void cursoAntes_mismaIdentidad_noEsAnterior()
    {
        QVERIFY(!cursoAntes(1, QStringLiteral("A"), 1, QStringLiteral("A")));
    }
};

QTEST_MAIN(TestCamposCurso)
#include "test_campos_curso.moc"
