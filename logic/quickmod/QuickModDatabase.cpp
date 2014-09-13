#include "QuickModDatabase.h"

#include <QDir>
#include <QCoreApplication>
#include <QTimer>

#include <pathutils.h>

#include "logic/quickmod/QuickModsList.h"
#include "InstalledMod.h"
#include "QuickModBaseDownloadAction.h"
#include "logic/MMCJson.h"
#include <logic/OneSixInstance.h>

QString QuickModDatabase::m_filename = QDir::current().absoluteFilePath("quickmods/quickmods.json");

QuickModDatabase::QuickModDatabase()
	: QObject(), m_timer(new QTimer(this))
{
	ensureFolderPathExists(QDir::current().absoluteFilePath("quickmods/"));
	loadFromDisk();
	cleanup();
	connect(qApp, &QCoreApplication::aboutToQuit, this, &QuickModDatabase::flushToDisk);
	connect(m_timer, &QTimer::timeout, this, &QuickModDatabase::flushToDisk);
	updateFiles();
}

void QuickModDatabase::setChecksum(const QUrl &url, const QByteArray &checksum)
{
	m_checksums[url] = checksum;
	delayedFlushToDisk();
}

QByteArray QuickModDatabase::checksum(const QUrl &url) const
{
	return m_checksums[url];
}

void QuickModDatabase::add(QuickModMetadataPtr metadata)
{
	m_metadata[metadata->uid().toString()][metadata->repo()] = metadata;

	delayedFlushToDisk();
}

void QuickModDatabase::add(QuickModVersionPtr version)
{
	// TODO merge versions
	m_versions[version->mod->uid().toString()][version->version().toString()] = version;

	delayedFlushToDisk();
}

QList<QuickModMetadataPtr> QuickModDatabase::allModMetadata(const QuickModRef &uid) const
{
	auto m_mods = metadata();
	return m_mods[uid];
}

QuickModMetadataPtr QuickModDatabase::someModMetadata(const QuickModRef &uid) const
{
	const auto mods = allModMetadata(uid);
	if (mods.isEmpty())
	{
		return QuickModMetadataPtr();
	}
	return mods.first();
}

QuickModVersionPtr QuickModDatabase::version(const QuickModVersionRef &version) const
{
	for (auto verPtr : m_versions[version.mod().toString()])
	{
		if (verPtr->version() == version)
		{
			return verPtr;
		}
	}
	return QuickModVersionPtr();
}

QuickModVersionRef QuickModDatabase::latestVersionForMinecraft(const QuickModRef &modUid,
															const QString &mcVersion) const
{
	QuickModVersionRef latest;
	for (auto version : versions(modUid, mcVersion))
	{
		if (!latest.isValid())
		{
			latest = version;
		}
		else if (version.isValid() && version > latest)
		{
			latest = version;
		}
	}
	return latest;
}

QStringList QuickModDatabase::minecraftVersions(const QuickModRef &uid) const
{
	QStringList out;
	auto modVersions = m_versions[uid.toString()];
	for (const auto version : modVersions)
	{
		out.append(version->compatibleVersions);
	}
	return out;
}

QList<QuickModVersionRef> QuickModDatabase::versions(const QuickModRef &uid,
												  const QString &mcVersion) const
{
	QSet<QuickModVersionRef> out;
	auto modVersions = m_versions[uid.toString()];
	for (const auto v : modVersions)
	{
		if (v->compatibleVersions.contains(mcVersion))
		{
			out.insert(v->version());
		}
	}
	return out.toList();
}

// FIXME: this doesn't belong here. invert semantics!
QList<QuickModRef> QuickModDatabase::updatedModsForInstance(std::shared_ptr<OneSixInstance> instance) const
{
	QList<QuickModRef> mods;
	auto iter = instance->installedMods()->iterateQuickMods();
	while (iter->isValid())
	{
		if (!iter->version().isValid())
		{
			iter->next();
			continue;
		}
		auto latest = latestVersionForMinecraft(iter->uid(), instance->intendedVersionId());
		if (!latest.isValid())
		{
			iter->next();
			continue;
		}
		if (iter->version() < latest)
		{
			mods.append(QuickModRef(iter->uid()));
		}
		iter->next();
	}
	return mods;
}

bool QuickModDatabase::haveUid(const QuickModRef &uid, const QString &repo) const
{
	auto m_mods = metadata();
	if (!m_mods.contains(uid))
	{
		return false;
	}
	if (repo.isNull())
	{
		return true;
	}
	for (auto mod : m_mods[uid])
	{
		if (mod->repo() == repo)
		{
			return true;
		}
	}
	return false;
}

void QuickModDatabase::registerMod(const QString &fileName)
{
	registerMod(QUrl::fromLocalFile(fileName));
}

void QuickModDatabase::registerMod(const QUrl &url)
{
	NetJob *job = new NetJob("QuickMod Download");
	job->addNetAction(QuickModBaseDownloadAction::make(job, url));
	connect(job, &NetJob::succeeded, job, &NetJob::deleteLater);
	connect(job, &NetJob::failed, job, &NetJob::deleteLater);
	job->start();
}

void QuickModDatabase::updateFiles()
{
	NetJob *job = new NetJob("QuickMod Download");
	QList<QuickModMetadataPtr> out;
	// FIXME: just remove the duct tape.
	auto m_mods = metadata();
	for (const auto mods : m_mods)
	{
		out.append(mods);
	}
	for (const auto mod : out)
	{
		job->addNetAction(
			QuickModBaseDownloadAction::make(job, mod->updateUrl(), mod->uid().toString(),
											 checksum(mod->updateUrl())));
	}
	connect(job, &NetJob::succeeded, job, &NetJob::deleteLater);
	connect(job, &NetJob::failed, job, &NetJob::deleteLater);
	job->start();
}

void QuickModDatabase::addMod(QuickModMetadataPtr mod)
{
	add(mod);

	connect(mod.get(), SIGNAL(iconUpdated(QuickModRef)), this, SIGNAL(modIconUpdated(QuickModRef)));
	connect(mod.get(), SIGNAL(logoUpdated(QuickModRef)), this, SIGNAL(modLogoUpdated(QuickModRef)));

	auto m_mods = metadata();
	m_mods[mod->uid()].append(mod);

	emit justAddedMod(mod->uid());
}

void QuickModDatabase::addVersion(QuickModVersionPtr version)
{
	add(version);
}

void QuickModDatabase::cleanup()
{
	// FIXME: rewrite.
	/*
	QDir dir;
	auto mods = MMC->quickmodSettings()->settings()->get("AvailableMods").toMap();
	QMutableMapIterator<QString, QVariant> modIt(mods);
	while (modIt.hasNext())
	{
		auto versions = modIt.next().value().toMap();
		QMutableMapIterator<QString, QVariant> versionIt(versions);
		while (versionIt.hasNext())
		{
			if (!dir.exists(versionIt.next().value().toString()))
			{
				versionIt.remove();
			}
		}
		modIt.setValue(versions);
		if (modIt.value().toMap().isEmpty())
		{
			modIt.remove();
		}
	}
	MMC->quickmodSettings()->settings()->set("AvailableMods", mods);
	*/
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

void QuickModDatabase::delayedFlushToDisk()
{
	m_isDirty = true;
	m_timer->start(1000); // one second
}

void QuickModDatabase::flushToDisk()
{
	if (!m_isDirty)
	{
		return;
	}

	QJsonObject quickmods;
	for (auto it = m_metadata.constBegin(); it != m_metadata.constEnd(); ++it)
	{
		QJsonObject metadata;
		for (auto metaIt = it.value().constBegin(); metaIt != it.value().constEnd(); ++metaIt)
		{
			metadata[metaIt.key()] = metaIt.value()->toJson();
		}

		auto versionsHash = m_versions[it.key()];
		QJsonObject versions;
		for (auto versionIt = versionsHash.constBegin(); versionIt != versionsHash.constEnd(); ++versionIt)
		{
			QJsonObject vObj = versionIt.value()->toJson();
			versions[versionIt.key()] = vObj;
		}

		QJsonObject obj;
		obj.insert("metadata", metadata);
		obj.insert("versions", versions);
		quickmods.insert(it.key(), obj);
	}

	QJsonObject checksums;
	for (auto it = m_checksums.constBegin(); it != m_checksums.constEnd(); ++it)
	{
		checksums.insert(it.key().toString(), QString::fromLatin1(it.value()));
	}

	QJsonObject root;
	root.insert("mods", quickmods);
	root.insert("checksums", checksums);

	if(!ensureFilePathExists(m_filename))
	{
		QLOG_ERROR() << "Unable to create folder for QuickMod database:" << m_filename;
		return;
	}

	QFile file(m_filename);
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Unable to save QuickMod Database to disk:" << file.errorString();
		return;
	}
	file.write(QJsonDocument(root).toJson());

	m_isDirty = false;
}

void QuickModDatabase::loadFromDisk()
{
	using namespace MMCJson;
	try
	{
		emit aboutToReset();
		m_metadata.clear();
		m_versions.clear();
		m_checksums.clear();

		const QJsonObject root = ensureObject(MMCJson::parseFile(m_filename, "QuickMod Database"));
		const QJsonObject quickmods = ensureObject(root.value("mods"));
		for (auto it = quickmods.constBegin(); it != quickmods.constEnd(); ++it)
		{
			const QString uid = it.key();
			const QJsonObject obj = ensureObject(it.value());

			// metadata
			{
				const QJsonObject metadata = ensureObject(obj.value("metadata"));
				for (auto metaIt = metadata.constBegin(); metaIt != metadata.constEnd(); ++metaIt)
				{
					QuickModMetadataPtr ptr = std::make_shared<QuickModMetadata>();
					ptr->parse(MMCJson::ensureObject(metaIt.value()));
					m_metadata[uid][metaIt.key()] = ptr;
				}
			}

			// versions
			{
				const QJsonObject versions = ensureObject(obj.value("versions"));
				for (auto versionIt = versions.constBegin(); versionIt != versions.constEnd(); ++versionIt)
				{
					// FIXME: giving it a fake 'metadata', because otherwise this causes crashes
					QuickModVersionPtr ptr = std::make_shared<QuickModVersion>(*(m_metadata[uid].begin()));
					ptr->parse(MMCJson::ensureObject(versionIt.value()));
					m_versions[uid][versionIt.key()] = ptr;
				}
			}
		}

		const QJsonObject checksums = ensureObject(root.value("checksums"));
		for (auto it = checksums.constBegin(); it != checksums.constEnd(); ++it)
		{
			m_checksums.insert(QUrl(it.key()), ensureString(it.value()).toLatin1());
		}
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << "Error while reading QuickMod Database:" << e.cause();
	}
	emit reset();
}
