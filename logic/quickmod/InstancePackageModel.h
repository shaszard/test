#pragma once
#include <QAbstractListModel>
#include <QString>
#include <memory>
#include "ChangeSource.h"
class InstancePackageList;

class InstancePackageModel : public QAbstractListModel
{
	Q_OBJECT
public:
	InstancePackageModel(std::shared_ptr<InstancePackageList> list);

private:
	void populate();

public: /* model interface */
	/// return the number of rows of this model
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

	/// return data for a particular index
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

	/// return column header data for a column
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	/// set data. used for checkboxes.
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

	/// return the number of columns of this model (COLUMN_COUNT)
	virtual int columnCount(const QModelIndex& parent) const;

	/// get the flags for a particular model index
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;

private: /* model interface, internal */
	/// data for the 'enabled' checkbox column
	QVariant enabledData(int row, int role = Qt::DisplayRole) const;

	/// data for the name column
	QVariant nameData(int row, int role = Qt::DisplayRole) const;

	/// data for the version column
	QVariant versionData(int row, int role = Qt::DisplayRole) const;

	/// data for the new version column
	QVariant newVersionData(int row, int role = Qt::DisplayRole) const;

private: /* internal types */
	enum Column
	{
		EnabledColumn,
		NameColumn,
		VersionColumn,
		NewVersionColumn,
		COLUMN_COUNT
	};

	struct TrackedItem
	{
		QString uid;
		bool instance = false;
		bool transaction = false;
	};

private slots: /* handled events */
	/// a source has added a component identified by uid
	void added(ChangeSource source, QString uid);

	/// a source has removed a component identified by uid
	void removed(ChangeSource source, QString uid);

	/// a source has changed the information of a component identified by uid
	void updated(ChangeSource source, QString uid);

private: /* data */
	QList<QString> m_order;
	QMap<QString, TrackedItem> m_tracked;
	std::shared_ptr<InstancePackageList> m_list;
};
