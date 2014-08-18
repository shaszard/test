#pragma once

#include <QAbstractItemModel>

#include "logic/net/NetJob.h"

class Node;

class TasksModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit TasksModel(QObject *parent = 0);

	enum ExtraRoles
	{
		ProgressCurrentRole = Qt::UserRole,
		ProgressTotalRole
	};

	void addItem(ProgressProvider *item);
	void addItem(NetAction *netaction);
	void addItem(std::shared_ptr<ProgressProvider> item)
	{
		addItem(item.get());
	}
	void addItem(std::shared_ptr<NetAction> netaction)
	{
		addItem(netaction.get());
	}
	void addItemAsChildTo(ProgressProvider *parent, ProgressProvider *item);
	void addItemAsChildTo(std::shared_ptr<ProgressProvider> parent,
						  std::shared_ptr<ProgressProvider> item)
	{
		addItemAsChildTo(parent.get(), item.get());
	}

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &child) const override;

private:
	Node *m_root;
	QHash<QObject *, Node *> m_items;

	friend class Node;
	QModelIndex indexFor(Node *node) const;
	void setup(Node *node);
};
