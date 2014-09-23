#pragma once
#include <QString>
#include <memory>
#include "logic/BaseVersion.h"

struct ForgeVersion;
typedef std::shared_ptr<ForgeVersion> ForgeVersionPtr;

struct ForgeVersion : public BaseVersion
{
	virtual QString descriptor() const override;
	virtual QString name() const override;
	virtual QString typeString() const override;
	virtual bool operator<(BaseVersion &a) const override;
	virtual bool operator>(BaseVersion& a) const override;

	QString filename() const;
	QString url() const;
	
	enum
	{
		Invalid,
		Legacy,
		Gradle
	} type = Invalid;
	
	bool usesInstaller() const;

	int m_buildnr = 0;
	QString branch;
	QString universal_url;
	QString changelog_url;
	QString installer_url;
	QString jobbuildver;
	QString mcver;
	QString mcver_sane;
	QString universal_filename;
	QString installer_filename;
	bool is_recommended = false;
};

Q_DECLARE_METATYPE(ForgeVersionPtr)
