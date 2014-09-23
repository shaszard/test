#include "ForgeVersion.h"
#include "logic/VersionFilterData.h"
#include <QObject>

QString ForgeVersion::name() const
{
	return "Forge " + jobbuildver;
}

QString ForgeVersion::descriptor() const
{
	return universal_filename;
}

QString ForgeVersion::typeString() const
{
	if (is_recommended)
		return QObject::tr("Recommended");
	return QString();
}

bool ForgeVersion::operator<(BaseVersion &a) const
{
	ForgeVersion *pa = dynamic_cast<ForgeVersion *>(&a);
	if (!pa)
		return true;
	return m_buildnr < pa->m_buildnr;
}

bool ForgeVersion::operator>(BaseVersion &a) const
{
	ForgeVersion *pa = dynamic_cast<ForgeVersion *>(&a);
	if (!pa)
		return false;
	return m_buildnr > pa->m_buildnr;
}

bool ForgeVersion::usesInstaller() const
{
	if(installer_url.isEmpty())
		return false;
	if(g_VersionFilterData.forgeInstallerBlacklist.contains(mcver))
		return false;
	return true;
}

QString ForgeVersion::filename() const
{
	return usesInstaller() ? installer_filename : universal_filename;
}

QString ForgeVersion::url() const
{
	return usesInstaller() ? installer_url : universal_url;
}
