#pragma once
#include <QString>
#include <QStringList>
#include <QJsonValue>
#include <QList>
#include <QJsonDocument>
#include <memory>
#include "QuickModRef.h"
#include "QuickModVersionRef.h"

struct InstalledMod;
class OneSixInstance;

class InstalledMod;
class QuickModView;
typedef std::shared_ptr<InstalledMod> InstalledModRef;

class InstalledMod
{
	friend class InstalledMods;
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
	QString qm_updateUrl;
	QString installedPatch;
	QList<File> installedFiles;

private: /* methods */
	static InstalledModRef parse(const QJsonObject &valueObject);
	QJsonObject serialize();
};

class InstalledMods
{
	friend class QuickModView;
public:
	InstalledMods(std::shared_ptr<OneSixInstance> parent);
	virtual ~InstalledMods(){};

	typedef QList<InstalledModRef> storage_type;

	bool loadFromFile(QString filename);
	bool saveToFile(QString filename);

	bool isQuickmodInstalled(const QuickModRef &mod);
	QuickModVersionRef installedQuickModVersion(const QuickModRef &mod);
	bool installedQuickIsHardDep(const QuickModRef& mod);

	std::unique_ptr<QuickModView> iterateQuickMods();

	// from QuickModInstaller
	void install(const QuickModVersionPtr version);

	// from QuickModLibraryInstaller
	bool installLibrariesFrom(const QuickModVersionPtr version);

	// from QuickModSettings
	void markModAsInstalled(const QuickModVersionRef& version, QString dest);
	void markModAsUninstalled(const QuickModRef uid);
	QList< InstalledMod::File > installedModFiles(const QuickModRef uid) const;
	bool isModMarkedAsInstalled(const QuickModRef uid) const;

	// from OneSixInstance
	void setQuickModVersion(const QuickModRef &uid, const QuickModVersionRef &version, const bool manualInstall = false);
	void setQuickModVersions(const QMap<QuickModRef, QPair<QuickModVersionRef, bool> > &mods);
	void removeQuickMod(const QuickModRef &uid);
	void removeQuickMods(const QList<QuickModRef> &uids);

private: /* variables */
	std::weak_ptr<OneSixInstance> parentInstance;

	QList<InstalledModRef> installedMods;
	QMap<QString, InstalledModRef> installedMods_index;

private: /* methods */
	void parse(const QJsonDocument &document);
	QJsonDocument serialize();

	void insert(InstalledModRef mod);
};

class QuickModView
{
public:
	QuickModView(const InstalledMods::storage_type &storage, int index = 0)
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
	// FIXME: what if is InstalledMods destroyed while we use this?
	const InstalledMods::storage_type &m_storage;
	InstalledMods::storage_type::const_iterator i;
};
