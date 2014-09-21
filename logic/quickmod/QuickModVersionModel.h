#pragma once

#include "QuickModMetadata.h"
#include "logic/BaseVersionList.h"
#include "logic/BaseInstance.h"

class QuickModVersionModel : public BaseVersionList
{
	Q_OBJECT
public:
	explicit QuickModVersionModel(QuickModRef mod, QString mcVersion, QObject *parent = 0);

	Task *getLoadTask();
	bool isLoaded();
	const BaseVersionPtr at(int i) const;
	int count() const;
	void sort()
	{
	}

protected slots:
	void updateListData(QList<BaseVersionPtr> versions)
	{
	}

private:
	QuickModRef m_mod;
	QString m_mcVersion;
	QList<QuickModVersionRef> m_versions;
};
