#pragma once
#include <memory>
#include <QObject>
#include <QString>
#include <QMap>
#include "ChangeSource.h"

class InstancePackageList;

class Transaction : public QObject
{
	friend class InstancePackageList;
	friend class QuickModDependencyResolver;
	Q_OBJECT

public:
	/// You can get one from InstancePackageList
	Transaction(){}
	
private:
	/// initial insert, for construction
	void insert(QString uid, QString version, QString repo);

public: /* data types */
	struct Action
	{
		QString uid;
		QString targetRepo;
		QString targetVersion;
		enum Type
		{
			Invalid,
			Add,
			ChangeVersion,
			Remove
		} type = Invalid;
		Action(QString uid, QString repo, QString version, Type t);
		Action() {}
	};

	struct Version
	{
		Version()
		{
		}
		Version(QString nRepo, QString nVersion) : repo(nRepo), version(nVersion), absent(false)
		{
		}
		bool operator==(const Version &rhs) const
		{
			return repo == rhs.repo && version == rhs.version && absent == rhs.absent;
		}
		bool operator!=(const Version & rhs) const
		{
			return !operator==(rhs);
		}
		bool isPresent() const
		{
			return !absent;
		}
		void remove()
		{
			absent = true;
		}

		QString repo;
		QString version;
		bool absent = true;
	};

	struct Component
	{
		QString uid;
		Version currentVersion;
		Version targetVersion;
		bool getActionInternal(Action &a) const;
	};

public: /* methods */
	/// return the list of action this transaction results in
	QList<Transaction::Action> getActions() const;

	/// get the action for one component.
	bool getAction(QString uid, Transaction::Action& action) const;

	/// make the transaction change the version of a component
	void setComponentVersion(QString uid, QString version, QString repo);
	
	/// reset the component version to original
	void resetComponentVersion(QString uid);

	/// reset the whole transaction
	void reset();

	/// make the transaction remove a component
	void removeComponent(QString uid);

signals:
	/// action for uid has changed
	void actionChanged(ChangeSource source, QString uid);

	/// a new action has been added for uid
	void actionAdded(ChangeSource source, QString uid);

	/// an action has been removed for uid
	void actionRemoved(ChangeSource source, QString uid);

private: /* data */
	QMap<QString, Component> components;
};
