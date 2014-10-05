#pragma once
#include <QAbstractListModel>
#include <memory>
#include "Transaction.h"
#include "QuickModVersion.h"

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

		QuickModMetadataPtr modObj;
		QuickModVersionPtr startVersionObj;
		QuickModVersionPtr versionObj;

		enum Status
		{
			Initial,
			Running,
			Failed,
			Done
		} status = Initial;
		QString statusString;

		int progress = 0;
		int totalProgress = 100;

		enum Download
		{
			Unknown,
			None,
			Direct,
			Web
		};
		QUrl downloadUrl;
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

private: /* data */
	std::shared_ptr<Transaction> m_transaction;
	QList<ExtendedAction> m_actions;
};
