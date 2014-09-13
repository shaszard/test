#pragma once

#include <QObject>
#include <QHash>
#include <memory>

#include "logic/db/Schema.h"

class QuickModsList;
class QuickModRef;
class SQLiteDatabase;
class MetadataSchema;
class VersionSchema;
class IndexSchema;
class Checksum;
class Index;
class ChecksumSchema;

typedef std::shared_ptr<class QuickModMetadata> QuickModMetadataPtr;
typedef std::shared_ptr<class QuickModVersion> QuickModVersionPtr;

class QuickModDatabase : public QObject
{
	Q_OBJECT
public:
	QuickModDatabase();

	void setChecksum(const QUrl &url, const QByteArray &checksum);
	QByteArray checksum(const QUrl &url) const;

	void add(QuickModMetadataPtr metadata);
	void add(QuickModVersionPtr version);

	QHash<QuickModRef, QList<QuickModMetadataPtr> > metadata() const;

signals:
	void aboutToReset();
	void reset();

private:
	SQLiteDatabase *m_db;
	SchemaRegistry<QuickModMetadataPtr, MetadataSchema> *m_metadataRegistry;
	SchemaRegistry<QuickModVersionPtr, VersionSchema> *m_versionRegistry;
	SchemaRegistry<Index, IndexSchema> *m_indexRegistry;
	SchemaRegistry<Checksum, ChecksumSchema> *m_checksumRegistry;

	static QString m_filename;
};
