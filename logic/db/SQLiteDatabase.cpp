#include "SQLiteDatabase.h"

#include <sqlite3.h>

#include "logger/QsLog.h"

SQLiteDatabase::SQLiteDatabase()
{
}

SQLiteDatabase::~SQLiteDatabase()
{
	if (isOpen())
	{
		close();
	}
}

bool SQLiteDatabase::isOpen() const
{
	return !!m_db;
}

bool SQLiteDatabase::open(const QString &filename)
{
	if (isOpen())
	{
		QLOG_ERROR() << "Database already connected";
		return false;
	}
	int rc = sqlite3_open(filename.toUtf8().constData(), &m_db);
	if (rc != SQLITE_OK)
	{
		QLOG_ERROR() << "Couldn't open database" << filename << ":" << sqlite3_errmsg(m_db);
		close();
		return false;
	}
	return true;
}
void SQLiteDatabase::close()
{
	if (!isOpen())
	{
		QLOG_WARN() << "Attemping to close non-open database";
	}
	else
	{
		sqlite3_close(m_db);
		m_db = nullptr;
	}
}

SelectQueryBuilder SQLiteDatabase::buildSelect()
{
	return SelectQueryBuilder(this);
}
InsertQueryBuilder SQLiteDatabase::buildInsert()
{
	return InsertQueryBuilder(this);
}

SQLiteStatement *SQLiteDatabase::prepare(const QString &statement)
{
	if (!isOpen())
	{
		QLOG_ERROR() << "Trying to execute a query on a non-open database";
		return nullptr;
	}

	SQLiteStatement *stmt;
	int rc = sqlite3_prepare(m_db, statement.toUtf8().constData(), statement.toUtf8().size(),
							 &stmt, 0);
	if (rc != SQLITE_OK)
	{
		QLOG_ERROR() << "Couldn't prepare" << statement << ":" << sqlite3_errmsg(m_db);
		sqlite3_finalize(stmt);
		return nullptr;
	}
	return stmt;
}
void SQLiteDatabase::finalize(SQLiteStatement *stmt)
{
	sqlite3_finalize(stmt);
}

QList<QVariantList> SQLiteDatabase::all(const QString &statement)
{
	SQLiteStatement *stmt = prepare(statement);
	QList<QVariantList> out;
	QVariantList row = next(stmt);
	while (!row.isEmpty())
	{
		out.append(row);
		row = next(stmt);
	}
	finalize(stmt);
	return out;
}

bool SQLiteDatabase::execute(const QString &statement)
{
	if (!isOpen())
	{
		QLOG_ERROR() << "Trying to execute a query on a non-open database";
		return false;
	}
	char *error = nullptr;
	sqlite3_exec(m_db, statement.toUtf8().constData(), nullptr, nullptr, &error);
	if (error)
	{
		QLOG_ERROR() << "Error while executing " << statement << ":" << error;
		sqlite3_free(error);
		return false;
	}
	return true;
}
QVariantList SQLiteDatabase::executeWithReturn(const QString &statement)
{
	SQLiteStatement *stmt = prepare(statement);
	QVariantList out = next(stmt);
	finalize(stmt);
	return out;
}

int SQLiteDatabase::lastInsertId() const
{
	return sqlite3_last_insert_rowid(m_db);
}

QVariantList SQLiteDatabase::next(SQLiteStatement *stmt)
{
	int res = sqlite3_step(stmt);
	if (res == SQLITE_ROW)
	{
		QVariantList out;
		out.reserve(sqlite3_column_count(stmt));
		for (int i = 0; i < sqlite3_column_count(stmt); ++i)
		{
			int type = sqlite3_column_type(stmt, i);
			if (type == SQLITE_INTEGER)
			{
				out += sqlite3_column_int64(stmt, i);
			}
			else if (type == SQLITE_FLOAT)
			{
				out += sqlite3_column_double(stmt, i);
			}
			else if (type == SQLITE_NULL)
			{
				out += QVariant();
			}
			else if (type == SQLITE_TEXT)
			{
				out += QString(reinterpret_cast<const QChar *>(sqlite3_column_text16(stmt, i)),
							   sqlite3_column_bytes16(stmt, i) / sizeof(QChar));
			}
		}
		return out;
	}
	else if (res == SQLITE_DONE)
	{
		return QVariantList();
	}
	return QVariantList();
}

void SQLiteDatabase::bindInternal(SQLiteStatement *stmt, const QVariantList &values)
{
	sqlite3_reset(stmt);
	for (int i = 0; i < values.size(); ++i)
	{
		const QVariant value = values.at(i);
		if (value.isNull())
		{
			sqlite3_bind_null(stmt, i + 1);
		}
		else if (value.type() == QVariant::String)
		{
			sqlite3_bind_text(stmt, i + 1, value.toString().toUtf8().constData(), -1, nullptr);
		}
		else if (value.type() == QVariant::Double)
		{
			sqlite3_bind_double(stmt, i + 1, value.toDouble());
		}
		else if (value.canConvert<qint64>())
		{
			sqlite3_bind_int64(stmt, i + 1, value.toLongLong());
		}
		else
		{
			QLOG_ERROR() << "Cannot bind" << value << ": Unknown type";
		}
	}
}

SQLiteStatement *SelectQueryBuilder::build() const
{
	return m_db->prepare(createSql());
}
QVariantList SelectQueryBuilder::execute() const
{
	return m_db->executeWithReturn(createSql());
}

QString SelectQueryBuilder::createSql() const
{
	QStringList parts;
	parts += "SELECT";
	if (m_columns.isEmpty())
	{
		parts += "*";
	}
	else
	{
		parts += m_columns.join(',');
	}
	parts += "FROM";
	parts += m_table;
	if (!m_condition.isEmpty())
	{
		parts += "WHERE";
		parts += m_condition;
	}
	if (!m_orderByColumn.isEmpty())
	{
		parts += "ORDER BY";
		parts += m_orderByColumn;
		parts += m_orderByOrder == Qt::AscendingOrder ? "ASC" : "DESC";
	}
	return parts.join(' ');
}

SQLiteStatement *InsertQueryBuilder::build() const
{
	return m_db->prepare(createSql());
}
QVariantList InsertQueryBuilder::execute() const
{
	return m_db->executeWithReturn(createSql());
}

QString InsertQueryBuilder::createSql() const
{
	QStringList parts;
	parts += "INSERT INTO";
	parts += m_table;
	parts += "(" + m_columns.join(", ") + ")";
	parts += "VALUES";
	parts += "(" + m_values + ")";
	return parts.join(' ');
}
