#include <QTest>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QSqlError>
#include <backend/database/DatabaseManager.hpp>

class TestDatabase : public QObject {
    Q_OBJECT

private slots:
    void initAndCheckTables() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString dbPath = tempDir.filePath("test.db");

        DatabaseManager db;
        QVERIFY(db.initialize(dbPath));
        QVERIFY(db.isInitialized());

        // Verificar tabla Profesores
        {
            QSqlQuery query(db.database());
            QVERIFY(query.exec(
                "SELECT name FROM sqlite_master "
                "WHERE type='table' AND name='Profesores'"
            ));
            QVERIFY(query.next());
        }

        // Verificar tabla Aulas
        {
            QSqlQuery query(db.database());
            QVERIFY(query.exec(
                "SELECT name FROM sqlite_master "
                "WHERE type='table' AND name='Aulas'"
            ));
            QVERIFY(query.next());
        }

        // Insertar y leer en Profesores
        {
            QSqlQuery query(db.database());
            QVERIFY(query.exec(
                "INSERT INTO Profesores(id,nombre,email) "
                "VALUES('P001','Profesor','profesor@test.com')"
            ));
            QVERIFY(query.exec("SELECT COUNT(*) FROM Profesores"));
            QVERIFY(query.next());
            QCOMPARE(query.value(0).toInt(), 1);
        }

        db.close();
        QVERIFY(!db.isInitialized());
    }

    void doubleInitialize_isIdempotent() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        DatabaseManager db;
        QVERIFY(db.initialize(tempDir.filePath("test.db")));
        QVERIFY(db.isInitialized());

        // Segunda inicialización no debe fallar
        QVERIFY(db.initialize(tempDir.filePath("test.db")));
        QVERIFY(db.isInitialized());
    }
};

QTEST_MAIN(TestDatabase)
#include "test_database.moc"
