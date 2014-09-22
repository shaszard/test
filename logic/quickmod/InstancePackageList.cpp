#include "logic/quickmod/InstancePackageList.h"

#include <QString>
#include <QMimeDatabase>

#include <JlCompress.h>
#include <pathutils.h>

#include "logic/MMCJson.h"
#include "logic/OneSixInstance.h"
#include "logic/quickmod/QuickModRef.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "Transaction.h"
#include "MultiMC.h"

using namespace MMCJson;

// current version of the file format
const int currentVersion = 1;

InstancePackageList::InstancePackageList(std::shared_ptr< OneSixInstance > parent)
{
	parentInstance = parent;
}

QuickModVersionRef InstancePackageList::installedQuickModVersion(const QuickModRef& mod)
{
	auto moditer = installedMods_index.find(mod.toString());
	if(moditer != installedMods_index.end())
	{
		return QuickModVersionRef(mod, (*moditer)->version);
	}
	return QuickModVersionRef();
}

bool InstancePackageList::installedQuickIsHardDep(const QuickModRef& mod)
{
	auto moditer = installedMods_index.find(mod.toString());
	if(moditer != installedMods_index.end())
	{
		return !(*moditer)->asDependency;
	}
	return false;
}

bool InstancePackageList::isQuickmodInstalled(const QuickModRef& mod)
{
	return installedMods_index.contains(mod.toString());
}

bool InstancePackageList::isQuickmodWanted(const QuickModRef& mod)
{
	auto transaction = getTransaction();
	Transaction::Action a;
	if(transaction->getAction(mod.toString(), a))
	{
		if(a.type==Transaction::Action::Remove)
			return false;
		return true;
	}
	return isQuickmodInstalled(mod);
}

std::unique_ptr< QuickModView > InstancePackageList::iterateQuickMods()
{
	return std::unique_ptr<QuickModView>(new QuickModView(installedMods));
}

std::shared_ptr< Transaction > InstancePackageList::getTransaction()
{
	if(!transaction)
	{
		transaction = std::make_shared<Transaction>();
		for(auto & mod: installedMods)
		{
			transaction->insert(mod->qm_uid, mod->version, mod->qm_repo);
		}
	}
	return transaction;
}


//BEGIN TODO: Legacy stuff, rewrite

/*
bool InstancePackageList::installLibrariesFrom(const QuickModVersionPtr version)
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

	instance->reloadVersion();

	return true;
}
*/

/*
void InstancePackageList::install(const QuickModVersionPtr version)
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
}
*/

/*
bool InstancePackageList::isModMarkedAsInstalled(const QuickModRef uid) const
{
	// query structures
	return false;
}
*/

/*
QList<InstalledMod::File> InstancePackageList::installedModFiles(const QuickModRef uid) const
{
	if(installedMods_index.contains(uid.toString()))
	{
		return installedMods_index[uid.toString()]->installedFiles;
	}
	return QList<InstalledMod::File>();
}
*/

/*
void InstancePackageList::removeQuickMods(const QList<QuickModRef> &uids)
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
*/

//END

//BEGIN: Serialization/Deserialization
void InstancePackageList::insert(InstancePackagePtr mod)
{
	Q_ASSERT(!installedMods_index.contains(mod->qm_uid));
	installedMods.push_back(mod);
	installedMods_index[mod->qm_uid] = mod;
}

bool InstancePackageList::loadFromFile(QString filename)
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

bool InstancePackageList::saveToFile(QString filename)
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


void InstancePackageList::parse(const QJsonDocument& document)
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
		insert(InstancePackage::parse(obj));
	}
}

QJsonDocument InstancePackageList::serialize()
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
//END: Serialization/Deserialization

