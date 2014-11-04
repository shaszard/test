#include "TransactionModel.h"
#include "QuickModDatabase.h"
#include <QWebPage>
#include <QWebFrame>

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
		int idx = m_actions.size();
		m_actions.append(ext_action);
		if (ext_action.versionObj)
		{
			m_idx_installs.append(idx);
		}
		if (!ext_action.versionObj || ext_action.startVersionObj != ext_action.versionObj)
		{
			m_idx_removes.append(idx);
		}
	}
	endResetModel();
}

TransactionModel::ExtendedAction::ExtendedAction(const Transaction::Action &a) : Action(a)
{
	modObj = MMC->qmdb()->someModMetadata(QuickModRef(uid));
	versionObj = MMC->qmdb()->version(uid, targetVersion, targetRepo);
	startVersionObj = MMC->qmdb()->version(uid, origVersion, origRepo);

	if (type == Transaction::Action::Add || type == Transaction::Action::ChangeVersion)
	{
		if (!versionObj)
		{
			status = ExtendedAction::Failed;
			statusString = tr("Unknown version");
			return;
		}
		if (versionObj->downloads.isEmpty())
		{
			status = ExtendedAction::Failed;
			statusString = tr("Error: No download URLs");
			return;
		}
		try
		{
			downloadUrl = versionObj->highestPriorityDownload().url;
		}
		catch (MMCError & e)
		{
			status = ExtendedAction::Failed;
			statusString = e.cause();
			return;
		}
		status = ExtendedAction::Downloading;
		statusString = tr("Will be downloaded");
		return;
	}
	else if (type == Transaction::Action::Remove)
	{
		if (!startVersionObj)
		{
			status = ExtendedAction::Failed;
			statusString = tr("Unknown version");
			return;
		}
		status = ExtendedAction::Ready;
		statusString = tr("Will be removed");
		return;
	}
	else
	{
		status = ExtendedAction::Failed;
		statusString = tr("Unknown action type");
		return;
	}
}

TransactionModel::ExtendedAction::~ExtendedAction()
{
	if(dlPage)
	{
		dlPage->deleteLater();
	}
}


//BEGIN logic
void TransactionModel::start()
{
	if (m_status != ExtendedAction::Initial)
	{
		return;
	}
	m_current_remove = -1;
	m_current_download = -1;
	m_current_install = -1;
	m_status = ExtendedAction::Downloading;
	startNextDownload();
}

void TransactionModel::startNextDownload()
{
	m_current_download++;
	// done downloading?
	if (m_current_download == m_idx_installs.size())
	{
		startInstalls();
		return;
	}
	// run a download
	auto &action = m_actions[m_idx_installs[m_current_download]];
	auto URL = action.downloadUrl;
	action.dlPage = new QWebPage();
	action.dlPage->setForwardUnsupportedContent(true);
	action.dlPage->setNetworkAccessManager(MMC->qnam().get());
	connect(action.dlPage, &QWebPage::unsupportedContent, this, &TransactionModel::unsupportedContent);
	connect(action.dlPage, &QWebPage::loadFinished, this, &TransactionModel::loadFinished);
	connect(action.dlPage, &QWebPage::loadProgress, this, &TransactionModel::downloadProgress);
	QLOG_INFO() << "Grabbing: " << URL.toString();
	action.dlPage->mainFrame()->setUrl(URL);
}

void TransactionModel::unsupportedContent(QNetworkReply *reply)
{
	// we hit an actual download/content unsupported by webkit!
	QLOG_INFO() << "Got download for: " << reply->url().toString();
	auto &action = m_actions[m_idx_installs[m_current_download]];

	emit hidePage();

	action.download = CacheDownload::make(reply, action.versionObj->cacheEntry());

	auto download = action.download.get();
	connect(download, &CacheDownload::succeeded, this, &TransactionModel::downloadSucceeded);
	connect(download, &CacheDownload::failed, this, &TransactionModel::downloadFailed);
	connect(download, &CacheDownload::progress, this, &TransactionModel::downloadProgress);
	download->start();
}

void TransactionModel::downloadSucceeded(int)
{
	auto idx = m_idx_installs[m_current_download];
	auto &action = m_actions[idx];
	action.progress = 100;
	action.totalProgress = 100;
	emit dataChanged(index(idx, 0), index(idx, columnCount(QModelIndex())));
	startNextDownload();
}

void TransactionModel::downloadFailed(int)
{
	// rollback from here.
}

void TransactionModel::downloadProgress(int, qint64 progress, qint64 total)
{
	auto idx = m_idx_installs[m_current_download];
	auto &action = m_actions[idx];
	action.progress = progress;
	action.totalProgress = total;
	QLOG_INFO() << action.progress << "/" << action.totalProgress;
	emit dataChanged(index(idx, 0), index(idx, columnCount(QModelIndex())));
}

void TransactionModel::downloadProgress(int progress)
{
	auto idx = m_idx_installs[m_current_download];
	auto &action = m_actions[idx];
	action.progress = progress;
	action.totalProgress = 100;
	QLOG_INFO() << action.progress << "/" << action.totalProgress;
	emit dataChanged(index(idx, 0), index(idx, columnCount(QModelIndex())));
}


void TransactionModel::loadFinished(bool success)
{
	QLOG_INFO() << "Site load finished: " << (success ? "success" : "failure");
	if(!success)
	{
		// couldn't load it -> it's a download, most probably.
		auto &action = m_actions[m_idx_installs[m_current_download]];
		action.dlPage->deleteLater();
		action.dlPage = nullptr;
	}
	else
	{
		// it was a success, show the page.
		emit showPageOfRow(m_idx_installs[m_current_download]);
	}
}

void TransactionModel::startInstalls()
{
	emit hidePage();
	QLOG_INFO() << "Installs would start now";
}

QWebPage* TransactionModel::getPage(int row)
{
	if(row >= 0 && row < m_actions.size())
	{
		return m_actions[row].dlPage;
	}
	return nullptr;
}

//END

//BEGIN Qt model machinery

QVariant TransactionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	auto column = (Column)section;
	if (role == Qt::DisplayRole)
	{
		switch (column)
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
		case ExtendedAction::Downloading:
			return tr("Downloading");
		case ExtendedAction::Ready:
			return tr("Ready");
		case ExtendedAction::Removing:
			return tr("Removing");
		case ExtendedAction::Installing:
			return tr("Installing");
		}
	}
	case EnabledRole:
	{
		return action.status == ExtendedAction::Downloading;
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
