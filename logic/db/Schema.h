#pragma once

#include <QString>
#include <QHash>

#include "SQLiteDatabase.h"
#include "logger/QsLog.h"

template<typename Object>
class Schema
{
public:
	explicit Schema(SQLiteDatabase *db)
		: m_db(db)
	{
	}
	virtual ~Schema() {}

	virtual QList<Object> read(const QString &query) const = 0;
	virtual bool write(const Object &obj) = 0;

	virtual int revision() const = 0;
	virtual bool drop() const = 0;
	virtual bool create() const = 0;

protected:

	SQLiteDatabase *m_db;

	static QString escape(const QString &string)
	{
		return QString("'%1'").arg(string);
	}
};

template<typename Object, typename Subclass = Schema<Object>>
class SchemaRegistry
{
public:
	explicit SchemaRegistry(const QString &id, SQLiteDatabase *db)
		: m_db(db), m_id(id)
	{
		m_db->execute("CREATE TABLE IF NOT EXISTS tables (id VARCHAR(64) NOT NULL, revision INT)");
	}

	template<typename Class>
	void registerSchema()
	{
		auto schema = new Class(m_db);
		m_schemas[schema->revision()] = schema;
		m_latestRevision = qMax(m_latestRevision, schema->revision());
	}

	bool migrateTo(int revision)
	{
		Schema<Object> *current = m_schemas[currentRevision()];
		Schema<Object> *target = m_schemas[revision];
		if (!current) // database is not yet setup
		{
			QLOG_INFO() << "Creating schema" << m_id;
			setCurrentRevision(target->revision());
			return target->create();
		}
		else
		{
			QLOG_INFO() << "Migrating schema" << m_id;
			const QList<Object> objects = current->read("");
			if (!current->drop())
			{
				return false;
			}
			target->create();
			setCurrentRevision(target->revision());
			for (const auto object : objects)
			{
				target->write(object);
			}
			return true;
		}
	}
	bool migrateToLatest()
	{
		if (currentRevision() == m_latestRevision)
		{
			return true;
		}
		return migrateTo(m_latestRevision);
	}

	int currentRevision()
	{
		if (m_currentRevision == -1)
		{
			const auto ret = m_db->executeWithReturn("SELECT revision FROM tables WHERE id = '" + m_id + "'");
			m_currentRevision = ret.isEmpty() ? -1 : ret.first().toInt();
		}
		return m_currentRevision;
	}

	Subclass *currentSchema()
	{
		return static_cast<Subclass *>(m_schemas[currentRevision()]);
	}

private:
	SQLiteDatabase *m_db;
	QString m_id;
	int m_currentRevision = -1;
	int m_latestRevision = 0;
	QHash<int, Schema<Object> *> m_schemas;

	void setCurrentRevision(int revision)
	{
		m_db->execute("INSERT INTO tables (id, revision) VALUES ('" + m_id + "', " + QString::number(revision) + ")");
		m_currentRevision = revision;
	}
};
