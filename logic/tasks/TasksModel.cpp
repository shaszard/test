#include "TasksModel.h"

#include "SequentialTask.h"
#include "Task.h"

class Node : public QObject
{
	Q_OBJECT
public:
	explicit Node(TasksModel *model, NetAction *netaction, Node *parent = 0)
		: QObject(parent), m_model(model), m_parent(parent)
	{
		m_model->m_items.insert(netaction, this);
		m_model->setup(this);

		connect(netaction, &NetAction::progress, [this](int, qint64 current, qint64 total)
		{ setProgress(current, total); });
		connect(netaction, &NetAction::started, [this](int)
		{ setState(Running); });
		connect(netaction, &NetAction::succeeded, [this](int)
		{ setState(Done); });
		connect(netaction, &NetAction::failed, [this](int)
		{ setState(Done); });
		m_status = m_name = netaction->m_url.toString(QUrl::PrettyDecoded);
		m_progress = qMakePair(netaction->m_progress, netaction->m_total_progress);
		m_state = Running;
	}
	explicit Node(TasksModel *model, ProgressProvider *task, Node *parent = 0)
		: QObject(parent), m_model(model), m_parent(parent)
	{
		m_model->m_items.insert(task, this);
		m_model->setup(this);

		connect(task, &ProgressProvider::status, this, &Node::setStatus);
		connect(task, &ProgressProvider::progress, this, &Node::setProgress);
		connect(task, &ProgressProvider::started, [this]()
		{ setState(Running); });
		connect(task, &ProgressProvider::succeeded, [this]()
		{ setState(Done); });
		connect(task, &ProgressProvider::failed, [this](const QString &status)
		{
			setState(Done);
			setStatus(status);
		});
		m_status = task->getStatus();
		qint64 current, total;
		task->getProgress(current, total);
		m_progress = qMakePair(current, total);
		m_name = task->getName();

		// TODO a way to actually find the current state of a task
		if (task->isRunning())
		{
			m_state = Running;
		}

		if (SequentialTask *sequential = dynamic_cast<SequentialTask *>(task))
		{
			qDebug() << "adding sequential task" << sequential << "with parent" << parent
					 << this;
			m_children.reserve(sequential->size());
			for (int i = 0; i < sequential->size(); ++i)
			{
				m_children.append(new Node(m_model, sequential->at(i).get(), this));
			}
		}
		else if (NetJob *netjob = dynamic_cast<NetJob *>(task))
		{
			qDebug() << "adding netjob" << netjob << "with parent" << parent << this;
			m_children.reserve(netjob->size());
			for (int i = 0; i < netjob->size(); ++i)
			{
				m_children.append(new Node(m_model, netjob->at(i).get(), this));
			}
			connect(netjob, &NetJob::failed, [this]()
			{ setState(Done); });
			connect(netjob, &NetJob::childAdded, [this](NetActionPtr child)
			{ addChild(new Node(m_model, child.get(), this)); });
		}
		else if (Task *tsk = dynamic_cast<Task *>(task))
		{
			qDebug() << "adding regular task" << task << "with parent" << parent << this;
			if (tsk->successful())
			{
				m_state = Done;
			}
		}
	}
	explicit Node(TasksModel *model) : QObject(0), m_model(model), m_parent(0)
	{
		m_model->setup(this);
	}

	void addChild(Node *node)
	{
		m_model->beginInsertRows(m_model->indexFor(this), m_children.size(), m_children.size());
		m_model->setup(node);
		m_children.append(node);
		node->m_parent = this;
		m_model->endInsertRows();
	}

	int numChildren() const
	{
		return m_children.size();
	}
	Node *child(int index) const
	{
		return m_children.at(index);
	}
	Node *parent() const
	{
		return m_parent;
	}
	int indexInParent() const
	{
		if (m_parent)
		{
			return m_parent->m_children.indexOf(const_cast<Node *>(this));
		}

		return 0;
	}

	enum State
	{
		Idle,
		Running,
		Done
	};

	QString status() const
	{
		return m_status;
	}
	QPair<qint64, qint64> progress() const
	{
		return m_progress;
	}
	State state() const
	{
		return m_state;
	}
	QString name() const
	{
		return m_name;
	}

signals:
	void statusChanged();
	void progressChanged();
	void stateChanged();

private:
	TasksModel *m_model;
	QList<Node *> m_children;
	Node *m_parent;
	QString m_status;
	QPair<qint64, qint64> m_progress;
	State m_state;
	QString m_name;

private
slots:
	void setStatus(const QString &status)
	{
		m_status = status;
		emit statusChanged();
	}
	void setProgress(const qint64 current, const qint64 total)
	{
		m_progress = qMakePair(current, total);
		emit progressChanged();
	}
	void setState(const State state)
	{
		m_state = state;
		emit stateChanged();
	}
};

TasksModel::TasksModel(QObject *parent) : QAbstractItemModel(parent), m_root(new Node(this))
{
}

int TasksModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid())
	{
		return m_root->numChildren();
	}
	else
	{
		return static_cast<Node *>(parent.internalPointer())->numChildren();
	}
}
int TasksModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

QVariant TasksModel::data(const QModelIndex &index, int role) const
{
	Node *node = static_cast<Node *>(index.internalPointer());
	if (index.column() == 0 && role == Qt::DisplayRole)
	{
		return node->name();
	}
	if (index.column() == 1)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			if (node->state() == Node::Idle)
			{
				return tr("Waiting");
			}
			else if (node->state() == Node::Running)
			{
				const qint64 current = node->progress().first;
				const qint64 total = node->progress().second;
				return tr("%1%").arg(total == 0 ? 0 : (current / total));
			}
			else if (node->state() == Node::Done)
			{
				return tr("Done");
			}
		case ProgressCurrentRole:
			return node->progress().first;
		case ProgressTotalRole:
			return node->progress().second;
		}
	}
	else if (index.column() == 2 && role == Qt::DisplayRole)
	{
		return node->status();
	}
	return QVariant();
}
Qt::ItemFlags TasksModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
QVariant TasksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
		case 0:
			return tr("Name");
		case 1:
			return tr("Progress");
		case 2:
			return tr("Status");
		}
	}
	return QVariant();
}

QModelIndex TasksModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid())
	{
		return createIndex(row, column,
						   static_cast<Node *>(parent.internalPointer())->child(row));
	}
	else
	{
		return createIndex(row, column, m_root->child(row));
	}
}
QModelIndex TasksModel::parent(const QModelIndex &child) const
{
	if (!child.isValid())
	{
		return QModelIndex();
	}

	Node *childNode = static_cast<Node *>(child.internalPointer());
	Node *parentNode = childNode->parent();

	if (!parentNode || parentNode == m_root)
	{
		return QModelIndex();
	}

	return createIndex(parentNode->indexInParent(), 0, parentNode);
}

QModelIndex TasksModel::indexFor(Node *node) const
{
	Node *parentNode = node->parent();

	if (!parentNode || parentNode == m_root)
	{
		return QModelIndex();
	}
	return createIndex(node->indexInParent(), 0, node);
}

void TasksModel::setup(Node *node)
{
	connect(node, &Node::stateChanged, [this, node]()
	{
		const QModelIndex index = indexFor(node);
		emit dataChanged(index.sibling(index.row(), 1), index.sibling(index.row(), 1),
						 QVector<int>() << Qt::DisplayRole);
	});
	connect(node, &Node::statusChanged, [this, node]()
	{
		const QModelIndex index = indexFor(node);
		emit dataChanged(index.sibling(index.row(), 2), index.sibling(index.row(), 2),
						 QVector<int>() << Qt::DisplayRole);
	});
	connect(node, &Node::progressChanged, [this, node]()
	{
		const QModelIndex index = indexFor(node);
		emit dataChanged(index.sibling(index.row(), 1), index.sibling(index.row(), 1),
						 QVector<int>() << Qt::DisplayRole << ProgressCurrentRole
										<< ProgressTotalRole);
	});
}

void TasksModel::addItem(ProgressProvider *item)
{
	m_root->addChild(new Node(this, item));
}
void TasksModel::addItem(NetAction *item)
{
	m_root->addChild(new Node(this, item));
}

void TasksModel::addItemAsChildTo(ProgressProvider *parent, ProgressProvider *item)
{
	if (!m_items.contains(parent))
	{
		addItem(item);
	}
	else
	{
		m_items[parent]->addChild(new Node(this, item));
	}
}

#include "TasksModel.moc"
