#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include <backend/database/DatabaseManager.hpp>

/// Tests del esquema único de `Cursos` (RF-1, RF-5, RF-6).
class TestEsquemaCursos : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir>   m_dir;
    std::unique_ptr<DatabaseManager> m_dbManager;
    QSqlDatabase                     m_db;

    bool tablaExiste(const QString& tabla)
    {
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral("SELECT name FROM sqlite_master "
                                     "WHERE type='table' AND name=:nombre"));
        query.bindValue(QStringLiteral(":nombre"), tabla);
        return query.exec() && query.next();
    }

    bool columnaExiste(const QString& tabla, const QString& columna)
    {
        QSqlQuery query(m_db);
        if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tabla)))
            return false;
        while (query.next())
            if (query.value(1).toString().compare(columna, Qt::CaseInsensitive) == 0)
                return true;
        return false;
    }

private slots:

    void init()
    {
        m_dir.reset(new QTemporaryDir());
        QVERIFY(m_dir->isValid());
        m_dbManager.reset(new DatabaseManager());
        QVERIFY(m_dbManager->initialize(m_dir->filePath("esquema.db")));
        m_db = m_dbManager->database();
        QSqlQuery pragma(m_db);
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    }

    void cleanup()
    {
        m_dbManager->close();
        m_dbManager.reset();
        m_dir.reset();
    }

    void cursos_noTieneColumnasRetiradas()
    {
        QVERIFY(!columnaExiste(QStringLiteral("Cursos"), QStringLiteral("nombre")));
        QVERIFY(!columnaExiste(QStringLiteral("Cursos"), QStringLiteral("num_estudiantes")));
        QVERIFY(!columnaExiste(QStringLiteral("Cursos"), QStringLiteral("codigo_plan")));
        QVERIFY(!columnaExiste(QStringLiteral("Cursos"), QStringLiteral("aula_fija")));
    }

    void cursos_tieneColumnasNuevasYSellos()
    {
        QVERIFY(columnaExiste(QStringLiteral("Cursos"), QStringLiteral("anio")));
        QVERIFY(columnaExiste(QStringLiteral("Cursos"), QStringLiteral("seccion")));
        QVERIFY(columnaExiste(QStringLiteral("Cursos"), QStringLiteral("turno")));
        QVERIFY(columnaExiste(QStringLiteral("Cursos"), QStringLiteral("aula")));
        QVERIFY(columnaExiste(QStringLiteral("Cursos"), QStringLiteral("fecha_creacion")));
        QVERIFY(columnaExiste(QStringLiteral("Cursos"), QStringLiteral("fecha_modificacion")));
    }

    void noExisteCursoMateria()
    {
        QVERIFY(!tablaExiste(QStringLiteral("Curso_Materia")));
    }

    void insertarCurso(int anio, const QString& seccion, const QVariant& aula = QVariant())
    {
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral("INSERT INTO Cursos (anio, seccion, aula) "
                                     "VALUES (:anio, :seccion, :aula)"));
        query.bindValue(QStringLiteral(":anio"), anio);
        query.bindValue(QStringLiteral(":seccion"), seccion);
        query.bindValue(QStringLiteral(":aula"), aula);
        QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    }

    void unicoAnioSeccion_rechazaInsertDirectoDuplicado()
    {
        insertarCurso(1, QStringLiteral("A"));

        QSqlQuery query(m_db);
        query.prepare(QStringLiteral("INSERT INTO Cursos (anio, seccion) VALUES (:anio, :seccion)"));
        query.bindValue(QStringLiteral(":anio"), 1);
        query.bindValue(QStringLiteral(":seccion"), QStringLiteral("A"));
        QVERIFY(!query.exec()); // UNIQUE(anio, seccion)
    }

    void check_rechazaAnioCeroYNegativo()
    {
        QSqlQuery q0(m_db);
        q0.prepare(QStringLiteral("INSERT INTO Cursos (anio, seccion) VALUES (0, 'A')"));
        QVERIFY(!q0.exec());

        QSqlQuery qNeg(m_db);
        qNeg.prepare(QStringLiteral("INSERT INTO Cursos (anio, seccion) VALUES (-1, 'B')"));
        QVERIFY(!qNeg.exec());
    }

    void check_rechazaSeccionDeDosCaracteres()
    {
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral("INSERT INTO Cursos (anio, seccion) VALUES (1, 'AB')"));
        QVERIFY(!query.exec());
    }

    void fk_rechazaAulaInexistente()
    {
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral("INSERT INTO Cursos (anio, seccion, aula) VALUES (1, 'A', 99999)"));
        QVERIFY(!query.exec()); // FOREIGN KEY (aula) REFERENCES Aulas(id)
    }
};

QTEST_MAIN(TestEsquemaCursos)
#include "test_esquema_cursos.moc"
