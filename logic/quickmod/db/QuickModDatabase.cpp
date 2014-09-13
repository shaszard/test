#include "QuickModDatabase.h"

#include <QDir>
#include <QCoreApplication>
#include <QTimer>

#include <pathutils.h>

#include "logic/MMCJson.h"
#include "logic/db/SQLiteDatabase.h"
#include "MetadataSchema.h"
#include "VersionSchema.h"
#include "IndexSchema.h"
#include "ChecksumSchema.h"

QString QuickModDatabase::m_filename = QDir::current().absoluteFilePath("quickmods/quickmods.sqlite3");

QuickModDatabase::QuickModDatabase()
	: QObject(), m_db(new SQLiteDatabase)
{
	ensureFolderPathExists(QDir::current().absoluteFilePath("quickmods/"));

	m_db->open(m_filename);

	m_metadataRegistry = new SchemaRegistry<QuickModMetadataPtr, MetadataSchema>("metadata", m_db);
	m_checksumRegistry = new SchemaRegistry<Checksum, ChecksumSchema>("checksums", m_db);
	m_versionRegistry = new SchemaRegistry<QuickModVersionPtr, VersionSchema>("versions", m_db);

	m_metadataRegistry->registerSchema<MetadataSchema1>();
	m_checksumRegistry->registerSchema<ChecksumSchema1>();
	m_versionRegistry->registerSchema<VersionSchema1>();

	m_metadataRegistry->migrateToLatest();
	m_checksumRegistry->migrateToLatest();
	m_versionRegistry->migrateToLatest();
}

void QuickModDatabase::setChecksum(const QUrl &url, const QByteArray &checksum)
{
	m_checksumRegistry->currentSchema()->setChecksum(url, checksum);
}
QByteArray QuickModDatabase::checksum(const QUrl &url) const
{
	return m_checksumRegistry->currentSchema()->checksum(url);
}

void QuickModDatabase::add(QuickModMetadataPtr metadata)
{
	m_metadataRegistry->currentSchema()->write(metadata);
}
void QuickModDatabase::add(QuickModVersionPtr version)
{
	// TODO merge versions
	m_versionRegistry->currentSchema()->write(version);
}

QHash<QuickModRef, QList<QuickModMetadataPtr>> QuickModDatabase::metadata() const
{
	QHash<QuickModRef, QList<QuickModMetadataPtr>> out;
	QList<QuickModMetadataPtr> ptrs = m_metadataRegistry->currentSchema()->read("");
	for (auto ptr : ptrs)
	{
		out[ptr->uid()].append(ptr);
	}
	return out;
}
