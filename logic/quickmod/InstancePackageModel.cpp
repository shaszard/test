#include "logic/quickmod/InstancePackageModel.h"
#include "logic/quickmod/InstancePackageList.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "Transaction.h"
#include "MultiMC.h"
#include <QIcon>

InstancePackageModel::InstancePackageModel(std::shared_ptr<InstancePackageList> list)
	: QAbstractListModel(), m_list(list)
{
	populate();
	connect(m_list.get(), SIGNAL(added(ChangeSource, QString)),
			SLOT(added(ChangeSource, QString)));
	connect(m_list.get(), SIGNAL(removed(ChangeSource, QString)),
			SLOT(removed(ChangeSource, QString)));
	connect(m_list.get(), SIGNAL(updated(ChangeSource, QString)),
			SLOT(updated(ChangeSource, QString)));

	auto t = m_list->getTransaction();
	connect(t.get(), SIGNAL(actionAdded(ChangeSource, QString)),
			SLOT(added(ChangeSource, QString)));
	connect(t.get(), SIGNAL(actionChanged(ChangeSource, QString)),
			SLOT(updated(ChangeSource, QString)));
	connect(t.get(), SIGNAL(actionRemoved(ChangeSource, QString)),
			SLOT(removed(ChangeSource, QString)));
}

//BEGIN: data interactions
void InstancePackageModel::populate()
{
	auto iter = m_list->iterateQuickMods();
	while (iter->isValid())
	{
		added(InstanceSource, iter->uid().toString());
		iter->next();
	}
}

void InstancePackageModel::added(ChangeSource source, QString uid)
{
	auto tracked = m_tracked.find(uid);

	// already present, we update the tracking record
	if (tracked != m_tracked.end())
	{
		auto index = m_order.indexOf(uid);
		// added to database?
		if (source == DatabaseSource)
		{
			emit dataChanged(createIndex(index, 0), createIndex(index, COLUMN_COUNT - 1));
			return;
		}
		// otherwise look at the other alternatives
		bool &change = (source == InstanceSource ? tracked->instance : tracked->transaction);
		if (change)
		{
			QLOG_ERROR() << (source == InstanceSource ? "Instance" : "Transaction")
						 << "double added uid" << uid;
		}
		change = true;
		emit dataChanged(createIndex(index, 0), createIndex(index, COLUMN_COUNT - 1));
	}
	// not present yet, we add a tracking record.
	else
	{
		TrackedItem t;
		t.uid = uid;
		bool &change = (source == InstanceSource ? t.instance : t.transaction);
		change = true;
		auto at = m_order.size();
		beginInsertRows(QModelIndex(), at, at);
		m_order.append(uid);
		m_tracked.insert(uid, t);
		endInsertRows();
	}
}

void InstancePackageModel::removed(ChangeSource source, QString uid)
{
	auto tracked = m_tracked.find(uid);
	if (tracked == m_tracked.end())
	{
		QLOG_ERROR() << "double removed uid" << uid;
		return;
	}

	auto index = m_order.indexOf(uid);

	// removed from database? just changed on our end...
	if (source == DatabaseSource)
	{
		emit dataChanged(createIndex(index, 0), createIndex(index, COLUMN_COUNT - 1));
		return;
	}

	// otherwise, determine where it was removed and stop tracking.
	bool &test = (source == InstanceSource ? tracked->instance : tracked->transaction);
	if (!test)
	{
		QLOG_ERROR() << (source == InstanceSource ? "Instance" : "Transaction")
					 << "double removed uid" << uid;
		return;
	}
	test = false;

	// completely removed?
	if ((tracked->instance || tracked->transaction) == false)
	{
		// then remove it from our tracking lists and signal view that it's gone.
		beginRemoveRows(QModelIndex(), index, index);
		m_order.removeAt(index);
		m_tracked.remove(uid);
		endRemoveRows();
	}
	// or just changed?
	else
	{
		// signal the view.
		emit dataChanged(createIndex(index, 0), createIndex(index, COLUMN_COUNT - 1));
	}
}

void InstancePackageModel::updated(ChangeSource source, QString uid)
{
	auto tracked = m_tracked.find(uid);
	if (tracked == m_tracked.end())
	{
		QLOG_INFO() << "Update to untracked uid" << uid;
		return;
	}
	auto index = m_order.indexOf(uid);
	emit dataChanged(createIndex(index, 0), createIndex(index, COLUMN_COUNT - 1));
}
//END

//BEGIN: model interface
QVariant InstancePackageModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (0 > index.row() || index.row() >= m_tracked.size())
	{
		return QVariant();
	}

	int row = index.row();

	// roles shared by all columns
	switch ((Roles)role)
	{
	case InstancePackageModel::NameRole:
		return nameData(row, Qt::DisplayRole);
	case InstancePackageModel::UidRole:
		return m_order[row];
	default:
		break;
	}

	switch ((Column)index.column())
	{
	case EnabledColumn:
		return enabledData(row, role);
	case NameColumn:
		return nameData(row, role);
	case VersionColumn:
		return versionData(row, role);
	case NewVersionColumn:
		return newVersionData(row, role);
	default:
		return QVariant();
	}
}

bool InstancePackageModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
	{
		return false;
	}

	if (role == Qt::CheckStateRole)
	{
		/*
		auto &mod = mods[index.row()];
		if (mod.enable(!mod.enabled()))
		{
			emit dataChanged(index, index);
			return true;
		}
		*/
	}
	return false;
}

QVariant InstancePackageModel::headerData(int section, Qt::Orientation orientation,
										  int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case EnabledColumn:
			return QString();
		case NameColumn:
			return QString("Name");
		case VersionColumn:
			return QString("Version");
		case NewVersionColumn:
			return QString("New Version");
		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case EnabledColumn:
			return "Is the component enabled?";
		case NameColumn:
			return "The name of the component.";
		case VersionColumn:
			return "The version of the component.";
		case NewVersionColumn:
			return "The new version of the component.";
		default:
			return QVariant();
		}
	default:
		return QVariant();
	}
	return QVariant();
}

Qt::ItemFlags InstancePackageModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
	if (index.isValid())
		return Qt::ItemIsUserCheckable | defaultFlags;
	else
		return defaultFlags;
}

QVariant InstancePackageModel::enabledData(int row, int role) const
{
	// enabling/disabling is orthogonal to the whole package management thing
	// it also adds complexity, which can be layered on later.
	// will have to wait
	if (role == Qt::CheckStateRole)
	{
		return Qt::Checked;
		// FIXME: implement
		// return mods[row].enabled() ? Qt::Checked : Qt::Unchecked;
	}
	return QVariant();
}

QVariant InstancePackageModel::nameData(int row, int role) const
{
	auto qm_uid = m_order[row];
	auto metastorage = MMC->qmdb();
	QuickModMetadataPtr qm_mod = metastorage->someModMetadata(QuickModRef(qm_uid));
	switch (role)
	{
	case Qt::DisplayRole:
	{
		if (qm_mod)
			return qm_mod->name();
		return qm_uid;
	}
	case Qt::DecorationRole:
	{
		if (qm_mod && !qm_mod->icon().isNull())
			return qm_mod->icon();
		return QIcon::fromTheme("quickmod");
	}
	case Qt::ToolTipRole:
	{
		if (qm_mod)
			return qm_mod->description();
		return "You are missing some data...";
	}
	default:
	{
		break;
	}
	}
	return QVariant();
}

QVariant InstancePackageModel::versionData(int row, int role) const
{
	auto qm_uid = m_order[row];
	auto &t = m_tracked[qm_uid];
	switch (role)
	{
	case Qt::DisplayRole:
	{
		if (t.instance)
		{
			return m_list->installedQuickModVersion(QuickModRef(qm_uid)).userFacing();
		}
		else
		{
			return tr("None");
		}
	}
	default:
	{
		break;
	}
	}
	return QVariant();
}

QVariant InstancePackageModel::newVersionData(int row, int role) const
{
	auto qm_uid = m_order[row];
	auto &t = m_tracked[qm_uid];
	switch (role)
	{
	case Qt::DisplayRole:
	{
		if (t.transaction)
		{
			auto transaction = m_list->getTransaction();
			Transaction::Action a;
			if (transaction->getAction(qm_uid, a))
			{
				if(a.type==Transaction::Action::Remove)
					return tr("Remove");
				return a.targetVersion;
			}
		}
		if (t.instance)
		{
			return m_list->installedQuickModVersion(QuickModRef(qm_uid)).userFacing();
		}
		return tr("None");
	}
	default:
	{
		break;
	}
	}
	return QVariant();
}

int InstancePackageModel::rowCount(const QModelIndex &parent) const
{
	return m_tracked.size();
}

int InstancePackageModel::columnCount(const QModelIndex &parent) const
{
	return COLUMN_COUNT;
}
//END
