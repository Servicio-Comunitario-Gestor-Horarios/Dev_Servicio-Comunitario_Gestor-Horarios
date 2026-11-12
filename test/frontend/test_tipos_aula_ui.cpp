#include <QTest>
#include <QVector>

#include "services/tipos_aula_ui.hpp"

/// Pruebas de la ayuda estricta que resuelve el tipo general (016 RF-2): devuelve el
/// general o un fallo, nunca otro tipo del catálogo.
class TestTiposAulaUI : public QObject
{
    Q_OBJECT

private:
    static TipoAulaDTO tipo(int id, const QString& nombre)
    {
        TipoAulaDTO dto;
        dto.id = id;
        dto.nombre = nombre;
        return dto;
    }

private slots:

    void encuentraElGeneral()
    {
        const QVector<TipoAulaDTO> tipos = {
            tipo(1, QStringLiteral("Laboratorio")),
            tipo(2, QStringLiteral("Aula de estudio")),
            tipo(3, QStringLiteral("Taller")),
        };

        const auto resultado = TiposAulaUI::idTipoGeneral(tipos);

        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
        QCOMPARE(resultado.valor, 2);
    }

    void sinElGeneral_fallaYNoDevuelveElPrimero()
    {
        const QVector<TipoAulaDTO> tipos = {
            tipo(1, QStringLiteral("Laboratorio")),
            tipo(2, QStringLiteral("Taller")),
        };

        const auto resultado = TiposAulaUI::idTipoGeneral(tipos);

        QVERIFY(!resultado.ok);
        // En error no se devuelve ningún id del catálogo (no hay respaldo silencioso).
        for (const TipoAulaDTO& candidato : tipos)
            QVERIFY2(resultado.valor != candidato.id,
                     "No debe devolver un tipo del catálogo");
        QVERIFY(!resultado.mensajeError.isEmpty());
    }

    void catalogoVacio_falla()
    {
        const auto resultado = TiposAulaUI::idTipoGeneral({});

        QVERIFY(!resultado.ok);
        QVERIFY(!resultado.mensajeError.isEmpty());
    }

    void aceptaMayusculasYEspacios()
    {
        const QVector<TipoAulaDTO> tipos = {
            tipo(5, QStringLiteral("Laboratorio")),
            tipo(7, QStringLiteral("  AULA DE ESTUDIO ")),
        };

        const auto resultado = TiposAulaUI::idTipoGeneral(tipos);

        QVERIFY2(resultado.ok, qPrintable(resultado.mensajeError));
        QCOMPARE(resultado.valor, 7);
    }

    void mensaje_nombraElTipoYLaAccion()
    {
        const auto resultado = TiposAulaUI::idTipoGeneral(
            {tipo(1, QStringLiteral("Laboratorio")), tipo(2, QStringLiteral("Taller"))});

        QVERIFY(!resultado.ok);
        const QString& mensaje = resultado.mensajeError;
        QVERIFY2(mensaje.contains(TiposAulaUI::nombreTipoGeneral()), qPrintable(mensaje));
        QVERIFY2(mensaje.contains(QStringLiteral("respaldo")), qPrintable(mensaje));
        QVERIFY2(mensaje.contains(QStringLiteral("base nueva")), qPrintable(mensaje));
        QVERIFY2(!mensaje.contains(QStringLiteral("QSQLITE"), Qt::CaseInsensitive),
                 qPrintable(mensaje));
        QVERIFY2(!mensaje.contains(QStringLiteral("sqlite")), qPrintable(mensaje));
    }
};

QTEST_MAIN(TestTiposAulaUI)
#include "test_tipos_aula_ui.moc"
