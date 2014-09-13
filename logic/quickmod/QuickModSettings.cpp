// FIXME: REMOVE.
// FIXME: REMOVE.
// FIXME: REMOVE.

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

#include "QuickModSettings.h"

#include <QMap>
#include <QDir>

#include "logic/settings/INISettingsObject.h"
#include "logic/settings/Setting.h"
#include "QuickModMetadata.h"
#include "QuickModVersionRef.h"
#include "QuickModRef.h"
#include "logic/BaseInstance.h"

// FIXME: REMOVE.
QuickModSettings::QuickModSettings()
	: m_settings(new INISettingsObject(QDir::current().absoluteFilePath("quickmod.cfg")))
{
	m_settings->registerSetting("AvailableMods",
								QVariant::fromValue(QMap<QString, QMap<QString, QString>>()));
	m_settings->registerSetting("TrustedWebsites", QVariantList());
	m_settings->registerSetting("Indices", QVariantMap());
}

// FIXME: REMOVE.
QuickModSettings::~QuickModSettings()
{
	delete m_settings;
}

// FIXME: REMOVE.
void QuickModSettings::markModAsExists(QuickModMetadataPtr mod, const QuickModVersionRef &version,
									   const QString &fileName)
{
	auto mods = m_settings->get("AvailableMods").toMap();
	auto map = mods[mod->internalUid()].toMap();
	map[version.toString()] = fileName;
	mods[mod->internalUid()] = map;
	m_settings->getSetting("AvailableMods")->set(QVariant(mods));
}

// FIXME: REMOVE.
bool QuickModSettings::isModMarkedAsExists(QuickModMetadataPtr mod,
										   const QuickModVersionRef &version) const
{
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods.contains(mod->internalUid()) &&
		   mods.value(mod->internalUid()).toMap().contains(version.toString());
}

// FIXME: REMOVE.
QString QuickModSettings::existingModFile(QuickModMetadataPtr mod,
										  const QuickModVersionRef &version) const
{
	if (!isModMarkedAsExists(mod, version))
	{
		return QString();
	}
	auto mods = m_settings->get("AvailableMods").toMap();
	return mods[mod->internalUid()].toMap()[version.toString()].toString();
}

// I hope this is a clear message :)