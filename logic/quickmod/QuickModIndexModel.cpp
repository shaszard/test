#include "QuickModIndexModel.h"

#include <QString>
#include <QMap>
#include <QUrl>
#include <QVariant>

#include "logic/settings/SettingsObject.h"
#include "logic/settings/Setting.h"
#include "QuickModRef.h"
#include "QuickModMetadata.h"
#include "QuickModDatabase.h"
#include "MultiMC.h"

// FIXME this file is bad

QuickModIndexModel::QuickModIndexModel(QObject *parent) : QAbstractItemModel(parent)
{
	/*
	connect(MMC->qmdb()->settings()->getSetting("Indices").get(), &Setting::SettingChanged,
	[this](const Setting &, QVariant)
	{
		reload();
	});

	QMetaObject::invokeMethod(this, "reload", Qt::QueuedConnection); // to prevent infinite loops
	*/
}

int QuickModIndexModel::rowCount(const QModelIndex &parent) const
{
	return 0;
	/*
	if (!parent.isValid())
	{
		// root -> repositories
		return m_repos.size();
	}
	else
	{
		// child -> mods
		return m_repos.at(parent.row()).mods.size();
	}
	*/
}
int QuickModIndexModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

QVariant QuickModIndexModel::data(const QModelIndex &index, int role) const
{
	/*
	if (index.parent().isValid())
	{
		const Repo repo = m_repos.at(index.row());
		if (role == Qt::DisplayRole)
		{
			switch (index.column())
			{
			case 0:
				return repo.name;
			case 1:
				return repo.url;
			}
		}
	}
	else
	{
		const Repo repo = m_repos.at(index.parent().row());
		const QuickModMetadataPtr qm = repo.mods.at(index.row());
		if (role == Qt::DisplayRole)
		{
			switch (index.column())
			{
			case 0:
				return qm->name();
			case 1:
				return qm->uid().toString();
			}
		}
		else if (role == Qt::UserRole)
		{
			return qm->internalUid();
		}
	}
	*/
	return QVariant();
}
QVariant QuickModIndexModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
		case 0:
			return tr("Name");
		case 1:
			return tr("UID");
		}
	}
	return QVariant();
}
Qt::ItemFlags QuickModIndexModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex QuickModIndexModel::index(int row, int column, const QModelIndex &parent) const
{
	return createIndex(row, column, parent.isValid() ? parent.row() : -1);
}
QModelIndex QuickModIndexModel::parent(const QModelIndex &child) const
{
	if (child.internalId() == (quintptr)-1)
	{
		return createIndex(0, 0);
	}
	else
	{
		return createIndex(child.internalId(), 0);
	}
}

void QuickModIndexModel::setRepositoryIndexUrl(const QString &repository, const QUrl &url)
{
	/*
	QMap<QString, QVariant> map = MMC->qmdb()->settings()->get("Indices").toMap();
	map[repository] = url.toString(QUrl::FullyEncoded);
	MMC->qmdb()->settings()->set("Indices", map);
	*/
}
QUrl QuickModIndexModel::repositoryIndexUrl(const QString &repository) const
{
	/*
	return QUrl(MMC->qmdb()->settings()->get("Indices").toMap()[repository].toString(), QUrl::StrictMode);
	*/
	return QUrl();
}
bool QuickModIndexModel::haveRepositoryIndexUrl(const QString &repository) const
{
	/*
	return MMC->qmdb()->settings()->get("Indices").toMap().contains(repository);
	*/
	return false;
}
QList<QUrl> QuickModIndexModel::indices() const
{
	
	QList<QUrl> out;
	/*
	const auto map = MMC->qmdb()->settings()->get("Indices").toMap();
	for (const auto value : map.values())
	{
		out.append(QUrl(value.toString(), QUrl::StrictMode));
	}
	*/
	return out;
}

void QuickModIndexModel::reload()
{
	/*
	QMap<QString, Repo> repos;
	const auto indices = MMC->quickmodSettings()->settings()->get("Indices").toMap();
	for (auto it = indices.constBegin(); it != indices.constEnd(); ++it)
	{
		repos.insert(it.key(), Repo(it.key(), it.value().toString()));
	}
	for (const auto mods : MMC->quickmodslist()->quickmods())
	{
		for (const auto mod : mods)
		{
			if (!repos.contains(mod->repo()))
			{
				repos.insert(mod->repo(), Repo(mod->repo()));
			}
			repos[mod->repo()].mods.append(mod);
		}
	}
	beginResetModel();
	m_repos = repos.values();
	endResetModel();
	*/
}
