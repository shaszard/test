#include "InstalledMod.h"
#include "QuickModVersion.h"
#include "QuickModDatabase.h"
#include "logic/MMCJson.h"
#include <logic/OneSixInstance.h>
#include <QString>
#include <QMimeDatabase>
#include "pathutils.h"
#include <JlCompress.h>
#include "MultiMC.h"

using namespace MMCJson;

// current version of the file format
const int currentVersion = 1;

InstalledMods::InstalledMods(std::shared_ptr< OneSixInstance > parent)
{
	parentInstance = parent;
}

QuickModVersionRef InstalledMods::installedQuickModVersion(const QuickModRef& mod)
{
	auto moditer = installedMods_index.find(mod.toString());
	if(moditer != installedMods_index.end())
	{
		return QuickModVersionRef(mod, (*moditer)->version);
	}
	return QuickModVersionRef();
}

bool InstalledMods::installedQuickIsHardDep(const QuickModRef& mod)
{
	auto moditer = installedMods_index.find(mod.toString());
	if(moditer != installedMods_index.end())
	{
		return !(*moditer)->asDependency;
	}
	return false;
}

bool InstalledMods::isQuickmodInstalled(const QuickModRef& mod)
{
	return installedMods_index.contains(mod.toString());
}

void InstalledMods::setQuickModVersion(const QuickModRef &uid, const QuickModVersionRef &version, const bool manualInstall)
{
	setQuickModVersions(QMap<QuickModRef, QPair<QuickModVersionRef, bool>>({{uid, qMakePair(version, manualInstall)}}));
}

void InstalledMods::setQuickModVersions(const QMap<QuickModRef, QPair<QuickModVersionRef, bool>> &mods)
{
	// FIXME: this is obviously nonsense
	/*
	for (auto it = mods.begin(); it != mods.end(); ++it)
	{
		QJsonObject qmObj;
		qmObj.insert("version", it.value().first.toString());
		qmObj.insert("updateUrl", it.key().findMod()->updateUrl().toString());
		qmObj.insert("isManualInstall", it.value().second);
	}
	*/
	// save file... ?
}

void InstalledMods::removeQuickMod(const QuickModRef &uid)
{
	removeQuickMods(QList<QuickModRef>() << uid);
}

bool InstalledMods::installLibrariesFrom(const QuickModVersionPtr version)
{
	auto instance = parentInstance.lock();
	if(!instance)
	{
		QLOG_ERROR() << "Unable to access instance for mod changes.";
		throw MMCError(QObject::tr("Unable to access instance for mod changes."));
	}

	QJsonObject obj;
	obj.insert("order", qMin(instance->getFullVersion()->getHighestOrder(), 99) + 1);
	obj.insert("name", version->mod->name());
	obj.insert("fileId", version->mod->uid().toString());
	obj.insert("version", version->name());
	obj.insert("mcVersion", instance->intendedVersionId());

	QJsonArray libraries;
	for (auto lib : version->libraries)
	{
		QJsonObject libObj;
		libObj.insert("name", lib.name);
		const QString urlString = lib.repo.toString(QUrl::FullyEncoded);
		libObj.insert("url", urlString);
		libObj.insert("insert", QString("prepend"));
		libObj.insert("MMC-depend", QString("soft"));
		libObj.insert("MMC-hint", QString("recurse"));
		libraries.append(libObj);
	}
	obj.insert("+libraries", libraries);

	auto filename = PathCombine(instance->instanceRoot(),"patches", QString("%1.json").arg(version->mod->uid().toString()) );

	QFile file(filename);
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}

void InstalledMods::install(const QuickModVersionPtr version)
{
	auto instance = parentInstance.lock();
	if(!instance)
	{
		QLOG_ERROR() << "Unable to access instance for mod changes.";
		throw MMCError(QObject::tr("Unable to access instance for mod changes."));
	}

	// remove any files from previously installed versions
	for (auto it : installedModFiles(version->mod->uid()))
	{
		if (!QFile::remove(PathCombine(instance->minecraftRoot(), it.path)))
		{
			QLOG_ERROR() << "Unable to remove previous version file" << it.path
						 << ", this may cause problems";
		}
		markModAsUninstalled(version->mod->uid());
	}

	// figure out what we will be actually installing
	// FIXME: get rid of this. SRSLY
	const QString file = MMC->qmdb()->existingModFile(version->mod, version);

	// decide where to install the mod
	QString finalDirPath;
	switch (version->installType)
	{
	case QuickModVersion::LiteLoaderMod:
	case QuickModVersion::ForgeMod:
		finalDirPath = PathCombine(instance->minecraftRoot(), "mods");
		break;
	case QuickModVersion::ForgeCoreMod:
		//FIXME: refer to CoreModFolderPage::shouldDisplay() for proper implementation.
		if (instance->instanceType() == "OneSix")
		{
			finalDirPath = PathCombine(instance->minecraftRoot(), "mods");
		}
		else
		{
			finalDirPath = PathCombine(instance->minecraftRoot(), "coremods");
		}
		break;
	case QuickModVersion::Extract:
		finalDirPath = instance->minecraftRoot();
		break;
	case QuickModVersion::ConfigPack:
		finalDirPath = PathCombine(instance->minecraftRoot(), "config");
		break;
	case QuickModVersion::Group:
		return;
	}

	// make sure the destination folder exists
	if(!finalDirPath.isEmpty() && !ensureFolderPathExists(finalDirPath))
	{
		QLOG_INFO() << "Unable to create mod destination folder " << finalDirPath;
		throw MMCError(QObject::tr("Unable to create mod destination folder %1").arg(finalDirPath));
	}

	if (version->installType == QuickModVersion::Extract ||
		version->installType == QuickModVersion::ConfigPack)
	{
		QLOG_INFO() << "Extracting" << file << "to" << finalDirPath;
		QFileInfo finfo(file);
		const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(finfo);
		if (mimeType.inherits("application/zip"))
		{
			JlCompress::extractDir(finfo.absoluteFilePath(), finalDirPath);
		}
		else
		{
			throw MMCError(QObject::tr("Error: Trying to extract an unknown file type %1")
								   .arg(finfo.completeSuffix()));
		}
	}
	else
	{
		const QString dest = PathCombine(finalDirPath, QFileInfo(file).fileName());
		if (QFile::exists(dest))
		{
			if (!QFile::remove(dest))
			{
				throw MMCError(QObject::tr("Error: Deploying %1 to %2").arg(file, dest));
			}
		}
		if (!QFile::copy(file, dest))
		{
			throw MMCError(QObject::tr("Error: Deploying %1 to %2").arg(file, dest));
		}
		markModAsInstalled(version, dest);
	}

	// do some things to libraries.
	if (!version->libraries.isEmpty())
	{
		if(!installLibrariesFrom(version))
		{
			throw MMCError(QObject::tr("Error installing JSON patch"));
		}
	}
	// WAT
	/*
	else
	{
		installer.remove(std::dynamic_pointer_cast<OneSixInstance>(instance).get());
	}
	*/
}

void InstalledMods::markModAsInstalled(const QuickModVersionRef &version, QString dest)
{
	// insert entry into structures
	// save file
}

void InstalledMods::markModAsUninstalled(const QuickModRef uid)
{
	// remove entry from structures
	// save file
}

bool InstalledMods::isModMarkedAsInstalled(const QuickModRef uid) const
{
	// query structures
	return false;
}

QList<InstalledMod::File> InstalledMods::installedModFiles(const QuickModRef uid) const
{
	if(installedMods_index.contains(uid.toString()))
	{
		return installedMods_index[uid.toString()]->installedFiles;
	}
	return QList<InstalledMod::File>();
}

void InstalledMods::removeQuickMods(const QList<QuickModRef> &uids)
{
	auto instance = parentInstance.lock();
	if(!instance)
	{
		QLOG_ERROR() << "Unable to access instance for mod changes.";
		throw MMCError(QObject::tr("Unable to access instance for mod changes."));
	}

	QStringList failedFiles;
	// remove deployed files
	for (const auto uid : uids)
	{
		QString uidStr = uid.toString();
		auto iter = installedMods_index.find(uidStr);

		// already gone?
		if(iter == installedMods_index.end())
			continue;

		// remove files
		for (const auto file : (*iter)->installedFiles)
		{
			QString filename = PathCombine(instance->minecraftRoot(), file.path);
			if (QFile::remove(filename))
			{
				// file removal finished, remove the metadata
				installedMods_index.remove(uidStr);
				installedMods.removeOne(*iter);
			}
			else
			{
				failedFiles.append(filename);
			}
		}
	}
	// remove data
	if (!failedFiles.isEmpty())
	{
		throw MMCError(QObject::tr("Error while removing the following files:\n%1").arg(failedFiles.join("\n, ")));
	}
}

std::unique_ptr< QuickModView > InstalledMods::iterateQuickMods()
{
	return std::unique_ptr<QuickModView>(new QuickModView(installedMods));
}

void InstalledMods::insert(InstalledModRef mod)
{
	Q_ASSERT(!installedMods_index.contains(mod->qm_uid));
	installedMods.push_back(mod);
	installedMods_index[mod->qm_uid] = mod;
}

bool InstalledMods::loadFromFile(QString filename)
{
	auto instance = parentInstance.lock();
	if(!instance)
	{
		QLOG_ERROR() << "Unable to open mod cache file for reading: instance is dead.";
		return false;
	}

	// open the meta cache file for the instance
	QString filePath = PathCombine(instance->instanceRoot(), filename);
	QFile cacheFile(filePath);
	if(!cacheFile.exists())
	{
		return false;
	}
	if (!cacheFile.open(QIODevice::ReadOnly))
	{
		QLOG_ERROR() << "Unable to open mod cache file for reading:" << cacheFile.errorString();
		return false;
	}

	try
	{
		parse(parseDocument(cacheFile.readAll(), "Mod cache"));
	}
	catch(MMCError & e)
	{
		QLOG_ERROR() << "Unable to read mod cache file:" << e.cause();
		return false;
	}
	return true;
}

bool InstalledMods::saveToFile(QString filename)
{
	auto instance = parentInstance.lock();
	if(!instance)
	{
		QLOG_ERROR() << "Unable to open mod cache file for writing: instance is dead.";
		return false;
	}

	// open the meta cache file for the instance
	QString filePath = PathCombine(instance->instanceRoot(), filename);
	QFile cacheFile(filePath);
	if (!cacheFile.open(QIODevice::WriteOnly))
	{
		QLOG_ERROR() << "Unable to open mod cache file for writing:" << cacheFile.errorString();
		return false;
	}
	auto document = serialize();
	cacheFile.write(document.toJson());
	return true;
}


void InstalledMods::parse(const QJsonDocument& document)
{
	auto obj = document.object();

	// check document version
	auto version = ensureInteger(obj.value("formatVersion"));
	if(version > currentVersion)
		throw JSONValidationError("Invalid mods.json version. Contents will be ignored.");

	// load all the mod entries
	auto array = ensureArray(obj.value("installed"));
	for(auto item: array)
	{
		auto obj = ensureObject(item);
		insert(InstalledMod::parse(obj));
	}
}

QJsonDocument InstalledMods::serialize()
{
	QJsonObject root;
	root.insert("formatVersion", currentVersion);

	// save all the mod entries
	QJsonArray modArray;
	for(auto & mod: installedMods)
	{
		modArray.append(mod->serialize());
	}
	root.insert("installed", modArray);

	QJsonDocument doc;
	doc.setObject(root);
	return doc;
}

std::shared_ptr<InstalledMod> InstalledMod::parse(const QJsonObject &valueObject)
{
	std::shared_ptr<InstalledMod> mod(new InstalledMod);
	mod->name = ensureString(valueObject.value("name"));
	mod->version = ensureString(valueObject.value("version"));
	mod->qm_uid = ensureString(valueObject.value("qm_uid"));
	mod->qm_updateUrl = ensureString(valueObject.value("qm_updateUrl"));
	mod->asDependency = valueObject.value("asDependency").toBool();
	mod->installedPatch = valueObject.value("installedPatch").toString();
	auto installedFilesJson = valueObject.value("installedFiles");

	// optional file array. only installed mods have files.
	auto array = installedFilesJson.toArray();
	for(auto item: array)
	{
		File f;
		auto fileObject = ensureObject(item);
		f.path = ensureString(fileObject.value("path"));
		f.sha1 = ensureString(fileObject.value("sha1"));
		// f.last_changed_timestamp = ensureDouble(fileObject.value("last_changed_timestamp"));
		mod->installedFiles.push_back(f);
	}
	return mod;
}

QJsonObject InstalledMod::serialize()
{
	QJsonObject obj;

	obj.insert("qm_uid", qm_uid);
	obj.insert("name", name);
	obj.insert("version", version);
	obj.insert("qm_updateUrl", qm_updateUrl);

	if(asDependency)
		obj.insert("asDependency", QJsonValue(true));

	if(!installedPatch.isEmpty())
		obj.insert("installedPatch", installedPatch);

	QJsonArray fileArray;
	for(auto & file: installedFiles)
	{
		QJsonObject fileObj;
		fileObj.insert("path", file.path);
		fileObj.insert("sha1", file.sha1);
		// fileObj.insert("last_changed_timestamp", QJsonValue( double(file.last_changed_timestamp)));
		fileArray.append(fileObj);
	}
	if(fileArray.size())
		obj.insert("installedFiles", fileArray);

	return obj;
}
