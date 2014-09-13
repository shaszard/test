#pragma once

#include <QObject>
#include <QHash>
#include <memory>

class QuickModsList;
class QuickModRef;
class QTimer;
class QuickModMetadata;
class QuickModVersion;

typedef std::shared_ptr<QuickModMetadata> QuickModMetadataPtr;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModDatabase : public QObject
{
	friend class QuickModsList;
	Q_OBJECT
public:
	QuickModDatabase(QuickModsList *list);

	// FIXME: use metacache.
	void setChecksum(const QUrl &url, const QByteArray &checksum);
	QByteArray checksum(const QUrl &url) const;

	void add(QuickModMetadataPtr metadata);
	void add(QuickModVersionPtr version);

	QHash<QuickModRef, QList<QuickModMetadataPtr> > metadata() const;

signals:
	void aboutToReset();
	void reset();

private:
	QuickModsList *m_list;

	// FIXME: merge. there is no need for parallel structures. It only produces bugs.
	//    uid            repo     data
	QHash<QString, QHash<QString, QuickModMetadataPtr>> m_metadata;
	//    uid            version  data
	QHash<QString, QHash<QString, QuickModVersionPtr>> m_versions;

	// FIXME: use metacache.
	//    url   checksum
	QHash<QUrl, QByteArray> m_checksums;

	bool m_isDirty = false;
	QTimer *m_timer;

	static QString m_filename;

private slots:
	void delayedFlushToDisk();
	void flushToDisk();
	void loadFromDisk();
};
