#pragma once

#include <QObject>
#include <QHash>
#include <memory>
#include "QuickModVersionRef.h"

class QuickModsList;
class QuickModRef;
class QTimer;
class QuickModMetadata;
class QuickModVersion;
class OneSixInstance;

typedef std::shared_ptr<QuickModMetadata> QuickModMetadataPtr;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModDatabase : public QObject
{
	friend class QuickModsList;
	Q_OBJECT
public:
	QuickModDatabase();

	// FIXME: use metacache instead.
	void setChecksum(const QUrl &url, const QByteArray &checksum);
	QByteArray checksum(const QUrl &url) const;

	void add(QuickModMetadataPtr metadata);
	void add(QuickModVersionPtr version);

	QHash<QuickModRef, QList<QuickModMetadataPtr> > metadata() const;

	// Originally from QuickModsList
	QList<QuickModMetadataPtr> allModMetadata(const QuickModRef &uid) const;
	QuickModMetadataPtr someModMetadata(const QuickModRef &uid) const;
	QuickModMetadataPtr quickmodMetadata(const QString &internalUid) const;
	QuickModVersionPtr version(const QuickModVersionRef &version) const;
	QuickModVersionRef latestVersionForMinecraft(const QuickModRef &modUid, const QString &mcVersion) const;
	QStringList minecraftVersions(const QuickModRef &uid) const;
	QList<QuickModVersionRef> versions(const QuickModRef &uid, const QString &mcVersion) const;
	bool haveUid(const QuickModRef &uid, const QString &repo = QString()) const;

	// FIXME: move to InstalledMods, query the db from there.
	QList<QuickModRef> updatedModsForInstance(std::shared_ptr<OneSixInstance> instance) const;

public slots:
	void registerMod(const QString &fileName);
	void registerMod(const QUrl &url);
	void updateFiles();

public:
	// called from QuickModDownloadAction
	void addMod(QuickModMetadataPtr mod);
	void addVersion(QuickModVersionPtr version);

signals:
	void aboutToReset();
	void reset();
	void justAddedMod(QuickModRef mod);
	void modIconUpdated(QuickModRef mod);
	void modLogoUpdated(QuickModRef mod);
	

private: /* data */
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

private: /* methods */
	// ensure that only mods that really exist on the local filesystem are marked as available
	void cleanup();
	
private slots:
	void delayedFlushToDisk();
	void flushToDisk();
	void loadFromDisk();
};
