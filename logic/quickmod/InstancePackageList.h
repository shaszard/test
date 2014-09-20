#pragma once
#include <QString>
#include <QStringList>
#include <QJsonValue>
#include <QList>
#include <QJsonDocument>
#include <QAbstractListModel>
#include <memory>

#include "QuickModRef.h"
#include "QuickModVersionRef.h"
#include "InstancePackage.h"
#include "ChangeSource.h"

class Task;
class Transaction;
class OneSixInstance;
class InstancePackage;
class QuickModView;

class InstancePackageList : public QObject
{
	Q_OBJECT
	friend class QuickModView;
	friend class OneSixInstance;

public:
	typedef QList<InstancePackagePtr> storage_type;

private: /* methods */
	InstancePackageList(std::shared_ptr<OneSixInstance> parent);

	bool loadFromFile(QString filename);
	bool saveToFile(QString filename);
	void parse(const QJsonDocument &document);
	QJsonDocument serialize();
	void insert(InstancePackagePtr mod);

public: /* methods */
	virtual ~InstancePackageList(){};

	bool isQuickmodInstalled(const QuickModRef &mod);
	QuickModVersionRef installedQuickModVersion(const QuickModRef &mod);
	bool installedQuickIsHardDep(const QuickModRef &mod);

	/// walk through the list from the start
	std::unique_ptr<QuickModView> iterateQuickMods();

	/// Get or create the current transaction to work with outside of the package list
	std::shared_ptr<Transaction> getTransaction();

	/// Begin to apply the current transaction
	std::shared_ptr<Task> applyCurrentTransaction();

signals:
	void added(ChangeSource source, QString uid);
	void removed(ChangeSource source, QString uid);
	void updated(ChangeSource source, QString uid);

private: /* variables */
	std::weak_ptr<OneSixInstance> parentInstance;
	QList<InstancePackagePtr> installedMods;
	QMap<QString, InstancePackagePtr> installedMods_index;
	std::shared_ptr<Transaction> transaction;
};

class QuickModView
{
public:
	QuickModView(const InstancePackageList::storage_type &storage, int index = 0)
		: m_storage(storage), i(storage.cbegin() + index)
	{
	}
	bool isValid() const
	{
		return i != m_storage.cend();
	}
	QuickModView &next()
	{
		i++;
		return *this;
	}

	QuickModRef uid() const
	{
		return QuickModRef((*i)->qm_uid);
	}
	QuickModVersionRef version() const
	{
		auto mod = (*i);
		return QuickModVersionRef(QuickModRef(mod->qm_uid), mod->version);
	}
	bool asDependency() const
	{
		return (*i)->asDependency;
	}

private:
	// FIXME: what if is InstancePackageList destroyed while we use this?
	const InstancePackageList::storage_type &m_storage;
	InstancePackageList::storage_type::const_iterator i;
};

class Task;