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
#include <QPushButton>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTime>
#include <QVBoxLayout>

#include <backend/services/NucleoDatos.hpp>
#include <seed/sembrador.hpp>

#include "dialogs/schedule_theme.hpp"
#include "services/preferencias_docente_ui.hpp"
#include "views/schedule_generation_widget.hpp"
#include "views/teacher_list_widget.hpp"
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

        // La vista lee de QSettings las preferencias de los docentes (turno,
        // plan y año): se aíslan en un espacio propio para no depender de la
        // configuración de la máquina.
        PreferenciasDocenteUI::usarOrganizacion(
            QStringLiteral("LiceoRobertSerraPrueba"),
            QStringLiteral("GestorHorariosPrueba"));
        PreferenciasDocenteUI::limpiar();

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

            // La tabla solo lleva las columnas de configuración del docente.
            QCOMPARE(tablaDocentes->columnCount(), 6);

            // Materias suplentes: ya no forman parte de la tabla de docentes.
            QVERIFY(!tablaDocentes->cellWidget(0, 7));

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

    /// Un alta de docente en la base debe aparecer al entrar de nuevo en la
    /// vista de generación, sin perder los ajustes ya hechos en las filas.
    void sincronizaDocentesNuevosSinPerderLoAjustado()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // Aislar QSettings (la vista precarga turno/plan/año del docente).
        PreferenciasDocenteUI::usarOrganizacion(
            QStringLiteral("LiceoRobertSerraPrueba"),
            QStringLiteral("GestorHorariosPrueba"));
        PreferenciasDocenteUI::limpiar();

        seed::OpcionesAplicacion siembra;
        siembra.rutaDb = dir.filePath(QStringLiteral("centro.db"));
        QVERIFY2(seed::aplicar(siembra).ok(), "no se pudo sembrar el centro");

        const QString conexion = QStringLiteral("test_selector_generacion_sync");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conexion);
            db.setDatabaseName(siembra.rutaDb);
            QVERIFY2(db.open(), qPrintable(db.lastError().text()));
            NucleoDatos nucleo(db);

            ScheduleGenerationWidget vista;
            vista.setNucleoDatos(&nucleo);

            auto* tabla =
                vista.findChild<QTableWidget*>(QStringLiteral("genTablaDocentes"));
            QVERIFY(tabla);
            const int inicial = tabla->rowCount();
            QVERIFY(inicial > 0);
            const QString primerDocente = tabla->item(0, 0)->text();

            // Ajuste manual en la primera fila: debe sobrevivir a la sincronización.
            auto* turno = qobject_cast<QComboBox*>(tabla->cellWidget(0, 3));
            QVERIFY(turno);
            const int indiceManana = turno->findText(QStringLiteral("Mañana"));
            QVERIFY(indiceManana > 0);
            turno->setCurrentIndex(indiceManana);

            // Preferencias editadas después de crear la vista: al volver a
            // sincronizar, las filas limpias quedan precargadas aunque el
            // conjunto de docentes no haya cambiado.
            const QString segundoDocente = tabla->item(1, 0)->text();
            QString idSegundo;
            for (const ProfesorDTO& docente : nucleo.listarDocentes().valor) {
                if (docente.nombre == segundoDocente)
                    idSegundo = docente.id;
            }
            QVERIFY(!idSegundo.isEmpty());

            PreferenciasDocenteUI::Preferencia preferencia;
            preferencia.turno = QStringLiteral("Tarde");
            preferencia.plan = QStringLiteral("NAT");
            preferencia.anio = 2;
            PreferenciasDocenteUI::guardar(idSegundo, preferencia);

            vista.sincronizarDocentes();
            QCOMPARE(qobject_cast<QComboBox*>(tabla->cellWidget(1, 3))->currentText(),
                     QStringLiteral("Tarde"));
            QCOMPARE(qobject_cast<QComboBox*>(tabla->cellWidget(1, 4))->currentText(),
                     QStringLiteral("NAT"));
            auto* aniosSegundo = qobject_cast<ComboMultiple*>(tabla->cellWidget(1, 5));
            QVERIFY(aniosSegundo);
            QCOMPARE(aniosSegundo->datosSeleccionados(), QVariantList{2});

            const auto creado = nucleo.crearDocente(
                QStringLiteral("T999"), QStringLiteral("Docente Nuevo"),
                QStringLiteral("t999@centro-de-prueba.edu.ve"));
            QVERIFY2(creado.ok, qPrintable(creado.mensajeError));

            vista.sincronizarDocentes();
            QCOMPARE(tabla->rowCount(), inicial + 1);

            int filaNuevo = -1;
            int filaPrimero = -1;
            for (int fila = 0; fila < tabla->rowCount(); ++fila) {
                const QString nombre = tabla->item(fila, 0)->text();
                if (nombre == QStringLiteral("Docente Nuevo"))
                    filaNuevo = fila;
                if (nombre == primerDocente)
                    filaPrimero = fila;
            }
            QVERIFY2(filaNuevo >= 0, "el docente nuevo debe aparecer en la tabla");
            QVERIFY(filaPrimero >= 0);
            QCOMPARE(qobject_cast<QComboBox*>(tabla->cellWidget(filaPrimero, 3))->currentText(),
                     QStringLiteral("Mañana"));

            QVERIFY(nucleo.eliminarDocente(QStringLiteral("T999")));
            vista.sincronizarDocentes();
            QCOMPARE(tabla->rowCount(), inicial);
        }
        QSqlDatabase::removeDatabase(conexion);
    }

    /// El panel de docentes (#94) guarda en la base la disponibilidad y las
    /// preferencias marcadas en las rejillas.
    void elPanelDelDocentePersisteDisponibilidadYPreferencias()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        PreferenciasDocenteUI::usarOrganizacion(
            QStringLiteral("LiceoRobertSerraPruebaPanel"),
            QStringLiteral("GestorHorariosPruebaPanel"));
        PreferenciasDocenteUI::limpiar();

        seed::OpcionesAplicacion siembra;
        siembra.rutaDb = dir.filePath(QStringLiteral("panel_docente.db"));
        const seed::ResultadoAplicacion sembrado = seed::aplicar(siembra);
        QVERIFY2(sembrado.ok(), qPrintable(sembrado.detalle));

        const QString conexion = QStringLiteral("test_panel_docente");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conexion);
            db.setDatabaseName(siembra.rutaDb);
            QVERIFY2(db.open(), qPrintable(db.lastError().text()));

            NucleoDatos nucleo(db);
            TeacherListWidget vista;
            vista.setNucleoDatos(&nucleo);
            vista.actualizarTema(true);
            vista.actualizarTema(false);

            auto* tabla = vista.findChild<QTableWidget*>(QStringLiteral("TeacherTable"));
            QVERIFY(tabla);
            QVERIFY(tabla->rowCount() > 0);

            tabla->setCurrentCell(0, 0);
            QVERIFY(QMetaObject::invokeMethod(&vista, "onDocenteSeleccionado"));

            auto* gridDisp = vista.findChild<QTableWidget*>(QStringLiteral("AvailabilityGrid"));
            auto* gridPref = vista.findChild<QTableWidget*>(QStringLiteral("PreferencesGrid"));
            QVERIFY(gridDisp && gridPref);
            QVERIFY(gridDisp->rowCount() > 0);

            const QString cedula = tabla->item(0, 0)->text();

            int filaDisp = -1;
            int colDisp = -1;
            for (int r = 0; r < gridDisp->rowCount() && filaDisp < 0; ++r) {
                for (int c = 0; c < gridDisp->columnCount(); ++c) {
                    auto* item = gridDisp->item(r, c);
                    if (item && item->toolTip() == QStringLiteral("Disponible")) {
                        filaDisp = r;
                        colDisp = c;
                        break;
                    }
                }
            }
            QVERIFY2(filaDisp >= 0, "debe existir alguna hora disponible en el seed");
            QVERIFY(QMetaObject::invokeMethod(&vista, "onDisponibilidadCellClicked",
                                              Q_ARG(int, filaDisp), Q_ARG(int, colDisp)));

            int filaPref = -1;
            int colPref = -1;
            for (int r = 0; r < gridPref->rowCount() && filaPref < 0; ++r) {
                for (int c = 0; c < gridPref->columnCount(); ++c) {
                    auto* item = gridPref->item(r, c);
                    auto* disp = gridDisp->item(r, c);
                    if (item && disp
                        && item->toolTip() == QStringLiteral("Sin preferencia")
                        && disp->toolTip() == QStringLiteral("Disponible")) {
                        filaPref = r;
                        colPref = c;
                        break;
                    }
                }
            }
            QVERIFY2(filaPref >= 0, "debe existir alguna hora libre para preferir");
            QVERIFY(QMetaObject::invokeMethod(&vista, "onPreferenciasCellClicked",
                                              Q_ARG(int, filaPref), Q_ARG(int, colPref)));

            auto* botonGuardar =
                vista.findChild<QPushButton*>(QStringLiteral("TeacherSaveButton"));
            QVERIFY(botonGuardar);
            QVERIFY(botonGuardar->isEnabled());
            botonGuardar->click();

            const auto guardado = nucleo.obtenerDocente(cedula);
            QVERIFY(guardado.ok);

            int duracion = 35;
            if (const auto config = nucleo.configuracionHoraria(); config.ok)
                duracion = config.valor.duracionMinutos;

            const QString etiquetaDisp = gridDisp->verticalHeaderItem(filaDisp)->text();
            const QTime inicioDisp = QTime::fromString(etiquetaDisp.left(5),
                                                       QStringLiteral("HH:mm"));
            QVERIFY(inicioDisp.isValid());
            const QTime finDisp = inicioDisp.addSecs(duracion * 60);

            bool cubierta = false;
            for (const DisponibilidadDTO& disp : guardado.valor.disponibilidad) {
                if (disp.franja.dia == colDisp + 1
                    && disp.franja.inicio <= inicioDisp
                    && disp.franja.fin >= finDisp) {
                    cubierta = true;
                    break;
                }
            }
            QVERIFY2(!cubierta, "la hora marcada como no disponible no debe persistir como disponible");

            const QString etiquetaPref = gridPref->verticalHeaderItem(filaPref)->text();
            const QTime inicioPref = QTime::fromString(etiquetaPref.left(5),
                                                       QStringLiteral("HH:mm"));
            QVERIFY(inicioPref.isValid());

            bool preferida = false;
            for (const PreferenciaDTO& pref : guardado.valor.preferencias) {
                if (pref.sentido == SentidoPreferencia::Preferida
                    && pref.franja.dia == colPref + 1
                    && pref.franja.inicio == inicioPref) {
                    preferida = true;
                    break;
                }
            }
            QVERIFY2(preferida, "la hora preferida debe quedar guardada en la base");

            QCOMPARE(gridDisp->item(filaDisp, colDisp)->toolTip(),
                     QStringLiteral("No disponible"));
        }
        QSqlDatabase::removeDatabase(conexion);
    }
};

QTEST_MAIN(TestSelectorGeneracion)

#include "test_selector_generacion.moc"
