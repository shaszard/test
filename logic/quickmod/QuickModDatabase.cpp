#include "QuickModDatabase.h"

#include <QDir>
#include <QCoreApplication>
#include <QTimer>

#include <pathutils.h>

#include "logic/quickmod/InstancePackageList.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/net/QuickModBaseDownloadAction.h"
#include "logic/MMCJson.h"
#include "logic/OneSixInstance.h"
#include "logic/settings/INISettingsObject.h"
#include "logic/settings/Setting.h"

QString QuickModDatabase::m_filename = QDir::current().absoluteFilePath("quickmods/quickmods.json");

QuickModDatabase::QuickModDatabase()
	: QObject(), m_timer(new QTimer(this)),
	m_settings(new INISettingsObject(QDir::current().absoluteFilePath("quickmod.cfg")))
{
	m_settings->registerSetting("AvailableMods",
								QVariant::fromValue(QMap<QString, QMap<QString, QString>>()));
	m_settings->registerSetting("TrustedWebsites", QVariantList());
	m_settings->registerSetting("Indices", QVariantMap());
	ensureFolderPathExists(QDir::current().absoluteFilePath("quickmods/"));
	loadFromDisk();
	checkFileCache();
	connect(qApp, &QCoreApplication::aboutToQuit, this, &QuickModDatabase::flushToDisk);
	connect(m_timer.get(), &QTimer::timeout, this, &QuickModDatabase::flushToDisk);
	updateFiles();
}

void QuickModDatabase::setLastETagForURL(const QUrl &url, const QByteArray &ETag)
{
	m_etags[url] = ETag;
	delayedFlushToDisk();
}

QByteArray QuickModDatabase::lastETagForURL(const QUrl &url) const
{
	return m_etags[url];
}

void QuickModDatabase::addMod(QuickModMetadataPtr mod)
{
	m_metadata[mod->uid()][mod->repo()] = mod;
	connect(mod.get(), SIGNAL(iconUpdated(QString)), this, SIGNAL(modIconUpdated(QString)));
	connect(mod.get(), SIGNAL(logoUpdated(QString)), this, SIGNAL(modLogoUpdated(QString)));
	delayedFlushToDisk();
	emit justAddedMod(mod->uid());
}

void QuickModDatabase::addVersion(QuickModVersionPtr version)
{
	// TODO merge versions
	m_versions[version->mod->uid()][version->version().toString()] = version;

	delayedFlushToDisk();
}

QList<QuickModMetadataPtr> QuickModDatabase::allModMetadata(const QuickModRef &uid) const
{
	auto iter = m_metadata.find(uid);
	if(iter == m_metadata.end())
	{
		return {};
	}
	return (*iter).values();
}

QuickModMetadataPtr QuickModDatabase::someModMetadata(const QuickModRef &uid) const
{
	auto iter = m_metadata.find(uid);
	if(iter == m_metadata.end())
	{
		return nullptr;
	}
	if((*iter).isEmpty())
	{
		return nullptr;
	}
	return *((*iter).begin());
}

QuickModVersionPtr QuickModDatabase::version(const QuickModVersionRef &version) const
{
	for (auto verPtr : m_versions[version.mod()])
	{
		if (verPtr->version() == version)
		{
			return verPtr;
		}
	}
	return QuickModVersionPtr();
}

// TODO fix this
QuickModVersionPtr QuickModDatabase::version(const QString &uid, const QString &version, const QString &repo) const
{
	for (auto verPtr : m_versions[QuickModRef(uid)])
	{
		if (verPtr->versionString == version && verPtr->mod->repo() == repo)
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
	QSet<QString> out;
	for (const auto version : m_versions[uid])
	{
		out.unite(version->mcVersions.toSet());
	}
	return out.toList();
}

QList<QuickModVersionRef> QuickModDatabase::versions(const QuickModRef &uid,
												  const QString &mcVersion) const
{
	QSet<QuickModVersionRef> out;
	for (const auto v : m_versions[uid])
	{
		if (v->mcVersions.contains(mcVersion))
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
	auto iter = instance->installedPackages()->iterateQuickMods();
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

QString QuickModDatabase::userFacingUid(const QString &uid) const
{
	const auto mod = someModMetadata(QuickModRef(uid));
	return mod ? mod->name() : uid;
}

bool QuickModDatabase::haveUid(const QuickModRef &uid, const QString &repo) const
{
	auto iter = m_metadata.find(uid);
	if (iter == m_metadata.end())
	{
		return false;
	}
	if (repo.isNull())
	{
		return true;
	}
	for (auto mod : *iter)
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
	for(auto mod: m_metadata)
	{
		for(auto meta: mod)
		{
			auto lastETag = lastETagForURL(meta->updateUrl());
			auto download = QuickModBaseDownloadAction::make(job, meta->updateUrl(), meta->uid().toString(), lastETag);
			job->addNetAction(download);
		}
	}
	connect(job, &NetJob::succeeded, job, &NetJob::deleteLater);
	connect(job, &NetJob::failed, job, &NetJob::deleteLater);
	job->start();
}

// FIXME: rewrite.
void QuickModDatabase::markModAsExists(QuickModMetadataPtr mod, const QuickModVersionRef &version,
									   const QString &fileName)
{
	auto mods = m_settings->get("AvailableMods").toMap();
	auto map = mods[mod->internalUid()].toMap();
	map[version.toString()] = fileName;
	mods[mod->internalUid()] = map;
	m_settings->getSetting("AvailableMods")->set(QVariant(mods));
}

// FIXME: rewrite.
bool QuickModDatabase::isModMarkedAsExists(QuickModMetadataPtr mod,
										   const QuickModVersionRef &version) const
{
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods.contains(mod->internalUid()) &&
		   mods.value(mod->internalUid()).toMap().contains(version.toString());
}

// FIXME: rewrite.
QString QuickModDatabase::existingModFile(QuickModMetadataPtr mod,
										  const QuickModVersionRef &version) const
{
	if (!isModMarkedAsExists(mod, version))
	{
		return QString();
	}
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods[mod->internalUid()].toMap()[version.toString()].toString();
}

// FIXME: rewrite.
void QuickModDatabase::checkFileCache()
{
	QDir dir;
	auto mods = m_settings->get("AvailableMods").toMap();
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
	m_settings->set("AvailableMods", mods);
}

QList<QuickModRef> QuickModDatabase::getPackageUIDs() const
{
	return m_metadata.keys();
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
		quickmods.insert(it.key().toString(), obj);
	}

	QJsonObject checksums;
	for (auto it = m_etags.constBegin(); it != m_etags.constEnd(); ++it)
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
		m_etags.clear();

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
					m_metadata[QuickModRef(uid)][metaIt.key()] = ptr;
				}
			}

			// versions
			{
				const QJsonObject versions = ensureObject(obj.value("versions"));
				for (auto versionIt = versions.constBegin(); versionIt != versions.constEnd(); ++versionIt)
				{
					// FIXME: giving it a fake 'metadata', because otherwise this causes crashes
					QuickModVersionPtr ptr = std::make_shared<QuickModVersion>(*(m_metadata[QuickModRef(uid)].begin()));
					ptr->parse(MMCJson::ensureObject(versionIt.value()));
					m_versions[QuickModRef(uid)][versionIt.key()] = ptr;
				}
			}
		}

		const QJsonObject checksums = ensureObject(root.value("checksums"));
		for (auto it = checksums.constBegin(); it != checksums.constEnd(); ++it)
		{
			m_etags.insert(QUrl(it.key()), ensureString(it.value()).toLatin1());
		}
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << "Error while reading QuickMod Database:" << e.cause();
	}
	emit reset();
}
