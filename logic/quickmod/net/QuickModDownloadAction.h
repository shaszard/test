#pragma once

#include "logic/quickmod/net/QuickModBaseDownloadAction.h"

class QuickModMetadata;
class QuickModVersion;

typedef std::shared_ptr<QuickModMetadata> QuickModMetadataPtr;
typedef std::shared_ptr<QuickModVersion> QuickModVersionPtr;

class QuickModDownloadAction : public QuickModBaseDownloadAction
{
	Q_OBJECT
public:
	explicit QuickModDownloadAction(const QUrl &url, const QString &expectedUid);

public:
	QString m_expectedUid;

	bool m_autoAdd = true;
	QuickModMetadataPtr m_resultMetadata;
	QList<QuickModVersionPtr> m_resultVersions;

	void add();

private:
	bool handle(const QByteArray &data) override;
};
