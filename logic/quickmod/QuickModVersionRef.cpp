#include "QuickModVersion.h"

#include <QString>

#include "QuickModVersionRef.h"
#include "QuickModDatabase.h"
#include "MultiMC.h"

QuickModVersionRef::QuickModVersionRef(const QuickModRef &mod, const Util::Version &id)
	: m_mod(mod), m_id(id)
{
}

QuickModVersionRef::QuickModVersionRef(const QuickModVersionPtr &ptr)
	: QuickModVersionRef(ptr->version())
{
}

//BEGIN dereferencing to actual mod version
QString QuickModVersionRef::userFacing() const
{
	const QuickModVersionPtr ptr = MMC->qmdb()->version(*this);
	return ptr ? ptr->name() : QString();
}

bool QuickModVersionRef::isValid() const
{
	const QuickModVersionPtr ptr = MMC->qmdb()->version(*this);
	return ptr.get() != nullptr;
}
//END

//BEGIN comparison and hash functions
bool QuickModVersionRef::operator<(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_id < other.m_id;
}

bool QuickModVersionRef::operator<=(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_id <= other.m_id;
}

bool QuickModVersionRef::operator>(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_id > other.m_id;
}

bool QuickModVersionRef::operator>=(const QuickModVersionRef &other) const
{
	Q_ASSERT(m_mod == other.m_mod);
	return m_id >= other.m_id;
}

bool QuickModVersionRef::operator==(const QuickModVersionRef &other) const
{
	return m_mod == other.m_mod && m_id == other.m_id;
}

uint qHash(const QuickModVersionRef &ref)
{
	return qHash(ref.mod().toString() + ref.toString());
}
//END