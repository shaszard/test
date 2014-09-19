#pragma once
#include <QString>
#include <QStringList>
#include <QJsonValue>
#include <QList>
#include <QJsonDocument>
#include <memory>
#include "QuickModRef.h"
#include "QuickModVersionRef.h"

class Task;
class Transaction;
struct InstalledMod;
class OneSixInstance;
class InstalledMod;
class QuickModView;

typedef std::shared_ptr<InstalledMod> InstalledModRef;

class InstalledMod
{
	friend class InstancePackageList;
	friend class QuickModView;

private: /* variables */
	struct File
	{
		QString path;
		QString sha1;
		// TODO: implement separate mod file meta-cache
		// int64_t last_changed_timestamp;
	};
	QString name;
	QString version;
	bool asDependency = false;
	QString qm_uid;
	QString qm_repo;
	QString qm_updateUrl;
	QString installedPatch;
	QList<File> installedFiles;

private: /* methods */
	static InstalledModRef parse(const QJsonObject &valueObject);
	QJsonObject serialize();
};

class InstancePackageList
{
	friend class QuickModView;
	friend class OneSixInstance;
public:
	typedef QList<InstalledModRef> storage_type;

private: /* methods */
	InstancePackageList(std::shared_ptr<OneSixInstance> parent);
	
	bool loadFromFile(QString filename);
	bool saveToFile(QString filename);

public: /* methods */
	virtual ~InstancePackageList(){};

	bool isQuickmodInstalled(const QuickModRef &mod);
	QuickModVersionRef installedQuickModVersion(const QuickModRef &mod);
	bool installedQuickIsHardDep(const QuickModRef &mod);

	std::unique_ptr<QuickModView> iterateQuickMods();

	/// Get or create the current transaction to work with outside of the package list
	std::shared_ptr<Transaction> getTransaction();

	/// Throw away the current transaction
	void abortCurrentTransaction();

	/// Begin to apply the current transaction
	std::shared_ptr<Task> applyCurrentTransaction();

	// from QuickModInstaller
	// void install(const QuickModVersionPtr version);

	// from QuickModLibraryInstaller
	// bool installLibrariesFrom(const QuickModVersionPtr version);

	// from QuickModSettings
	// void markModAsInstalled(const QuickModVersionRef &version, QString dest);
	// void markModAsUninstalled(const QuickModRef uid);
	// QList<InstalledMod::File> installedModFiles(const QuickModRef uid) const;
	// bool isModMarkedAsInstalled(const QuickModRef uid) const;

private: /* variables */
	std::weak_ptr<OneSixInstance> parentInstance;

	QList<InstalledModRef> installedMods;
	QMap<QString, InstalledModRef> installedMods_index;
	std::shared_ptr<Transaction> transaction;

private: /* methods */
	void parse(const QJsonDocument &document);
	QJsonDocument serialize();

	void insert(InstalledModRef mod);
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