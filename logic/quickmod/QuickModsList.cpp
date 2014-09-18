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

#include "QuickModsList.h"

#include <QMimeData>
#include <QIcon>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "logic/net/CacheDownload.h"
#include "logic/net/NetJob.h"
#include "logic/quickmod/net/QuickModBaseDownloadAction.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "logic/Mod.h"
#include "logic/BaseInstance.h"
#include "logic/OneSixInstance.h"
#include "logic/settings/Setting.h"
#include "MultiMC.h"
#include "logic/InstanceList.h"
#include "logic/quickmod/InstanceModManager.h"
#include "modutils.h"

#include "logic/settings/INISettingsObject.h"

QuickModsList::QuickModsList(QObject *parent) : QAbstractListModel(parent)
{
	auto storage = MMC->qmdb();
	m_uids = storage->getModUIDs();

	connect(storage.get(), &QuickModDatabase::aboutToReset, [this]()
			{
		beginResetModel();
	});
	connect(storage.get(), &QuickModDatabase::reset, [this, storage]()
			{
		m_uids = storage->getModUIDs();
		endResetModel();
	});
	connect(storage.get(), SIGNAL(modIconUpdated(QuickModRef)), SLOT(modIconUpdated(QuickModRef)));
	connect(storage.get(), SIGNAL(modLogoUpdated(QuickModRef)), SLOT(modLogoUpdated(QuickModRef)));
	connect(storage.get(), SIGNAL(justAddedMod(QuickModRef)), SLOT(modAdded(QuickModRef)));
}

QuickModsList::~QuickModsList()
{
}

int QuickModsList::getQMIndex(QuickModMetadataPtr mod) const
{
	for (int i = 0; i < m_uids.count(); i++)
	{
		if (mod->uid() == m_uids[i])
		{
			return i;
		}
	}
	return -1;
}

void QuickModsList::modIconUpdated(QuickModRef uid)
{
	auto row = m_uids.indexOf(uid);
	if(row != -1)
	{
		auto modIndex = index(row, 0);
		emit dataChanged(modIndex, modIndex, QVector<int>() << Qt::DecorationRole << IconRole);
	}
}

void QuickModsList::modLogoUpdated(QuickModRef uid)
{
	auto row = m_uids.indexOf(uid);
	if(row != -1)
	{
		auto modIndex = index(row, 0);
		emit dataChanged(modIndex, modIndex, QVector<int>() << LogoRole);
	}
}

void QuickModsList::modAdded(QuickModRef uid)
{
	auto row = m_uids.indexOf(uid);
	if(row == -1)
	{
		beginInsertRows(QModelIndex(), 0, 0);
		m_uids.prepend(uid);
		endInsertRows();
	}
	else
	{
		// assume all data changed
		auto modIndex = index(row, 0);
		emit dataChanged(modIndex, modIndex);
	}
}


QHash<int, QByteArray> QuickModsList::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[NameRole] = "name";
	roles[WebsiteRole] = "website";
	roles[IconRole] = "icon";
	roles[LogoRole] = "logo";
	roles[UpdateRole] = "update";
	roles[ReferencesRole] = "recommendedUrls";
	roles[DependentUrlsRole] = "dependentUrls";
	roles[ModIdRole] = "modId";
	roles[CategoriesRole] = "categories";
	roles[TagsRole] = "tags";
	return roles;
}

int QuickModsList::rowCount(const QModelIndex &) const
{
	return m_uids.size(); // <-----
}

Qt::ItemFlags QuickModsList::flags(const QModelIndex &index) const
{
	return Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant QuickModsList::data(const QModelIndex &index, int role) const
{
	if (0 > index.row() || index.row() >= m_uids.size())
	{
		return QVariant();
	}

	auto storage = MMC->qmdb();

	QuickModMetadataPtr mod = storage->someModMetadata(m_uids[index.row()]);

	switch (role)
	{
	case Qt::DisplayRole:
		return mod->name();
	case Qt::DecorationRole:
		if (mod->icon().isNull())
		{
			return QColor(0, 0, 0, 0);
		}
		else
		{
			return mod->icon();
		}
	case Qt::ToolTipRole:
		return mod->description();
	case NameRole:
		return mod->name();
	case UidRole:
		return QVariant::fromValue(mod->uid());
	case RepoRole:
		return mod->repo();
	case DescriptionRole:
		return mod->description();
	case WebsiteRole:
		return mod->websiteUrl();
	case IconRole:
		return mod->icon();
	case LogoRole:
		return mod->logo();
	case UpdateRole:
		return mod->updateUrl();
	case ReferencesRole:
		return QVariant::fromValue(mod->references());
	case ModIdRole:
		return mod->modId();
	case CategoriesRole:
		return mod->categories();
	case MCVersionsRole:
		return MMC->qmdb()->minecraftVersions(mod->uid());
	case TagsRole:
		return mod->tags();
	case QuickModRole:
		return QVariant::fromValue(mod);
	}

	return QVariant();
}

bool QuickModsList::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row,
									int column, const QModelIndex &parent) const
{
	if (action != Qt::CopyAction)
	{
		return false;
	}
	return data->hasText() | data->hasUrls();
}

bool QuickModsList::dropMimeData(const QMimeData *data, Qt::DropAction action, int row,
								 int column, const QModelIndex &parent)
{
	if (!canDropMimeData(data, action, row, column, parent))
	{
		return false;
	}

	if (data->hasText())
	{
		MMC->qmdb()->registerMod(QUrl(data->text()));
	}
	else if (data->hasUrls())
	{
		for (const QUrl &url : data->urls())
		{
			MMC->qmdb()->registerMod(url);
		}
	}
	else
	{
		return false;
	}

	return true;
}

Qt::DropActions QuickModsList::supportedDropActions() const
{
	return Qt::CopyAction;
}

Qt::DropActions QuickModsList::supportedDragActions() const
{
	return 0;
}
