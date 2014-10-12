#pragma once

#include <QString>
#include <QJsonObject>

class QuickModDownload
{
public:
	enum DownloadType
	{
		Invalid = 0,
		Direct = 1,
		Parallel,
		Sequential,
		Encoded
	};

	QString url;
	DownloadType type;
	int priority;
	QString hint;
	QString group;

	QJsonObject toJson() const;
};
