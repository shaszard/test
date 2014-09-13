#pragma once

#include "logic/db/Schema.h"
#include "logic/quickmod/QuickModMetadata.h"

struct Checksum
{
	QUrl url;
	QByteArray checksum;
};

class ChecksumSchema : public Schema<Checksum>
{
public:
	using Schema<Checksum>::Schema;

	virtual QByteArray checksum(const QUrl &url) const = 0;
	virtual void setChecksum(const QUrl &url, const QByteArray &checksum) = 0;
};

class ChecksumSchema1 : public ChecksumSchema
{
public:
	using ChecksumSchema::ChecksumSchema;

	QByteArray checksum(const QUrl &url) const override;
	void setChecksum(const QUrl &url, const QByteArray &checksum) override;

	QList<Checksum> read(const QString &query) const override;
	bool write(const Checksum &obj) override;
	int revision() const override { return 1; }
	bool drop() const override;
	bool create() const override;
};
