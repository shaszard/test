#include "QuickModVersionModel.h"
#include "QuickModVersionRef.h"
#include "QuickModVersion.h"
#include "QuickModDatabase.h"
#include "logger/QsLog.h"
#include "MultiMC.h"

QuickModVersionModel::QuickModVersionModel(QuickModRef mod, InstancePtr instance, QObject *parent)
	: BaseVersionList(parent), m_mod(mod), m_instance(instance)
{
	m_versions = MMC->qmdb()->versions(mod, m_instance->intendedVersionId());
}

Task *QuickModVersionModel::getLoadTask()
{
	return 0;
}
bool QuickModVersionModel::isLoaded()
{
	return true;
}

const BaseVersionPtr QuickModVersionModel::at(int i) const
{
	if(i < m_versions.size() && i >= 0)
	{
		return MMC->qmdb()->version(m_versions[i]);
	}

	QLOG_WARN() << "QuickModVersionModel::at -> Index out of range (" << i << ")";
	return QuickModVersionPtr();
}

int QuickModVersionModel::count() const
{
	return m_versions.count();
}
