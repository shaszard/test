#pragma once
#include <memory>
#include <QObject>
#include <QString>
#include <QMap>

class InstanceModManager;
struct TransactionPrivate;

class Transaction : public QObject
{
	friend class InstanceModManager;
	friend class QuickModDependencyResolver;

private:
	/// You can get one from InstanceModManager
	Transaction(){};

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
	};

	struct Version
	{
		Version()
		{
		}
		Version(QString nRepo, QString nVersion) : repo(nRepo), version(nVersion), absent(false)
		{
		}
		bool operator==(const Version &rhs)
		{
			return repo == rhs.repo && version == rhs.version && absent == rhs.absent;
		}
		operator bool() const
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
	};

public: /* methods */
	/// return the list of action this transaction results in
	QList<Transaction::Action> getActions() const;

	/// make the transaction change the version of a component
	void setComponentVersion(QString uid, QString version, QString repo);

	/// make the transaction remove a component
	void removeComponent(QString uid);

signals:
	/// action for uid has changed
	void actionChanged(QString uid);

	/// a new action has been added for uid
	void actionAdded(QString uid);

	/// an action has been removed for uid
	void actionRemoved(QString uid);

private: /* data */
	QMap<QString, Component> components;
};
