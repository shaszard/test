/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QAbstractListModel>

#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "logic/BaseInstance.h"

class QUrl;

class QuickModDatabase;
class Mod;
class SettingsObject;
class OneSixInstance;

class QuickModModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit QuickModModel(QObject *parent = 0);
	~QuickModModel();

	enum Roles
	{
		NameRole = Qt::UserRole,
		UidRole,
		RepoRole,
		DescriptionRole,
		WebsiteRole,
		IconRole,
		LogoRole,
		UpdateRole,
		ReferencesRole,
		DependentUrlsRole,
		ModIdRole,
		CategoriesRole,
		MCVersionsRole,
		TagsRole,
		QuickModRole
	};
	QHash<int, QByteArray> roleNames() const;

	int rowCount(const QModelIndex &) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	QVariant data(const QModelIndex &index, int role) const;

	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
						 const QModelIndex &parent) const;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
					  const QModelIndex &parent);
	Qt::DropActions supportedDropActions() const;
	Qt::DropActions supportedDragActions() const;

signals:
	void error(const QString &message);
	
public slots:
	void modIconUpdated(QuickModRef uid);
	void modLogoUpdated(QuickModRef uid);
	void modAdded(QuickModRef uid);

private:
	/// Gets the index of the given mod in the list.
	int getQMIndex(QuickModMetadataPtr mod) const;

	// list that stays ordered for the model. Updates when the data updates vial signals.
	QList<QuickModRef> m_uids;
};
