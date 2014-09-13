#include "MetadataSchema.h"

#include "logic/quickmod/QuickModBuilder.h"

bool MetadataSchema1::drop() const
{
	m_db->execute("DROP TABLE metadata");
	m_db->execute("DROP TABLE metadata_urls");
	m_db->execute("DROP TABLE metadata_authors");
	m_db->execute("DROP TABLE metadata_tags");
	m_db->execute("DROP TABLE metadata_categories");
	m_db->execute("DROP TABLE metadata_references");
	return true;
}
bool MetadataSchema1::create() const
{
	m_db->execute("PRAGMA foreign_keys = ON");
	m_db->execute("CREATE TABLE metadata (id INTEGER PRIMARY KEY NOT NULL, uid VARCHAR(128) NOT "
				  "NULL, repo VARCHAR(128) NOT "
				  "NULL, name VARCHAR(128) NOT NULL, updateUrl VARCHAR(128) NOT NULL, modId "
				  "VARCHAR(128), description TEXT, license VARCHAR(128))");
	m_db->execute("CREATE TABLE metadata_urls (metadata INT NOT NULL, type VARCHAR(32) NOT "
				  "NULL, url VARCHAR(128) NOT NULL, FOREIGN KEY(metadata) REFERENCES "
				  "metadata(id))");
	m_db->execute("CREATE TABLE metadata_authors (metadata INT NOT NULL, role VARCHAR(32) NOT "
				  "NULL, name VARCHAR(128) NOT NULL, FOREIGN KEY(metadata) REFERENCES "
				  "metadata(id))");
	m_db->execute("CREATE TABLE metadata_tags (metadata INT NOT NULL, tag VARCHAR(64) NOT "
				  "NULL, FOREIGN KEY(metadata) REFERENCES metadata(id))");
	m_db->execute("CREATE TABLE metadata_categories (metadata INT NOT NULL, category "
				  "VARCHAR(64) NOT NULL, FOREIGN KEY(metadata) REFERENCES metadata(id))");
	m_db->execute("CREATE TABLE metadata_references (metadata INT NOT NULL, uid VARCHAR(128) "
				  "NOT NULL, url VARCHAR(128) NOT NULL, FOREIGN KEY(metadata) REFERENCES "
				  "metadata(id))");
	return true;
}

bool MetadataSchema1::contains(const QuickModRef &ref, const QString &repo) const
{
	return internalId(ref, repo) != -1;
}
int MetadataSchema1::internalId(const QuickModRef &ref, const QString &repo) const
{
	QVariantList list = m_db->executeWithReturn("SELECT id FROM metadata WHERE uid = " +
												escape(ref.toString()) + " AND repo = " + escape(repo));
	if (list.isEmpty())
	{
		return -1;
	}
	return list.first().toInt();
}
bool MetadataSchema1::givesResult(const QString &query) const
{
	return !m_db->executeWithReturn(query).isEmpty();
}

QList<QuickModMetadataPtr> MetadataSchema1::read(const QString &query) const
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

	QList<QuickModMetadataPtr> out;
	const auto rows = m_db->all("SELECT id,uid,repo,name,updateUrl,modId,description,license FROM metadata" + where);
	qDebug() << rows.size();
	for (const QVariantList row : rows)
	{
		QuickModBuilder builder;
		builder = builder.setUid(row[1].toString())
					  .setRepo(row[2].toString())
					  .setName(row[3].toString())
					  .setUpdateUrl(QUrl(row[4].toString()))
					  .setModId(row[5].toString())
					  .setDescription(row[6].toString())
					  .setLicense(row[7].toString());
		int metadataId = row[0].toInt();

		// urls
		{
			for (const auto urlsRow :
				 m_db->all("SELECT type,url FROM metadata_urls WHERE metadata = " +
						   QString::number(metadataId)))
			{
				builder.addUrl(urlsRow[0].toString(), QUrl(urlsRow[1].toString()));
			}
		}
		// authors
		{
			for (const auto authorsRow :
				 m_db->all("SELECT role,name FROM metadata_authors WHERE metadata = " +
						   QString::number(metadataId)))
			{
				builder.addAuthor(authorsRow[0].toString(), authorsRow[1].toString());
			}
		}
		// tags
		{
			QStringList tags;
			for (const auto tagsRow :
				 m_db->all("SELECT tag FROM metadata_tags WHERE metadata = " +
						   QString::number(metadataId)))
			{
				tags += tagsRow[0].toString();
			}
			builder.setTags(tags);
		}
		// categories
		{
			QStringList categories;
			for (const auto categoriesRow :
				 m_db->all("SELECT category FROM metadata_categories WHERE metadata = " +
						   QString::number(metadataId)))
			{
				categories += categoriesRow[0].toString();
			}
			builder.setCategories(categories);
		}
		// references
		{
			for (const auto referencesRow :
				 m_db->all("SELECT uid,url FROM metadata_references WHERE metadata = " +
						   QString::number(metadataId)))
			{
				builder.addReference(QuickModRef(referencesRow[0].toString()),
									 QUrl(referencesRow[1].toString()));
			}
		}

		out.append(builder.buildPtr());
	}
	return out;
}

bool MetadataSchema1::write(const QuickModMetadataPtr &obj)
{
	int id = internalId(obj->uid(), obj->repo());

	if (id == -1)
	{
		SQLiteStatement *stmt = m_db->prepare("INSERT INTO metadata "
											  "(uid,repo,name,updateUrl,modId,description,"
											  "license) VALUES (?,?,?,?,?,?,?)");
		m_db->bindAndExec(stmt, (obj->uid().toString()), (obj->repo()),
						  (obj->name()), (obj->updateUrl().toString()),
						  (obj->modId()), (obj->description()),
						  (obj->license()));
		id = m_db->lastInsertId();
	}
	else
	{
		SQLiteStatement *stmt =
			m_db->prepare("UPDATE metadata SET name = ?, updateUrl = ?, modId = ?, description "
						  "= ?, license = ? WHERE uid = ? AND repo = ?");
		m_db->bindAndExec(stmt, obj->name(), obj->updateUrl().toString(), obj->modId(),
						  obj->description(), obj->license(), obj->uid().toString(),
						  obj->repo());
	}

	const QString stringId = QString::number(id);

	for (const auto tag : obj->tags())
	{
		if (!givesResult("SELECT * FROM metadata_tags WHERE metadata = " + stringId +
						 " AND tag = " + escape(tag)))
		{
			m_db->execute("INSERT INTO metadata_tags (metadata,tag) VALUES (" + stringId + "," +
						  escape(tag) + ")");
		}
	}
	for (const auto category : obj->categories())
	{
		if (!givesResult("SELECT * FROM metadata_categories WHERE metadata = " + stringId +
						 " AND category = " + escape(category)))
		{
			m_db->execute("INSERT INTO metadata_categories (metadata,category) VALUES (" +
						  stringId + "," + escape(category) + ")");
		}
	}
	for (const auto role : obj->authorTypes())
	{
		for (const auto name : obj->authors(role))
		{
			if (!givesResult("SELECT * FROM metadata_authors WHERE metadata = " + stringId +
							 " AND role = " + escape(role) + " AND name = " + escape(name)))
			{
				m_db->execute("INSERT INTO metadata_authors (metadata,role,name) VALUES (" +
							  stringId + "," + escape(role) + "," + escape(name) + ")");
			}
		}
	}
	for (const auto type : obj->urlTypes())
	{
		for (const auto url : obj->url(type))
		{
			if (!givesResult("SELECT * FROM metadata_urls WHERE metadata = " + stringId +
							 " AND type = " + escape(QuickModMetadata::urlId(type)) +
							 " AND url = " + escape(url.toString())))
			{
				m_db->execute("INSERT INTO metadata_urls (metadata,type,url) VALUES (" +
							  stringId + "," + escape(QuickModMetadata::urlId(type)) + "," +
							  escape(url.toString()) + ")");
			}
		}
	}
	for (auto it = obj->references().constBegin(); it != obj->references().constEnd(); ++it)
	{
		const QString escapedUid = escape(it.key().toString());
		const QString escapedUrl = escape(it.value().toString());
		if (givesResult("SELECT * FROM metadata_references WHERE metadata = " + stringId + " AND uid = " + escapedUid))
		{
			m_db->execute("UPDATE metadata_references SET url = " + escapedUrl + " WHERE metadata = " + stringId + " AND uid = " + escapedUid);
		}
		else
		{
			m_db->execute("INSERT INTO metadata_references (metadata,uid,url) VALUES (" + stringId + "," + escapedUid + "," + escapedUrl + ")");
		}
	}

	return true;
}
