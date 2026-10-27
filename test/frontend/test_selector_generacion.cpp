/**
 * @file test_selector_generacion.cpp
 * @brief Pruebas de los controles nuevos de la vista «Generación de horario»:
 *        el desplegable de selección múltiple (años), el botón+dialogo de
 *        materias suplentes, y los ajustes visuales de las tablas (altura de
 *        fila y paleta de los popups de los desplegables).
 */

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QHeaderView>
#include <QPalette>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>

#include <backend/services/NucleoDatos.hpp>
#include <seed/sembrador.hpp>

#include "dialogs/schedule_theme.hpp"
#include "views/schedule_generation_widget.hpp"
#include "widgets/combo_multiple.hpp"
#include "widgets/selector_materias.hpp"

class TestSelectorGeneracion : public QObject
{
    Q_OBJECT

    /// Centro de la fila indicada de la lista del popup (para simular el clic).
    static QPoint centroDeFila(QAbstractItemView* lista, int fila)
    {
        const QRect caja = lista->visualRect(lista->model()->index(fila, 0));
        return caja.isValid() ? caja.center() : QPoint(12, 18 * fila + 10);
    }

private slots:
    // ── Desplegable de selección múltiple (años) ─────────────────────────
    void elComboMultipleMarcaYResume()
    {
        ComboMultiple combo;
        combo.setTextoVacio(QStringLiteral("Todos"));
        combo.agregarOpciones({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")},
                              QVariantList{1, 2, 3});

        QCOMPARE(combo.count(), 3);
        QVERIFY(combo.seleccionados().isEmpty());
        QCOMPARE(combo.resumen(), QStringLiteral("Todos"));

        combo.setSeleccionDatos(QVariantList{1, 3});
        QVariantList esperado13;
        esperado13 << 1 << 3;
        QCOMPARE(combo.datosSeleccionados(), esperado13);
        QCOMPARE(combo.seleccionados(), QStringList({QStringLiteral("1"), QStringLiteral("3")}));
        QCOMPARE(combo.resumen(), QStringLiteral("1, 3"));

        // Un dato inexistente no marca nada (los presets pueden traer años que
        // ya no existen en la base).
        QVariantList inexistente;
        inexistente << 9;
        combo.setSeleccionDatos(inexistente);
        QVERIFY(combo.seleccionados().isEmpty());
        QCOMPARE(combo.resumen(), QStringLiteral("Todos"));

        combo.setSeleccion({QStringLiteral("2")});
        QVariantList soloDos;
        soloDos << 2;
        QCOMPARE(combo.datosSeleccionados(), soloDos);

        // Con más de tres marcas el resumen pasa a contar en lugar de enumerar.
        combo.agregarOpciones({QStringLiteral("4"), QStringLiteral("5")});
        combo.setSeleccion({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
                            QStringLiteral("4")});
        QCOMPARE(combo.resumen(), QStringLiteral("4 seleccionados"));
    }

    void elComboMultipleAlternaVariasSinCerrarElPopup()
    {
        // Se monta el control en una ventana real, como en la vista.
        QWidget contenedor;
        auto* disposicion = new QVBoxLayout(&contenedor);
        auto* combo = new ComboMultiple(&contenedor);
        disposicion->addWidget(combo);
        contenedor.resize(260, 120);
        contenedor.show();

        combo->agregarOpciones({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")});
        QAbstractItemView* lista = combo->view();
        QVERIFY(lista);

        QSignalSpy aviso(combo, &ComboMultiple::seleccionCambiada);

        combo->showPopup();
        QTRY_VERIFY_WITH_TIMEOUT(lista->isVisible(), 2000);

        QTest::mouseClick(lista->viewport(), Qt::LeftButton, Qt::NoModifier,
                          centroDeFila(lista, 0));
        QTest::mouseClick(lista->viewport(), Qt::LeftButton, Qt::NoModifier,
                          centroDeFila(lista, 2));

        // El popup sigue abierto tras marcar y las dos marcas se conservan.
        QVERIFY(lista->isVisible());
        QCOMPARE(combo->seleccionados(),
                 QStringList({QStringLiteral("1"), QStringLiteral("3")}));
        QCOMPARE(aviso.count(), 2);

        // Volver a pulsar desmarca.
        QTest::mouseClick(lista->viewport(), Qt::LeftButton, Qt::NoModifier,
                          centroDeFila(lista, 0));
        QCOMPARE(combo->seleccionados(), QStringList({QStringLiteral("3")}));

        combo->hidePopup();
    }

    // ── Materias suplentes (botón + diálogo) ─────────────────────────────
    void elBotonDeSuplentesResumeLaSeleccion()
    {
        BotonMateriasSuplentes boton;
        boton.setDisponibles({QStringLiteral("Física"), QStringLiteral("Química")});
        QCOMPARE(boton.text(), QStringLiteral("Elegir materias…"));
        QVERIFY(boton.seleccion().isEmpty());

        boton.setSeleccion({QStringLiteral("Física")});
        QCOMPARE(boton.text(), QStringLiteral("Física"));
        QCOMPARE(boton.seleccion(), QStringList({QStringLiteral("Física")}));

        boton.setSeleccion({QStringLiteral("Física"), QStringLiteral("Química")});
        QCOMPARE(boton.text(), QStringLiteral("Física, Química"));

        boton.setSeleccion({QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")});
        QCOMPARE(boton.text(), QStringLiteral("3 materias"));
        QCOMPARE(boton.toolTip(), QStringLiteral("A\nB\nC"));
    }

    void elDialogoDeMateriasPrecargaLaSeleccion()
    {
        SelectorMateriasDialog dialogo({QStringLiteral("Álgebra"), QStringLiteral("Biología")},
                                       {QStringLiteral("Biología")});
        QCOMPARE(dialogo.seleccionadas(), QStringList({QStringLiteral("Biología")}));

        // Un clic marca; el orden devuelto es el de la rejilla, no el de pulsación.
        QAbstractButton* algebra = nullptr;
        for (QAbstractButton* boton : dialogo.findChildren<QAbstractButton*>())
            if (boton->text() == QStringLiteral("Álgebra"))
                algebra = boton;
        QVERIFY(algebra);
        algebra->click();
        QCOMPARE(dialogo.seleccionadas(),
                 QStringList({QStringLiteral("Álgebra"), QStringLiteral("Biología")}));

        // Sin materias disponibles, el diálogo no ofrece nada y no falla.
        SelectorMateriasDialog vacio({}, {});
        QVERIFY(vacio.seleccionadas().isEmpty());
    }

    // ── Ajustes visuales de la vista ─────────────────────────────────────
    void laVistaCentraLosControlesYUsaLaPaletaDelTema()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        seed::OpcionesAplicacion siembra;
        siembra.rutaDb = dir.filePath(QStringLiteral("centro.db"));
        const seed::ResultadoAplicacion sembrado = seed::aplicar(siembra);
        QVERIFY2(sembrado.ok(), qPrintable(sembrado.detalle));

        const QString conexion = QStringLiteral("test_selector_generacion");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conexion);
            db.setDatabaseName(siembra.rutaDb);
            QVERIFY2(db.open(), qPrintable(db.lastError().text()));
            NucleoDatos nucleo(db);

            QApplication::instance()->setProperty("isDarkMode", false);
            ScheduleGenerationWidget vista;
            vista.setNucleoDatos(&nucleo);

            auto* tablaCursos = vista.findChild<QTableWidget*>(QStringLiteral("genTablaCursos"));
            auto* tablaDocentes =
                vista.findChild<QTableWidget*>(QStringLiteral("genTablaDocentes"));
            QVERIFY(tablaCursos);
            QVERIFY(tablaDocentes);
            QVERIFY(tablaCursos->rowCount() > 0);
            QVERIFY(tablaDocentes->rowCount() > 0);

            // Los controles de celda piden 32 px + 8 px de relleno de ítem: con
            // menos de 40 la fila los desborda y se ven descentrados.
            for (QTableWidget* tabla : {tablaCursos, tablaDocentes})
                QVERIFY2(tabla->verticalHeader()->defaultSectionSize() >= 40,
                         "la fila debe dar cabida al control de celda con su relleno");

            // Años: desplegable de selección múltiple con los años existentes.
            auto* comboAnios = qobject_cast<ComboMultiple*>(tablaDocentes->cellWidget(0, 5));
            QVERIFY2(comboAnios, "la columna «Años» debe ser un desplegable");
            QCOMPARE(comboAnios->count(), 5); // el conjunto siembra 5 años
            QCOMPARE(comboAnios->resumen(), QStringLiteral("Todos"));

            // Materias suplentes: botón que abre la rejilla de materias.
            auto* botonSuplentes =
                qobject_cast<BotonMateriasSuplentes*>(tablaDocentes->cellWidget(0, 7));
            QVERIFY2(botonSuplentes, "la columna «Materias suplentes» debe ser un botón");

            // Todos los popups de la vista llevan la paleta del tema (en claro,
            // fondo claro; sin esto el delegado pinta los ítems en negro).
            const auto compruebaPopups = [&vista, tablaDocentes](const ScheduleTheme::Palette& pal) {
                const auto combos = vista.findChildren<QComboBox*>();
                QVERIFY(!combos.isEmpty());
                int revisados = 0;
                for (QComboBox* combo : combos) {
                    QAbstractItemView* lista = combo->view();
                    QVERIFY(lista);
                    QCOMPARE(lista->palette().color(QPalette::Base), QColor(pal.surface));
                    QCOMPARE(lista->palette().color(QPalette::Text), QColor(pal.text));
                    QCOMPARE(lista->palette().color(QPalette::Highlight), QColor(pal.accent));
                    ++revisados;
                }
                QVERIFY(revisados >= tablaDocentes->rowCount());
            };

            compruebaPopups(ScheduleTheme::current());

            // Al cambiar de tema, los popups se vuelven a teñir.
            QApplication::instance()->setProperty("isDarkMode", true);
            vista.actualizarTema(true);
            const ScheduleTheme::Palette oscuro = ScheduleTheme::current();
            QVERIFY(oscuro.dark);
            compruebaPopups(oscuro);
        }
        QSqlDatabase::removeDatabase(conexion);
    }
};

QTEST_MAIN(TestSelectorGeneracion)

#include "test_selector_generacion.moc"
