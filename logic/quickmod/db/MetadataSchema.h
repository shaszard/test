#pragma once

#include "logic/db/Schema.h"
#include "logic/quickmod/QuickModMetadata.h"

class MetadataSchema : public Schema<QuickModMetadataPtr>
{
public:
	using Schema<QuickModMetadataPtr>::Schema;

	virtual bool contains(const QuickModRef &ref, const QString &repo) const = 0;
};

class MetadataSchema1 : public MetadataSchema
{
public:
	using MetadataSchema::MetadataSchema;

	QList<QuickModMetadataPtr> read(const QString &query) const override;
	bool write(const QuickModMetadataPtr &obj) override;
	int revision() const override { return 1; }
	bool drop() const override;
	bool create() const override;

	bool contains(const QuickModRef &ref, const QString &repo) const override;
	int internalId(const QuickModRef &ref, const QString &repo) const;
	bool givesResult(const QString &query) const;
};
