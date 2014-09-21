#pragma once
#include "QuickModMetadata.h"
#include <modutils.h>
class QuickModVersion;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;
class QuickModVersionRef
{
	QuickModRef m_mod;
	Util::Version m_id;

public:
	explicit QuickModVersionRef(const QuickModRef &mod, const Util::Version &id);
	QuickModVersionRef()
	{
	}
	QuickModVersionRef(const QuickModVersionPtr &ptr);

	bool isValid() const;
	QString userFacing() const;

	bool operator<(const QuickModVersionRef &other) const;
	bool operator<=(const QuickModVersionRef &other) const;
	bool operator>(const QuickModVersionRef &other) const;
	bool operator>=(const QuickModVersionRef &other) const;
	bool operator==(const QuickModVersionRef &other) const;

	QString toString() const
	{
		return m_id.toString();
	}
	QuickModRef mod() const
	{
		return m_mod;
	}
};
uint qHash(const QuickModVersionRef &ref);

Q_DECLARE_METATYPE(QuickModVersionRef)
