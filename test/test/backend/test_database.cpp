#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <cassert>

#include "backend/database_manager.hpp"

#ifndef QVERIFY
#define QVERIFY(condition) assert(condition)
#endif

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QTemporaryDir tempDir;

    QVERIFY(tempDir.isValid());

    const QString dbPath = tempDir.filePath("test.db");

    DatabaseManager db;

    QVERIFY(db.initialize(dbPath));

    QVERIFY(db.isInitialized());

    {
        QSqlQuery query(db.database());

        QVERIFY(query.exec(

            "SELECT name FROM sqlite_master "

            "WHERE type='table' "

            "AND name='teachers'"

        ));

        QVERIFY(query.next());
    }

    {
        QSqlQuery query(db.database());

        QVERIFY(query.exec(

            "SELECT name FROM sqlite_master "

            "WHERE type='table' "

            "AND name='classrooms'"

        ));

        QVERIFY(query.next());
    }

    {
        QSqlQuery query(db.database());

        QVERIFY(query.exec(

            "INSERT INTO teachers(name,email)"

            "VALUES('Profesor','profesor@test.com')"

        ));

        QVERIFY(query.exec(

            "SELECT COUNT(*) FROM teachers"

        ));

        QVERIFY(query.next());

        QVERIFY(query.value(0).toInt() == 1);
    }

    db.close();

    QVERIFY(!db.isInitialized());

    return 0;
}