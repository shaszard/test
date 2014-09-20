#include "logic/quickmod/InstancePackage.h"
#include "logic/MMCJson.h"

//BEGIN: Serialization/Deserialization
std::shared_ptr<InstancePackage> InstancePackage::parse(const QJsonObject &valueObject)
{
	using namespace MMCJson;

	std::shared_ptr<InstancePackage> mod(new InstancePackage);
	mod->name = ensureString(valueObject.value("name"));
	mod->version = ensureString(valueObject.value("version"));
	mod->qm_uid = ensureString(valueObject.value("qm_uid"));
	mod->qm_repo = ensureString(valueObject.value("qm_repo"));
	mod->qm_updateUrl = ensureString(valueObject.value("qm_updateUrl"));
	mod->asDependency = valueObject.value("asDependency").toBool();
	mod->installedPatch = valueObject.value("installedPatch").toString();
	auto installedFilesJson = valueObject.value("installedFiles");

	// optional file array. only installed mods have files.
	auto array = installedFilesJson.toArray();
	for(auto item: array)
	{
		File f;
		auto fileObject = ensureObject(item);
		f.path = ensureString(fileObject.value("path"));
		f.sha1 = ensureString(fileObject.value("sha1"));
		// f.last_changed_timestamp = ensureDouble(fileObject.value("last_changed_timestamp"));
		mod->installedFiles.push_back(f);
	}
	return mod;
}

QJsonObject InstancePackage::serialize()
{
	QJsonObject obj;

	obj.insert("qm_uid", qm_uid);
	obj.insert("qm_repo", qm_repo);
	obj.insert("name", name);
	obj.insert("version", version);
	obj.insert("qm_updateUrl", qm_updateUrl);

	if(asDependency)
		obj.insert("asDependency", QJsonValue(true));

	if(!installedPatch.isEmpty())
		obj.insert("installedPatch", installedPatch);

	QJsonArray fileArray;
	for(auto & file: installedFiles)
	{
		QJsonObject fileObj;
		fileObj.insert("path", file.path);
		fileObj.insert("sha1", file.sha1);
		// fileObj.insert("last_changed_timestamp", QJsonValue( double(file.last_changed_timestamp)));
		fileArray.append(fileObj);
	}
	if(fileArray.size())
		obj.insert("installedFiles", fileArray);

	return obj;
}
//END: Serialization/Deserialization