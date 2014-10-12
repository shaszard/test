/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "QuickModVersion.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "logic/net/HttpMetaCache.h"
#include "logic/OneSixInstance.h"
#include "QuickModMetadata.h"
#include "InstancePackageList.h"
#include "modutils.h"
#include <JlCompress.h>
#include "MultiMC.h"
#include "logic/MMCJson.h"
#include "logic/BaseInstance.h"

//BEGIN mod file logic
// FIXME: see the pattern. This CALLS for splitting into more classes, or abstracting away
// into data.
QString QuickModVersion::fileName() const
{
	QString ending;
	switch (installType)
	{
	case QuickModVersion::ForgeMod:
	case QuickModVersion::ForgeCoreMod:
	{
		ending = ".jar";
		break;
	}

	case QuickModVersion::LiteLoaderMod:
	{
		ending = ".litemod";
		break;
	}

	case QuickModVersion::Extract:
	case QuickModVersion::ConfigPack:
	{
		ending = ".zip";
		break;
	}

	case QuickModVersion::Group:
	default:
	{
		return QString();
	}
	}
	return mod->internalUid() + "-" + name() + ending;
}

MetaEntryPtr QuickModVersion::cacheEntry() const
{
	if(!fileName().isNull())
	{
		return MMC->metacache()->resolveEntry("quickmods/cache", fileName());
	}
	return nullptr;
}

bool QuickModVersion::needsDeploy() const
{
	return !instancePath().isNull();
}

QString QuickModVersion::storagePath() const
{
	auto entry = cacheEntry();
	if(!entry)
	{
		return QString();
	}
	return entry->getFullPath();
}

// FIXME: make this part of the json.
QString QuickModVersion::instancePath() const
{
	switch (installType)
	{
	case QuickModVersion::ForgeMod:
	case QuickModVersion::ForgeCoreMod:
	case QuickModVersion::LiteLoaderMod:
		return "mods";
	case QuickModVersion::Extract:
		return ".";
	case QuickModVersion::ConfigPack:
		return "config";
	case QuickModVersion::Group:
	default:
		return QString();
	}
}

void QuickModVersion::installInto(std::shared_ptr<OneSixInstance> m_instance)
{
	// remove any files from previously installed versions
	/*
	for (auto it : installedModFiles(version->mod->uid()))
	{
		if (!QFile::remove(PathCombine(instance->minecraftRoot(), it.path)))
		{
			QLOG_ERROR() << "Unable to remove previous version file" << it.path
						 << ", this may cause problems";
		}
		markModAsUninstalled(version->mod->uid());
	}
	*/

	QString source = PathCombine(storagePath(), fileName());
	QString destination = instancePath();

	// with nothing to install, we are finished.
	if(destination.isNull())
	{
		return;
	}
	
	destination = PathCombine(m_instance->minecraftRoot(), destination);

	// make sure the destination folder exists
	if(!ensureFolderPathExists(destination))
	{
		QLOG_INFO() << "Unable to create mod destination folder " << destination;
		throw MMCError(QObject::tr("Unable to create mod destination folder %1").arg(destination));
	}

	if (installType == QuickModVersion::Extract ||
		installType == QuickModVersion::ConfigPack)
	{
		QLOG_INFO() << "Extracting" << source << "to" << destination;
		QFileInfo finfo(source);
		const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(finfo);
		if (mimeType.inherits("application/zip"))
		{
			JlCompress::extractDir(finfo.absoluteFilePath(), destination);
		}
		else
		{
			throw MMCError(QObject::tr("Error: Trying to extract an unknown file type %1")
								   .arg(finfo.completeSuffix()));
		}
	}
	else
	{
		const QString dest = PathCombine(destination, QFileInfo(source).fileName());
		if (QFile::exists(dest))
		{
			if (!QFile::remove(dest))
			{
				throw MMCError(QObject::tr("Error: Deploying %1 to %2").arg(source, dest));
			}
		}
		if (!QFile::copy(source, dest))
		{
			throw MMCError(QObject::tr("Error: Deploying %1 to %2").arg(source, dest));
		}
		//FIXME: replace.
		// markModAsInstalled(version, dest);
	}

	// do some things to libraries.
	if (!libraries.isEmpty())
	{
		installLibrariesInto(m_instance);
	}
}

//FIXME: this is crap. Use the actual version objects!
void QuickModVersion::installLibrariesInto(std::shared_ptr<OneSixInstance> m_instance)
{
	QJsonObject obj;
	obj.insert("order", qMin(m_instance->getFullVersion()->getHighestOrder(), 99) + 1);
	obj.insert("name", mod->name());
	obj.insert("fileId", mod->uid().toString());
	obj.insert("version", name());
	obj.insert("mcVersion", m_instance->intendedVersionId());

	QJsonArray librariesJson;
	for (auto lib : libraries)
	{
		QJsonObject libObj;
		libObj.insert("name", lib.name);
		const QString urlString = lib.repo.toString(QUrl::FullyEncoded);
		libObj.insert("url", urlString);
		libObj.insert("insert", QString("prepend"));
		libObj.insert("MMC-depend", QString("soft"));
		libObj.insert("MMC-hint", QString("recurse"));
		librariesJson.append(libObj);
	}
	obj.insert("+libraries", librariesJson);

	auto filename = PathCombine(m_instance->instanceRoot(),"patches", QString("%1.json").arg(mod->uid().toString()) );

	QFile file(filename);
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName()
					 << "for reading:" << file.errorString();
		throw MMCError(QObject::tr("Error installing JSON patch"));
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	m_instance->reloadVersion();
}

QuickModDownload QuickModVersion::highestPriorityDownload(const QuickModDownload::DownloadType type)
{
	if (downloads.isEmpty())
	{
		throw MMCError(QObject::tr("No downloads available"));
	}
	return downloads.first();
}
//END


//BEGIN de/serialization
QList<QuickModVersionPtr> QuickModVersion::parse(const QJsonObject &object, QuickModMetadataPtr mod)
{
	QList<QuickModVersionPtr> out;
	for (auto versionVal : MMCJson::ensureObject(object.value("versions")))
	{
		QuickModVersionPtr ptr = std::make_shared<QuickModVersion>(mod);
		ptr->parse(MMCJson::ensureObject(versionVal));
		out.append(ptr);
	}
	return out;
}

void QuickModVersion::parse(const QJsonObject &object)
{
	versionString = MMCJson::ensureString(object.value("version"));
	if(object.contains("name"))
	{
		versionName = MMCJson::ensureString(object.value("name"), "name");
	}
	else
	{
		versionName = versionString;
	}
	if (object.contains("type"))
	{
		type = MMCJson::ensureString(object.value("type"), "type");
	}
	else
	{
		type = "Release";
	}

	m_version = Util::Version(versionString);
	sha1 = object.value(QStringLiteral("sha1")).toString();
	mcVersions.clear();
	for (auto val : MMCJson::ensureArray(object.value("mcCompat"), "mcCompat"))
	{
		mcVersions.append(MMCJson::ensureString(val));
	}
	dependencies.clear();
	recommendations.clear();
	suggestions.clear();
	conflicts.clear();
	provides.clear();
	if (object.contains("references"))
	{
		for (auto val : MMCJson::ensureArray(object.value("references"), "references"))
		{
			const QJsonObject obj = MMCJson::ensureObject(val, "reference");
			const QString uid = MMCJson::ensureString(obj.value("uid"), "uid");
			const QuickModVersionRef version = QuickModVersionRef(
				QuickModRef(uid), MMCJson::ensureString(obj.value("version"), "version"));
			const QString type = MMCJson::ensureString(obj.value("type"), "type");
			if (type == "depends")
			{
				dependencies.insert(QuickModRef(uid),
									qMakePair(version, obj.value("isSoft").toBool(false)));
			}
			else if (type == "recommends")
			{
				recommendations.insert(QuickModRef(uid), version);
			}
			else if (type == "suggests")
			{
				suggestions.insert(QuickModRef(uid), version);
			}
			else if (type == "conflicts")
			{
				conflicts.insert(QuickModRef(uid), version);
			}
			else if (type == "provides")
			{
				provides.insert(QuickModRef(uid), version);
			}
			else
			{
				throw MMCError(QObject::tr("Unknown reference type '%1'").arg(type));
			}
		}
	}

	/*
	 * FIXME: redo. we already have a library list parser. use it here.
	 */
	libraries.clear();
	if (object.contains("libraries"))
	{
		for (auto lib : MMCJson::ensureArray(object.value("libraries"), "libraries"))
		{
			const QJsonObject libObj = MMCJson::ensureObject(lib, "library");
			Library library;
			library.name = MMCJson::ensureString(libObj.value("name"), "library 'name'");
			if (libObj.contains("url"))
			{
				library.repo = MMCJson::ensureUrl(libObj.value("repo"), "library repo");
			}
			else
			{
				library.repo = QUrl("http://repo1.maven.org/maven2/");
			}
			libraries.append(library);
		}
	}

	downloads.clear();
	for (auto dlValue : MMCJson::ensureArray(object.value("urls"), "urls"))
	{
		const QJsonObject dlObject = dlValue.toObject();
		QuickModDownload download;
		download.url = MMCJson::ensureString(dlObject.value("url"), "url");
		download.priority = MMCJson::ensureInteger(dlObject.value("priority"), "priority", 0);
		// download type
		{
			const QString typeString = dlObject.value("downloadType").toString("parallel");
			if (typeString == "direct")
			{
				download.type = QuickModDownload::Direct;
			}
			else if (typeString == "parallel")
			{
				download.type = QuickModDownload::Parallel;
			}
			else if (typeString == "sequential")
			{
				download.type = QuickModDownload::Sequential;
			}
			else if (typeString == "encoded")
			{
				download.type = QuickModDownload::Encoded;
			}
			else
			{
				throw MMCError(QObject::tr("Unknown value for \"downloadType\" field"));
			}
		}
		download.hint = dlObject.value("hint").toString();
		download.group = dlObject.value("group").toString();
		downloads.append(download);
	}
	std::stable_sort(downloads.begin(), downloads.end(),
			  [](const QuickModDownload dl1, const QuickModDownload dl2)
	{ return dl1.priority < dl2.priority; });

	// install type
	{
		const QString typeString = object.value("installType").toString("forgeMod");
		if (typeString == "forgeMod")
		{
			installType = ForgeMod;
		}
		else if (typeString == "forgeCoreMod")
		{
			installType = ForgeCoreMod;
		}
		else if (typeString == "liteloaderMod")
		{
			installType = LiteLoaderMod;
		}
		else if (typeString == "extract")
		{
			installType = Extract;
		}
		else if (typeString == "configPack")
		{
			installType = ConfigPack;
		}
		else if (typeString == "group")
		{
			installType = Group;
		}
		else
		{
			throw MMCError(QObject::tr("Unknown value for \"installType\" field"));
		}
	}
}

QJsonObject QuickModVersion::toJson() const
{
	QJsonArray refs;
	auto refToJson = [&refs](const QString & type,
							 const QMap<QuickModRef, QuickModVersionRef> & references)
	{
		for (auto it = references.constBegin(); it != references.constEnd(); ++it)
		{
			QJsonObject obj;
			obj.insert("type", type);
			obj.insert("uid", it.key().toString());
			obj.insert("version", it.value().toString());
			refs.append(obj);
		}
	};

	QJsonObject obj;
	obj.insert("name", versionName);
	obj.insert("mcCompat", QJsonArray::fromStringList(mcVersions));
	MMCJson::writeString(obj, "version", versionString);
	MMCJson::writeString(obj, "type", type);
	MMCJson::writeString(obj, "sha1", sha1);
	MMCJson::writeObjectList(obj, "libraries", libraries);
	for (auto it = dependencies.constBegin(); it != dependencies.constEnd(); ++it)
	{
		QJsonObject obj;
		obj.insert("type", QStringLiteral("depends"));
		obj.insert("uid", it.key().toString());
		obj.insert("version", it.value().first.toString());
		obj.insert("isSoft", it.value().second);
		refs.append(obj);
	}
	refToJson("recommends", recommendations);
	refToJson("suggests", suggestions);
	refToJson("conflicts", conflicts);
	refToJson("provides", provides);
	obj.insert("references", refs);
	switch (installType)
	{
	case ForgeMod:
		obj.insert("installType", QStringLiteral("forgeMod"));
		break;
	case ForgeCoreMod:
		obj.insert("installType", QStringLiteral("forgeCoreMod"));
		break;
	case LiteLoaderMod:
		obj.insert("installType", QStringLiteral("liteloaderMod"));
		break;
	case Extract:
		obj.insert("installType", QStringLiteral("extract"));
		break;
	case ConfigPack:
		obj.insert("installType", QStringLiteral("configPack"));
		break;
	case Group:
		obj.insert("installType", QStringLiteral("group"));
		break;
	}
	MMCJson::writeObjectList(obj, "urls", downloads);
	return obj;
}

QJsonObject QuickModVersion::Library::toJson() const
{
	QJsonObject obj;
	obj.insert("name", name);
	if (!repo.isEmpty())
	{
		obj.insert("url", repo.toString(QUrl::FullyEncoded));
	}
	return obj;
}
//END