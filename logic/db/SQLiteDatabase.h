#pragma once

#include <QString>
#include <QVariantList>

// QUESTION exceptions?

typedef class sqlite3_stmt SQLiteStatement;
class SQLiteDatabase;

class SelectQueryBuilder
{
public:
	SQLiteStatement *build() const;
	QVariantList execute() const;

	template <typename... Params> SelectQueryBuilder select(Params... columns)
	{
		m_columns = QStringList({columns...});
		return *this;
	}

	SelectQueryBuilder from(const QString &table)
	{
		m_table = table;
		return *this;
	}

	SelectQueryBuilder where(const QString &condition)
	{
		m_condition = condition;
		return *this;
	}

	SelectQueryBuilder orderBy(const QString &column,
							   const Qt::SortOrder order = Qt::AscendingOrder)
	{
		m_orderByColumn = column;
		m_orderByOrder = order;
		return *this;
	}

private:
	friend class SQLiteDatabase;
	explicit SelectQueryBuilder(SQLiteDatabase *db) : m_db(db)
	{
	}
	SQLiteDatabase *m_db;

	QStringList m_columns;
	QString m_table;
	QString m_condition;
	QString m_orderByColumn;
	Qt::SortOrder m_orderByOrder;

	QString createSql() const;
};

class InsertQueryBuilder
{
public:
	SQLiteStatement *build() const;
	QVariantList execute() const;

	InsertQueryBuilder into(const QString &table)
	{
		m_table = table;
		return *this;
	}
	template <typename... Params> InsertQueryBuilder columns(Params... columns)
	{
		m_columns = QStringList({columns...});
		return *this;
	}
	InsertQueryBuilder values(const QString &values)
	{
		m_values = values;
		return *this;
	}

private:
	friend class SQLiteDatabase;
	explicit InsertQueryBuilder(SQLiteDatabase *db) : m_db(db)
	{
	}
	SQLiteDatabase *m_db;

	QString m_table;
	QStringList m_columns;
	QString m_values;

	QString createSql() const;
};

class SQLiteDatabase
{
public:
	explicit SQLiteDatabase();
	~SQLiteDatabase();

	bool isOpen() const;
	bool open(const QString &filename);
	void close();

	SQLiteStatement *prepare(const QString &statement);
	QVariantList next(SQLiteStatement *stmt);
	void finalize(SQLiteStatement *stmt);
	QList<QVariantList> all(const QString &statement);

	bool execute(const QString &statement);
	QVariantList executeWithReturn(const QString &statement);

	int lastInsertId() const;

	template <typename... Params> void bind(SQLiteStatement *stmt, Params... params)
	{
		bindInternal(stmt, {QVariant::fromValue<Params>(params)...});
	}

	template <typename... Params> void bindAndExec(SQLiteStatement *stmt, Params... params)
	{
		bind(stmt, params...);
		next(stmt);
	}

	SelectQueryBuilder buildSelect();
	InsertQueryBuilder buildInsert();

private:
	class sqlite3 *m_db = nullptr;

	void bindInternal(SQLiteStatement *stmt, const QVariantList &values);
};
