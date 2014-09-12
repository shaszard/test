#include "QuickModDatabase.h"

#include <QDir>
#include <QCoreApplication>
#include <QTimer>

#include <pathutils.h>

#include "logic/quickmod/QuickModsList.h"
#include "logic/MMCJson.h"
#include "logic/SQLiteDatabase.h"

QString QuickModDatabase::m_filename = QDir::current().absoluteFilePath("quickmods/quickmods.sqlite3");

QuickModDatabase::QuickModDatabase(QuickModsList *list)
	: QObject(list), m_list(list), m_db(new SQLiteDatabase)
{
	ensureFolderPathExists(QDir::current().absoluteFilePath("quickmods/"));

	m_db->open(m_filename);
}

void QuickModDatabase::setChecksum(const QUrl &url, const QByteArray &checksum)
{
	m_checksums[url] = checksum;
}
QByteArray QuickModDatabase::checksum(const QUrl &url) const
{
	return m_checksums[url];
}

void QuickModDatabase::add(QuickModMetadataPtr metadata)
{
	m_metadata[metadata->uid().toString()][metadata->repo()] = metadata;
}
void QuickModDatabase::add(QuickModVersionPtr version)
{
	// TODO merge versions
	m_versions[version->mod->uid().toString()][version->version().toString()] = version;
}

QHash<QuickModRef, QList<QuickModMetadataPtr>> QuickModDatabase::metadata() const
{
	QHash<QuickModRef, QList<QuickModMetadataPtr>> out;
	for (auto uidIt = m_metadata.constBegin(); uidIt != m_metadata.constEnd(); ++uidIt)
	{
		out.insert(QuickModRef(uidIt.key()), uidIt.value().values());
	}
	return out;
}
