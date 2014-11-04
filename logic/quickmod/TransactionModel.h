#pragma once
#include <QAbstractListModel>
#include <memory>
#include "Transaction.h"
#include "QuickModVersion.h"

class QWebPage;
class QWebPage;
class TransactionModel : public QAbstractListModel
{
	Q_OBJECT
public: /* methods */
	explicit TransactionModel(std::shared_ptr<Transaction> transaction);
	virtual ~TransactionModel(){};

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual int columnCount(const QModelIndex &parent) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation,
								int role = Qt::DisplayRole) const override;

	/// Start processing the wrapped transaction
	void start();

	// FIXME: dirty hack to get this working NOW.
	QWebPage * getPage(int row);

public: /* types */
	enum ProgressRoles
	{
		EnabledRole = Qt::UserRole,
		CurrentRole,
		TotalRole,
	};

private: /* types */
	struct ExtendedAction : public Transaction::Action
	{
		ExtendedAction(const Transaction::Action &a);
		~ExtendedAction();
		enum Status
		{
			Initial,
			Downloading,
			Ready,
			Installing,
			Removing,
			Failed,
			Done
		} status = Initial;

		QuickModMetadataPtr modObj;
		QuickModVersionPtr startVersionObj;
		QuickModVersionPtr versionObj;

		QString statusString;

		int progress = 0;
		int totalProgress = 100;

		QUrl downloadUrl;
		QWebPage * dlPage = nullptr;
		CacheDownloadPtr download;
	};

	enum Column
	{
		NameColumn,
		ActionColumn,
		StatusColumn
	};

private: /* methods */
	QVariant nameData(int row, int role) const;
	QVariant actionData(int row, int role) const;
	QVariant statusData(int row, int role) const;

	void startNextDownload();

	void startInstalls();
	void startNextInstall();

	void startRemoves();
	void startNextRemove();

private: /* data */
	std::shared_ptr<Transaction> m_transaction;
	QList<ExtendedAction> m_actions;

	// index to look up removes in m_actions
	QList<int> m_idx_removes;
	int m_current_remove = -1;

	// index to look up installs in m_actions
	QList<int> m_idx_installs;
	int m_current_download = -1;
	int m_current_install = -1;

	// overall status of the transaction
	ExtendedAction::Status m_status = ExtendedAction::Initial;

signals:
	void showPageOfRow(int row);
	void hidePage();

private slots: /* slots for downloads */
	void unsupportedContent(QNetworkReply *reply);
	void loadFinished(bool);
	void webDownloadProgress(int);

	void downloadSucceeded(int);
	void downloadFailed(int);
	void downloadProgress(int,qint64,qint64);
};
