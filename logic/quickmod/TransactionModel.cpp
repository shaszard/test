#include "TransactionModel.h"
#include "QuickModDatabase.h"

TransactionModel::TransactionModel(std::shared_ptr<Transaction> transaction)
	: QAbstractListModel()
{
	m_transaction = transaction;

	beginResetModel();
	/// turn actions into extended actions, fit for processing/display
	auto actions = m_transaction->getActions();
	for (const auto action : actions)
	{
		ExtendedAction ext_action(action);
		m_actions.append(ext_action);
	}
	endResetModel();
}

TransactionModel::ExtendedAction::ExtendedAction(const Transaction::Action &a) : Action(a)
{
	modObj = MMC->qmdb()->someModMetadata(QuickModRef(uid));
	if (type == Transaction::Action::Add || type == Transaction::Action::ChangeVersion)
	{
		versionObj = MMC->qmdb()->version(uid, targetVersion, targetRepo);
		if (!versionObj)
		{
			status = ExtendedAction::Failed;
			statusString = tr("Unknown version");
		}
	}
	startVersionObj = MMC->qmdb()->version(uid, origVersion, origRepo);
	if(type == Transaction::Action::Remove)
	{
		if(!startVersionObj)
		{
			status = ExtendedAction::Failed;
			statusString = tr("Unknown version");
		}
	}
}

//BEGIN logic
void TransactionModel::start()
{
	// probe and classify the downloads for actions
	for(auto &action: m_actions)
	{
		auto version = action.versionObj;
		if(!version)
			continue;
		
	}
}
//END

//BEGIN Qt model machinery

QVariant TransactionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	auto column = (Column) section;
	if(role == Qt::DisplayRole)
	{
		switch(column)
		{
			case TransactionModel::ActionColumn:
				return tr("Action");
			case TransactionModel::NameColumn:
				return tr("Mod");
			case TransactionModel::StatusColumn:
				return tr("Status");
			default:
				return tr("Invalid");
		}
	}
	return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant TransactionModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	if (row > rowCount() || row < 0)
		return QVariant();

	int raw_column = index.column();
	if (raw_column > 3 || raw_column < 0)
		return QVariant();

	auto column = (Column)raw_column;
	switch (column)
	{
	case TransactionModel::NameColumn:
		return nameData(row, role);
	case TransactionModel::ActionColumn:
		return actionData(row, role);
	case TransactionModel::StatusColumn:
		return statusData(row, role);
	default:
		return QVariant();
	}
}

QVariant TransactionModel::nameData(int row, int role) const
{
	auto &action = m_actions[row];

	switch (role)
	{
	case Qt::DisplayRole:
	{
		if (action.modObj)
		{
			return action.modObj->name();
		}
		return action.uid;
	}

	default:
		return QVariant();
	}
}

QVariant TransactionModel::actionData(int row, int role) const
{
	auto &action = m_actions[row];

	switch (role)
	{
	case Qt::DisplayRole:
	{
		QString originalVersion, targetVersion;
		originalVersion = targetVersion = tr("Unknown");

		if (action.versionObj)
		{
			targetVersion = action.versionObj->name();
		}
		// TODO: else log why
		if (action.startVersionObj)
		{
			originalVersion = action.startVersionObj->name();
		}
		// TODO: else log why
		switch (action.type)
		{

		case Transaction::Action::Add:
		{
			return tr("Install version %1").arg(targetVersion);
		}
		case Transaction::Action::ChangeVersion:
		{
			return tr("Change version from %1 to %2").arg(originalVersion, targetVersion);
		}
		case Transaction::Action::Remove:
		{
			return tr("Remove");
		}
		default:
		{
			// TODO: log why
			return tr("Invalid action");
		}
		}
	}

	default:
		return QVariant();
	}
}

QVariant TransactionModel::statusData(int row, int role) const
{
	auto &action = m_actions[row];

	switch (role)
	{
	case Qt::DisplayRole:
	{
		if (action.statusString.size())
			return action.statusString;
		switch (action.status)
		{
		case ExtendedAction::Done:
			return tr("Done");
		case ExtendedAction::Failed:
			return tr("Failed");
		case ExtendedAction::Initial:
			return tr("Pending");
		case ExtendedAction::Running:
			return tr("Running");
		}
	}
	case EnabledRole:
	{
		return action.status == ExtendedAction::Running;
	}
	case CurrentRole:
	{
		return action.progress;
	}
	case TotalRole:
	{
		return action.totalProgress;
	}
	default:
		return QVariant();
	}
}

int TransactionModel::rowCount(const QModelIndex &parent) const
{
	return m_actions.size();
}

int TransactionModel::columnCount(const QModelIndex &parent) const
{
	return 3;
}
//END Qt model machinery
