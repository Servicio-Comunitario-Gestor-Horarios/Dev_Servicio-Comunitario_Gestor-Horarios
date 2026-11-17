#include <QApplication>
#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <memory>

#include <backend/database/DatabaseManager.hpp>
#include <backend/services/NucleoDatos.hpp>

#include "dialogs/gestion_cursos_dialog.hpp"

/// Pruebas del diálogo que crea y elimina años y secciones (cursos).
class TestGestionCursosDialog : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<DatabaseManager> m_db;
    std::unique_ptr<NucleoDatos> m_nucleo;

    void abrirBase()
    {
        m_db.reset(new DatabaseManager());
        QVERIFY(m_db->initialize(m_dir->filePath(QStringLiteral("cursos.db"))));
        m_nucleo.reset(new NucleoDatos(m_db->database()));
    }

    int contarCursos() const
    {
        const auto lista = m_nucleo->listarCursos();
        return lista.ok ? lista.valor.size() : -1;
    }

    /// Acepta la próxima confirmación modal (QMessageBox) que aparezca.
    static void aceptarConfirmacion()
    {
        QTimer::singleShot(0, []() {
            auto* caja = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!caja)
                return;
            if (auto* si = caja->button(QMessageBox::Yes))
                si->click();
        });
    }

    static QTreeWidget* arbol(GestionCursosDialog& dialogo)
    {
        return dialogo.findChild<QTreeWidget*>(QStringLiteral("cursosArbol"));
    }

    static QPushButton* botonAgregar(GestionCursosDialog& dialogo)
    {
        return dialogo.findChild<QPushButton*>(QStringLiteral("cursosPrimario"));
    }

    static QPushButton* botonEliminar(GestionCursosDialog& dialogo)
    {
        return dialogo.findChild<QPushButton*>(QStringLiteral("cursosPeligro"));
    }

    static QComboBox* campoSeccion(GestionCursosDialog& dialogo)
    {
        return dialogo.findChild<QComboBox*>(QStringLiteral("cursosSeccion"));
    }

    static QSpinBox* campoAnio(GestionCursosDialog& dialogo)
    {
        return dialogo.findChild<QSpinBox*>(QStringLiteral("cursosAnio"));
    }

    /// Selecciona una letra aunque esté desactivada (para probar el rechazo).
    static void forzarSeccion(GestionCursosDialog& dialogo, const QString& letra)
    {
        QComboBox* combo = campoSeccion(dialogo);
        combo->setCurrentIndex(combo->findData(letra));
    }

private slots:
    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());
        abrirBase();
    }

    void cleanup()
    {
        m_nucleo.reset();
        m_db->close();
        m_db.reset();
        m_dir.reset();
    }

    void agrupaAniosYSeccionesEnOrden()
    {
        QVERIFY(m_nucleo->crearCurso(2, QStringLiteral("A")).ok);
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("B")).ok);
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QTreeWidget* arbolCursos = arbol(dialogo);
        QVERIFY(arbolCursos);
        QCOMPARE(arbolCursos->topLevelItemCount(), 2);

        QTreeWidgetItem* primero = arbolCursos->topLevelItem(0);
        QCOMPARE(primero->data(0, Qt::UserRole).toInt(), 1);
        QCOMPARE(primero->childCount(), 2);
        QCOMPARE(primero->child(0)->text(0), QStringLiteral("Sección A"));
        QCOMPARE(primero->child(1)->text(0), QStringLiteral("Sección B"));

        QTreeWidgetItem* segundo = arbolCursos->topLevelItem(1);
        QCOMPARE(segundo->data(0, Qt::UserRole).toInt(), 2);
        QCOMPARE(segundo->childCount(), 1);
    }

    void sugiereUnAnioNuevoYSuPrimeraSeccion()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);
        QVERIFY(m_nucleo->crearCurso(3, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QCOMPARE(campoAnio(dialogo)->value(), 4);
        QCOMPARE(campoSeccion(dialogo)->currentData().toString(), QStringLiteral("A"));
    }

    void agregaUnAnioNuevo()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QSignalSpy spy(&dialogo, &GestionCursosDialog::cursosCambiados);

        QTest::mouseClick(botonAgregar(dialogo), Qt::LeftButton);

        QCOMPARE(contarCursos(), 2);
        QCOMPARE(spy.count(), 1);
        QTreeWidget* arbolCursos = arbol(dialogo);
        QCOMPARE(arbolCursos->topLevelItemCount(), 2);
        QCOMPARE(arbolCursos->topLevelItem(1)->data(0, Qt::UserRole).toInt(), 2);
        // Tras crear el año 2 con la sección A, sugiere la siguiente sección libre.
        QCOMPARE(campoAnio(dialogo)->value(), 2);
        QCOMPARE(campoSeccion(dialogo)->currentData().toString(), QStringLiteral("B"));
    }

    void agregaUnaSeccionAUnAnioExistente()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);
        QVERIFY(m_nucleo->crearCurso(2, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QTreeWidget* arbolCursos = arbol(dialogo);
        arbolCursos->setCurrentItem(arbolCursos->topLevelItem(0));
        QCOMPARE(campoAnio(dialogo)->value(), 1);
        QCOMPARE(campoSeccion(dialogo)->currentData().toString(), QStringLiteral("B"));

        QTest::mouseClick(botonAgregar(dialogo), Qt::LeftButton);

        QCOMPARE(contarCursos(), 3);
        QCOMPARE(arbolCursos->topLevelItem(0)->childCount(), 2);
        QCOMPARE(arbolCursos->topLevelItem(0)->child(1)->text(0),
                 QStringLiteral("Sección B"));
    }

    void rechazaUnaSeccionDuplicada()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QTreeWidget* arbolCursos = arbol(dialogo);
        arbolCursos->setCurrentItem(arbolCursos->topLevelItem(0));

        campoAnio(dialogo)->setValue(1);
        forzarSeccion(dialogo, QStringLiteral("A"));
        QTest::mouseClick(botonAgregar(dialogo), Qt::LeftButton);

        QCOMPARE(contarCursos(), 1);
        QCOMPARE(arbolCursos->topLevelItem(0)->childCount(), 1);
    }

    void ofreceElAbecedarioEnMayusculas()
    {
        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QComboBox* combo = campoSeccion(dialogo);
        QVERIFY(combo);
        QCOMPARE(combo->count(), 26);
        QCOMPARE(combo->itemText(0), QStringLiteral("A"));
        QCOMPARE(combo->itemText(25), QStringLiteral("Z"));
        // Sin cursos, todas las letras están disponibles.
        for (int i = 0; i < combo->count(); ++i) {
            const QModelIndex indice = combo->model()->index(i, 0);
            QVERIFY(combo->model()->flags(indice) & Qt::ItemIsEnabled);
        }
    }

    void desactivaLasSeccionesYaUsadas()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("B")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QComboBox* combo = campoSeccion(dialogo);
        QTreeWidget* arbolCursos = arbol(dialogo);
        arbolCursos->setCurrentItem(arbolCursos->topLevelItem(0));
        const QModelIndex indiceA = combo->model()->index(combo->findData(QStringLiteral("A")), 0);
        const QModelIndex indiceC = combo->model()->index(combo->findData(QStringLiteral("C")), 0);
        QVERIFY(!(combo->model()->flags(indiceA) & Qt::ItemIsEnabled));
        QVERIFY(combo->model()->flags(indiceC) & Qt::ItemIsEnabled);
    }

    void limitaElAnioAlMaximoConfigurado()
    {
        for (int anio = 1; anio <= 5; ++anio)
            QVERIFY(m_nucleo->crearCurso(anio, QStringLiteral("A")).ok);

        GestionCursosDialog conCinco(m_nucleo.get(), nullptr, 5);
        QCOMPARE(campoAnio(conCinco)->maximum(), 5);
        QCOMPARE(campoAnio(conCinco)->value(), 5);

        GestionCursosDialog conSeis(m_nucleo.get(), nullptr, 6);
        QCOMPARE(campoAnio(conSeis)->maximum(), 6);
        QCOMPARE(campoAnio(conSeis)->value(), 6);
    }

    void ocultaCursosSuperioresAlMaximo()
    {
        QVERIFY(m_nucleo->crearCurso(5, QStringLiteral("A")).ok);
        QVERIFY(m_nucleo->crearCurso(6, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QTreeWidget* arbolCursos = arbol(dialogo);
        QCOMPARE(arbolCursos->topLevelItemCount(), 1);
        QCOMPARE(arbolCursos->topLevelItem(0)->data(0, Qt::UserRole).toInt(), 5);
    }

    void eliminaUnaSeccion()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("B")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QSignalSpy spy(&dialogo, &GestionCursosDialog::cursosCambiados);
        QTreeWidget* arbolCursos = arbol(dialogo);
        arbolCursos->setCurrentItem(arbolCursos->topLevelItem(0)->child(0));

        aceptarConfirmacion();
        QTest::mouseClick(botonEliminar(dialogo), Qt::LeftButton);

        QCOMPARE(contarCursos(), 1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(arbolCursos->topLevelItem(0)->childCount(), 1);
        QCOMPARE(arbolCursos->topLevelItem(0)->child(0)->text(0),
                 QStringLiteral("Sección B"));
    }

    void eliminaUnAnioCompleto()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("B")).ok);
        QVERIFY(m_nucleo->crearCurso(2, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QSignalSpy spy(&dialogo, &GestionCursosDialog::cursosCambiados);
        QTreeWidget* arbolCursos = arbol(dialogo);
        arbolCursos->setCurrentItem(arbolCursos->topLevelItem(0));

        aceptarConfirmacion();
        QTest::mouseClick(botonEliminar(dialogo), Qt::LeftButton);

        QCOMPARE(contarCursos(), 1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(arbolCursos->topLevelItemCount(), 1);
        QCOMPARE(arbolCursos->topLevelItem(0)->data(0, Qt::UserRole).toInt(), 2);
    }

    void cancelarLaConfirmacionNoElimina()
    {
        QVERIFY(m_nucleo->crearCurso(1, QStringLiteral("A")).ok);

        GestionCursosDialog dialogo(m_nucleo.get(), nullptr, 5);
        QTreeWidget* arbolCursos = arbol(dialogo);
        arbolCursos->setCurrentItem(arbolCursos->topLevelItem(0));

        QTimer::singleShot(0, []() {
            auto* caja = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!caja)
                return;
            if (auto* no = caja->button(QMessageBox::No))
                no->click();
        });
        QTest::mouseClick(botonEliminar(dialogo), Qt::LeftButton);

        QCOMPARE(contarCursos(), 1);
        QCOMPARE(arbolCursos->topLevelItem(0)->childCount(), 1);
    }
};

QTEST_MAIN(TestGestionCursosDialog)
#include "test_gestion_cursos_dialog.moc"
