#include "Transaction.h"

Transaction::Action::Action(QString uid, QString repo, QString version,
							Transaction::Action::Type t)
	: uid(uid), targetRepo(repo), targetVersion(version), type(t)
{
}

void Transaction::insert(QString uid, QString version, QString repo)
{
	auto ver = Version(repo, version);
	components.insert(uid, {uid, ver, ver});
}

void Transaction::setComponentVersion(QString uid, QString version, QString repo)
{
	if (components.contains(uid))
	{
		Version newversion(version, repo);
		components[uid].targetVersion = newversion;

		// setting target version to the same as current -> no more action
		if (newversion == components[uid].currentVersion)
		{
			emit actionRemoved(TransactionSource, uid);
			return;
		}

		// otherwise the action changed
		emit actionChanged(TransactionSource, uid);
	}
	else
	{
		components.insert(uid, {uid, Version(), Version(repo, version)});
		emit actionAdded(TransactionSource, uid);
	}
}

void Transaction::removeComponent(QString uid)
{
	// already not there? no change.
	if (!components.contains(uid))
		return;

	auto &component = components[uid];

	// if it wasn't there to start with, we remove the action and component altogether.
	if (!component.currentVersion)
	{
		components.remove(uid);
		emit actionRemoved(TransactionSource, uid);
	}

	// if the target version is null, do nothing
	if (!component.targetVersion)
		return;

	// otherwise we remove the target version
	component.targetVersion.remove();
	emit actionChanged(TransactionSource, uid);
}

QList<Transaction::Action> Transaction::getActions() const
{
	QList<Action> result;
	for (auto &component : components)
	{
		Action a;
		if(component.getActionInternal(a))
		{
			result.append(a);
		}
	}
	return result;
}

bool Transaction::Component::getActionInternal(Transaction::Action &a) const
{
	// component started as missing
	if (!currentVersion)
	{
		// and we are adding a version?
		if (targetVersion)
		{
			a.type = Transaction::Action::Add;
			a.uid = uid;
			a.targetVersion = targetVersion.version;
			a.targetRepo = targetVersion.repo; 
			return true;
		}
	}
	// component started with a version
	else
	{
		// and it no longer has one?
		if (!targetVersion)
		{
			a.type = Transaction::Action::Remove;
			a.uid = uid;
			a.targetVersion = QString();
			a.targetRepo = QString(); 
			return true;
		}
		// or the target version is there and different?
		else if (targetVersion != currentVersion)
		{
			a.type = Transaction::Action::ChangeVersion;
			a.uid = uid;
			a.targetVersion = targetVersion.version;
			a.targetRepo = targetVersion.repo;
			return true;
		}
	}
	return false;
}

bool Transaction::getAction(QString uid, Transaction::Action& action) const
{
	auto component = components.find(uid);
	if(component == components.end())
	{
		return false;
	}
	return component->getActionInternal(action);
}