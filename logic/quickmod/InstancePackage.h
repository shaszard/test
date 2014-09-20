#pragma once
#include <QString>
#include <QJsonObject>
#include <QList>
#include <memory>

class InstancePackage;

typedef std::shared_ptr<InstancePackage> InstancePackagePtr;

class InstancePackage
{
	friend class InstancePackageList;
	friend class QuickModView;

private: /* variables */
	struct File
	{
		QString path;
		QString sha1;
		// TODO: implement separate mod file meta-cache
		// int64_t last_changed_timestamp;
	};
	QString name;
	QString version;
	bool asDependency = false;
	QString qm_uid;
	QString qm_repo;
	QString qm_updateUrl;
	QString installedPatch;
	QList<File> installedFiles;

private: /* methods */
	static InstancePackagePtr parse(const QJsonObject &valueObject);
	QJsonObject serialize();
};
