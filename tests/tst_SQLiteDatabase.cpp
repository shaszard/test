#include <QTest>

#include <sqlite3.h>

#include "logic/db/SQLiteDatabase.h"
#include "logger/QsLog.h"
#include "logger/QsLogDest.h"

class SQLiteDatabaseTest : public QObject
{
	Q_OBJECT
	decltype(QsLogging::DestinationFactory::MakeDebugOutputDestination()) logDest;
private slots:
	void initTestCase()
	{
		logDest = QsLogging::DestinationFactory::MakeDebugOutputDestination();
		QsLogging::Logger::instance().addDestination(logDest.get());
	}
	void cleanupTestCase()
	{
	}

	void connectionTest()
	{
		SQLiteDatabase db;
		QVERIFY(!db.isOpen());
		QVERIFY(db.open("test.sqlite3"));
		QVERIFY(db.isOpen());
		db.close();
		QVERIFY(!db.isOpen());
	}

	void queryTest()
	{
		SQLiteDatabase db;
		QVERIFY(db.open("test.sqlite3"));

		SQLiteStatement *stmt = db.prepare("SELECT SQLITE_VERSION()");
		QVERIFY(stmt);
		QCOMPARE(db.next(stmt), QVariantList() << sqlite3_libversion());
		db.finalize(stmt);
	}

	void dataTypesTest()
	{
		SQLiteDatabase db;
		QVERIFY(db.open("test.sqlite3"));

		db.execute("CREATE TEMPORARY TABLE test (number INT, text VARCHAR(128), real DOUBLE)");

		const QString testString = "This is some text with ÛTF-8 äåö←¥";

		SQLiteStatement *stmt =
			db.prepare("INSERT INTO test (number, text, real) VALUES (?, ?, ?)");
		QVERIFY(stmt);
		db.bind(stmt, 42, testString, 0.05);
		db.next(stmt);
		db.finalize(stmt);

		QCOMPARE(db.executeWithReturn("SELECT number, text, real FROM test"),
				 QVariantList() << 42 << testString << 0.05);
	}

	void multirowQueryTest()
	{
		SQLiteDatabase db;
		QVERIFY(db.open("test.sqlite3"));
		db.execute("CREATE TEMPORARY TABLE test (data INT)");

		SQLiteStatement *insertStmt = db.prepare("INSERT INTO test (data) VALUES(?)");
		db.bindAndExec(insertStmt, 42);
		db.bindAndExec(insertStmt, 555);
		db.bindAndExec(insertStmt, -52);
		db.bindAndExec(insertStmt, 0);
		db.bindAndExec(insertStmt, INT64_MAX);
		db.bindAndExec(insertStmt, INT64_MIN);

		SQLiteStatement *selectCountStmt = db.prepare("SELECT COUNT(*) FROM test");
		QCOMPARE(db.next(selectCountStmt), QVariantList() << 6);

		SQLiteStatement *selectStmt =
			db.prepare("SELECT * FROM test WHERE data IN(42, 0, -52) ORDER BY data");
		QCOMPARE(db.next(selectStmt), QVariantList() << -52);
		QCOMPARE(db.next(selectStmt), QVariantList() << 0);
		QCOMPARE(db.next(selectStmt), QVariantList() << 42);
		QCOMPARE(db.next(selectStmt), QVariantList());

		db.execute("DELETE FROM test");
		selectCountStmt = db.prepare("SELECT COUNT(*) FROM test");
		QCOMPARE(db.next(selectCountStmt), QVariantList() << 0);
	}

	void queryBuilderTest()
	{
		SQLiteDatabase db;
		QVERIFY(db.open("test.sqlite3"));
		db.execute("CREATE TEMPORARY TABLE test (data INT)");
		SQLiteStatement *insertStmt = db.prepare("INSERT INTO test (data) VALUES(?)");
		db.buildInsert().into("test").columns("data").values("42").execute();
		db.buildInsert().into("test").columns("data").values("555").execute();
		db.bindAndExec(insertStmt, -52);
		db.bindAndExec(insertStmt, 0);
		db.bindAndExec(insertStmt, INT64_MAX);
		db.bindAndExec(insertStmt, INT64_MIN);

		SQLiteStatement *stmt = db.buildSelect()
									.select("data")
									.from("test")
									.where("data = 555 OR data = 0")
									.orderBy("data", Qt::DescendingOrder)
									.build();
		QCOMPARE(db.next(stmt), QVariantList() << 555);
		QCOMPARE(db.next(stmt), QVariantList() << 0);
		QCOMPARE(db.next(stmt), QVariantList());
	}
};

QTEST_APPLESS_MAIN(SQLiteDatabaseTest)

#include "tst_SQLiteDatabase.moc"
