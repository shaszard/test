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

#include "VersionFinal.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileSystemWatcher>
#include <QJsonDocument>

#include "OneSixVersionBuilder.h"
#include "OneSixInstance.h"
#include "MMCJson.h"

Q_DECLARE_METATYPE(Qt::CheckState)

template <typename A, typename B> QMap<A, B> invert(const QMap<B, A> &in)
{
	QMap<A, B> out;
	for (auto it = in.begin(); it != in.end(); ++it)
	{
		out.insert(it.value(), it.key());
	}
	return out;
}

ModsModel::ModsModel(VersionFinal *version, OneSixInstance *instance, QObject *parent)
	: QAbstractListModel(parent), m_version(version), m_instance(instance)
{
	m_modsDir = QDir(m_instance->minecraftRoot());
	m_modsDir.cd("mods");
	m_watcher = new QFileSystemWatcher(this);
	directoriesChanged();
	connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &ModsModel::directoriesChanged);
	m_watcher->addPath(m_instance->minecraftRoot());
	setWatching(true);
}

QVariant ModsModel::data(const QModelIndex &index, int role) const
{
	const int row = index.row();
	const int column = index.column();
	if (!index.isValid() || row < 0 || row >= m_version->mods.size() || column < 0 || column > columnCount(index.parent()))
	{
		return QVariant();
	}

	VersionFinalMod mod = m_version->mods.at(row);

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case NameColumn:
			return mod.mod.name();
		case VersionColumn:
			return mod.mod.version();
		}
	}
	else if (role == Qt::CheckStateRole)
	{
		switch (column)
		{
		case ActiveColumn:
			return mod.mod.enabled() ? Qt::Checked : Qt::Unchecked;
		}
	}
	else if (role == Qt::ToolTipRole)
	{
		return mod.mod.mmc_id();
	}
	return QVariant();
}
int ModsModel::rowCount(const QModelIndex &parent) const
{
	return m_version->mods.size();
}
int ModsModel::columnCount(const QModelIndex &parent) const
{
	return 3;
}
bool ModsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
	{
		return false;
	}

	if (role == Qt::CheckStateRole)
	{
		if (setEnabled(
					index.row(),
					value.value<Qt::CheckState>() == Qt::Checked))
		{
			emit dataChanged(index, index);
			return true;
		}
	}
	return false;
}
QVariant ModsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case ActiveColumn:
			return QString();
		case NameColumn:
			return QString("Name");
		case VersionColumn:
			return QString("Version");
		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case ActiveColumn:
			return "Is the mod enabled?";
		case NameColumn:
			return "The name of the mod.";
		case VersionColumn:
			return "The version of the mod.";
		default:
			return QVariant();
		}
	default:
		return QVariant();
	}
	return QVariant();
}
Qt::ItemFlags ModsModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
	if (index.isValid())
	{
		return Qt::ItemIsUserCheckable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |
			   defaultFlags;
	}
	else
	{
		return Qt::ItemIsDropEnabled | defaultFlags;
	}
}

bool ModsModel::installMod(const QFileInfo &file)
{
	for (int i = 0; i < m_version->mods.size(); ++i)
	{
		const VersionFinalMod mod = m_version->mods.at(i);
		if (mod.type == VersionFinalMod::Local && file == mod.mod.filename())
		{
			return true;
		}
	}
	try
	{
		QJsonObject object = m_version->getUserJsonObject();
		QJsonObject plusmods = object.value("+mods");
	}
	catch (MMCError &e)
	{
		QLOG_ERROR() << e.cause();
		return false;
	}
	return true;
}
void ModsModel::deleteMods(const int first, const int last)
{

}
Mod &ModsModel::operator[](const int index)
{
	return m_version->mods[index].mod;
}

void ModsModel::directoriesChanged()
{
	setWatching(false);
	for (auto file : m_modsDir.entryInfoList(QDir::Files | QDir::Dirs))
	{
		installMod(file);
		QFile::remove(file.absoluteFilePath());
	}
	setWatching(true);
}

void ModsModel::setWatching(const bool watching)
{
	QDir dir(m_instance->minecraftRoot());
	dir.cd("mods");
	if (watching && dir.exists())
	{
		m_watcher->addPath(dir.absolutePath());
	}
	else
	{
		m_watcher->removePath(dir.absolutePath());
	}
}

bool ModsModel::setEnabled(const int index, const bool enabled)
{
	return false;
}

VersionFinal::VersionFinal(OneSixInstance *instance, QObject *parent)
	: QAbstractListModel(parent), modsModel(new ModsModel(this, instance, this)), m_instance(instance)
{
	clear();
}

void VersionFinal::reload(const bool onlyVanilla, const QStringList &external)
{
	//FIXME: source of epic failure.
	beginResetModel();
	modsModel->beginReset();
	OneSixVersionBuilder::build(this, m_instance, onlyVanilla, external);
	reapply(true);
	endResetModel();
	modsModel->endReset();
}

void VersionFinal::clear()
{
	id.clear();
	time.clear();
	releaseTime.clear();
	type.clear();
	assets.clear();
	processArguments.clear();
	minecraftArguments.clear();
	minimumLauncherVersion = 0xDEADBEAF;
	mainClass.clear();
	libraries.clear();
	tweakers.clear();
}

bool VersionFinal::canRemove(const int index) const
{
	if (index < versionFiles.size())
	{
		return versionFiles.at(index)->fileId != "org.multimc.version.json";
	}
	return false;
}
bool VersionFinal::remove(const int index)
{
	if (canRemove(index) && QFile::remove(versionFiles.at(index)->filename))
	{
		beginResetModel();
		modsModel->beginReset();
		versionFiles.removeAt(index);
		reapply(true);
		endResetModel();
		modsModel->endReset();
		return true;
	}
	return false;
}

QString VersionFinal::versionFileId(const int index) const
{
	if (index < 0 || index >= versionFiles.size())
	{
		return QString();
	}
	return versionFiles.at(index)->fileId;
}
VersionFilePtr VersionFinal::versionFile(const QString &id)
{
	for (auto file : versionFiles)
	{
		if (file->fileId == id)
		{
			return file;
		}
	}
	return 0;
}

QJsonObject VersionFinal::getUserJsonObject() const
{
	QFile f(m_instance->instanceRoot() + "/user.json");
	if (!f.exists())
	{
		return QJsonObject();
	}
	if (!f.open(QFile::ReadOnly))
	{
		throw FileOpenError(f);
	}
	return MMCJson::ensureObject(MMCJson::parseDocument(f.readAll(), f.fileName()), f.fileName());
}
void VersionFinal::setUserJsonObject(const QJsonObject &object)
{
	QFile f(m_instance->instanceRoot() + "/user.json");
	if (!f.open(QFile::WriteOnly | QFile::Truncate))
	{
		throw FileOpenError(f);
	}
	f.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

QList<std::shared_ptr<OneSixLibrary> > VersionFinal::getActiveNormalLibs()
{
	QList<std::shared_ptr<OneSixLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}
QList<std::shared_ptr<OneSixLibrary> > VersionFinal::getActiveNativeLibs()
{
	QList<std::shared_ptr<OneSixLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

std::shared_ptr<VersionFinal> VersionFinal::fromJson(const QJsonObject &obj)
{
	std::shared_ptr<VersionFinal> version(new VersionFinal(0));
	try
	{
		OneSixVersionBuilder::readJsonAndApplyToVersion(version.get(), obj);
	}
	catch(MMCError & err)
	{
		return 0;
	}
	return version;
}

QVariant VersionFinal::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= versionFiles.size())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return versionFiles.at(row)->name;
		case 1:
			return versionFiles.at(row)->version;
		default:
			return QVariant();
		}
	}
	return QVariant();
}
QVariant VersionFinal::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			switch (section)
			{
			case 0:
				return tr("Name");
			case 1:
				return tr("Version");
			default:
				return QVariant();
			}
		}
	}
	return QVariant();
}
Qt::ItemFlags VersionFinal::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int VersionFinal::rowCount(const QModelIndex &parent) const
{
	return versionFiles.size();
}

int VersionFinal::columnCount(const QModelIndex &parent) const
{
	return 2;
}

bool VersionFinal::isCustom()
{
	return QDir(m_instance->instanceRoot()).exists("custom.json");
}
bool VersionFinal::revertToBase()
{
	return QDir(m_instance->instanceRoot()).remove("custom.json");
}

QMap<QString, int> VersionFinal::getExistingOrder() const
{

	QMap<QString, int> order;
	// default
	{
		for (auto file : versionFiles)
		{
			order.insert(file->fileId, file->order);
		}
	}
	// overriden
	{
		QMap<QString, int> overridenOrder = OneSixVersionBuilder::readOverrideOrders(m_instance);
		for (auto id : order.keys())
		{
			if (overridenOrder.contains(id))
			{
				order[id] = overridenOrder[id];
			}
		}
	}
	return order;
}

void VersionFinal::move(const int index, const MoveDirection direction)
{
	int theirIndex;
	if (direction == MoveUp)
	{
		theirIndex = index - 1;
	}
	else
	{
		theirIndex = index + 1;
	}
	if (theirIndex < 0 || theirIndex >= versionFiles.size())
	{
		return;
	}
	const QString ourId = versionFileId(index);
	const QString theirId = versionFileId(theirIndex);
	if (ourId.isNull() || ourId.startsWith("org.multimc.") ||
			theirId.isNull() || theirId.startsWith("org.multimc."))
	{
		return;
	}

	VersionFilePtr we = versionFiles[index];
	VersionFilePtr them = versionFiles[theirIndex];
	if (!we || !them)
	{
		return;
	}
	beginMoveRows(QModelIndex(), index, index, QModelIndex(), theirIndex);
	versionFiles.replace(theirIndex, we);
	versionFiles.replace(index, them);
	endMoveRows();

	auto order = getExistingOrder();
	order[ourId] = theirIndex;
	order[theirId] = index;

	if (!OneSixVersionBuilder::writeOverrideOrders(order, m_instance))
	{
		throw MMCError(tr("Couldn't save the new order"));
	}
	else
	{
		reapply();
	}
}
void VersionFinal::resetOrder()
{
	QDir(m_instance->instanceRoot()).remove("order.json");
	reapply();
}

void VersionFinal::reapply(const bool alreadyReseting)
{
	if (!alreadyReseting)
	{
		beginResetModel();
		modsModel->beginReset();
	}

	clear();

	auto existingOrders = getExistingOrder();
	QList<int> orders = existingOrders.values();
	std::sort(orders.begin(), orders.end());
	QList<VersionFilePtr> newVersionFiles;
	for (auto order : orders)
	{
		auto file = versionFile(existingOrders.key(order));
		newVersionFiles.append(file);
		file->applyTo(this);
	}
	versionFiles.swap(newVersionFiles);
	finalize();
	if (!alreadyReseting)
	{
		endResetModel();
		modsModel->endReset();
	}
}

void VersionFinal::finalize()
{
	if (assets.isEmpty())
	{
		assets = "legacy";
	}
	if (minecraftArguments.isEmpty())
	{
		QString toCompare = processArguments.toLower();
		if (toCompare == "legacy")
		{
			minecraftArguments = " ${auth_player_name} ${auth_session}";
		}
		else if (toCompare == "username_session")
		{
			minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
		}
		else if (toCompare == "username_session_version")
		{
			minecraftArguments = "--username ${auth_player_name} "
								 "--session ${auth_session} "
								 "--version ${profile_name}";
		}
	}
}
