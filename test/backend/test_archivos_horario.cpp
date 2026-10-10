#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <backend/services/ServicioHorarioSalida.hpp>

/// Tests del guardado/carga de un horario generado como archivo JSON (RF-4).
class TestArchivosHorario : public QObject
{
    Q_OBJECT

private:
    HorarioSalida horarioEjemplo() const
    {
        HorarioSalida h;
        h.metadata.fecha_generacion = QStringLiteral("2026-10-10T12:00:00");
        h.metadata.total_asignaciones = 2;

        CursoOutput curso;
        curso.turno = QStringLiteral("manana");
        DiaOutput dia;
        dia.dia = 0;
        dia.asignaciones = {AsignacionOutput{0, 0, 0, 0}, AsignacionOutput{1, 0, 0, 0}};
        curso.dias = {dia};
        h.horarios[QStringLiteral("1ro A")] = curso;
        return h;
    }

private slots:

    void guardarYcargar_roundtrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ServicioHorarioSalida servicio(dir.path());

        const HorarioSalida h = horarioEjemplo();
        const auto guardado = servicio.guardarHorario(h, QStringLiteral("horario.json"));
        QVERIFY2(guardado.ok, qPrintable(guardado.mensajeError));
        QVERIFY(QFile::exists(dir.filePath("horario.json")));

        const auto leido = servicio.cargarHorario(QStringLiteral("horario.json"));
        QVERIFY2(leido.ok, qPrintable(leido.mensajeError));

        // El horario guardado se recupera idéntico.
        QCOMPARE(leido.valor.toJson(), h.toJson());
    }

    void guardarEnRutaInvalida_errorSinPerderContenido()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ServicioHorarioSalida servicio(dir.path());

        const HorarioSalida h = horarioEjemplo();

        // Subcarpeta inexistente: la escritura falla y se informa.
        const auto fallo = servicio.guardarHorario(h, QStringLiteral("sub/no_existe.json"));
        QVERIFY(!fallo.ok);
        QVERIFY(!fallo.mensajeError.isEmpty());

        // El contenido en memoria no se pierde: se puede guardar en una ruta válida.
        const auto guardado = servicio.guardarHorario(h, QStringLiteral("ok.json"));
        QVERIFY2(guardado.ok, qPrintable(guardado.mensajeError));

        const auto leido = servicio.cargarHorario(QStringLiteral("ok.json"));
        QVERIFY(leido.ok);
        QCOMPARE(leido.valor.toJson(), h.toJson());
    }

    void cargarArchivoInexistente_error()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ServicioHorarioSalida servicio(dir.path());

        const auto leido = servicio.cargarHorario(QStringLiteral("no_existe.json"));
        QVERIFY(!leido.ok);
        QVERIFY(!leido.mensajeError.isEmpty());
    }

    void cargarArchivoCorrupto_error()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ServicioHorarioSalida servicio(dir.path());

        {
            QFile archivo(dir.filePath("corrupto.json"));
            QVERIFY(archivo.open(QIODevice::WriteOnly));
            archivo.write("{ esto no es JSON");
        }

        const auto leido = servicio.cargarHorario(QStringLiteral("corrupto.json"));
        QVERIFY(!leido.ok);
        QVERIFY(!leido.mensajeError.isEmpty());
    }
};

QTEST_MAIN(TestArchivosHorario)
#include "test_archivos_horario.moc"
