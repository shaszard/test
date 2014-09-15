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
			emit actionRemoved(uid);
			return;
		}

		// otherwise the action changed
		emit actionChanged(uid);
	}
	else
	{
		components.insert(uid, {uid, Version(), Version(repo, version)});
		emit actionAdded(uid);
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
		emit actionRemoved(uid);
	}

	// if the target version is null, do nothing
	if (!component.targetVersion)
		return;

	// otherwise we remove the target version
	component.targetVersion.remove();
	emit actionChanged(uid);
}

QList<Transaction::Action> Transaction::getActions() const
{
	QList<Action> result;
	for (auto &component : components)
	{
		auto &currentVersion = component.currentVersion;
		auto &targetVersion = component.targetVersion;
		auto &uid = component.uid;

		// component started as missing
		if (!currentVersion)
		{
			// and we are adding a version?
			if (targetVersion)
			{
				result.append(Action(uid, targetVersion.repo, targetVersion.version,
									 Transaction::Action::Add));
			}
		}
		// component started with a version
		else
		{
			// and it no longer has one?
			if (!targetVersion)
			{
				result.append(Action(uid, QString(), QString(), Transaction::Action::Remove));
			}
			// or the target version is there and different?
			else if (targetVersion != currentVersion)
			{
				result.append(Action(uid, targetVersion.repo, targetVersion.version,
									 Transaction::Action::ChangeVersion));
			}
		}
	}
	return result;
}
