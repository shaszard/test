#pragma once

#include "logic/db/Schema.h"
#include "logic/quickmod/QuickModVersion.h"

class VersionSchema : public Schema<QuickModVersionPtr>
{
public:
	using Schema<QuickModVersionPtr>::Schema;
};

class VersionSchema1 : public VersionSchema
{
public:
	using VersionSchema::VersionSchema;

	QList<QuickModVersionPtr> read(const QString &query) const;
	bool write(const QuickModVersionPtr &obj);
	int revision() const { return 1; }
	bool drop() const;
	bool create() const;
};
