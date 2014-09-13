#include "ChecksumSchema.h"

QByteArray ChecksumSchema1::checksum(const QUrl &url) const
{
	const auto all = read("url = " + escape(url.toString()));
	if (all.isEmpty())
	{
		return QByteArray();
	}
	else
	{
		return all.first().checksum;
	}
}
void ChecksumSchema1::setChecksum(const QUrl &url, const QByteArray &checksum)
{
	write(Checksum{url, checksum});
}

QList<Checksum> ChecksumSchema1::read(const QString &query) const
{
	QString where;
	if (query.isEmpty())
	{
		where = "";
	}
	else
	{
		where = " WHERE " + query;
	}

	QList<Checksum> out;

	const auto rows = m_db->all("SELECT url,checksum FROM checksums" + where);
	for (const QVariantList row : rows)
	{
		out.append(Checksum{QUrl(row[0].toString()), row[1].toString().toLatin1()});
	}
	return out;
}
bool ChecksumSchema1::write(const Checksum &obj)
{
	m_db->execute("DELETE FROM checksums WHERE url = " + escape(obj.url.toString()));
	m_db->buildInsert().into("checksums").columns("url", "checksum").values(escape(obj.url.toString()) + "," + escape(QString::fromLatin1(obj.checksum))).execute();
	return true;
}

bool ChecksumSchema1::drop() const
{
	return m_db->execute("DROP TABLE checksums");
}
bool ChecksumSchema1::create() const
{
	return m_db->execute("CREATE TABLE checksums (url VARCHAR(128) NOT NULL, checksum VARCHAR(128) NOT NULL)");
}
