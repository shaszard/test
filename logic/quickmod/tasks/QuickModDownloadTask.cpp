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

#include "QuickModDownloadTask.h"

#include "logic/net/ByteArrayDownload.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickModDependencyResolver.h"
#include <logic/quickmod/InstalledMod.h>
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

QuickModDownloadTask::QuickModDownloadTask(std::shared_ptr<OneSixInstance> instance,
										   Bindable *parent)
	: Task(parent), m_instance(instance)
{
}

void QuickModDownloadTask::executeTask()
{
	const bool hasResolveError = QuickModDependencyResolver(m_instance).hasResolveError();

	auto modManager = m_instance->installedMods();
	auto iter = modManager->iterateQuickMods();
	QList<QuickModRef> mods;
	while (iter->isValid())
	{
		auto version = iter->version();
		QuickModVersionPtr ptr = MMC->qmdb()->version(version);

		// FIXME: unify the refs and don't do this silly stuff.
		// FIXME: this is not respecting user preferences... taking first mod and running with
		// it.
		QuickModMetadataPtr mod;
		if (version.isValid())
			mod = MMC->qmdb()->someModMetadata(version.mod());
		else
			mod = MMC->qmdb()->allModMetadata(iter->uid()).first();

		if(!mod)
		{
			// we don't have a mod for that in the db... no updating.
			iter->next();
			continue;
		}

		bool processMod = false;
		processMod |= !ptr;
		processMod |= !MMC->qmdb()->isModMarkedAsExists(mod, version);
		processMod |= hasResolveError;
		processMod |= ptr->needsDeploy() && !modManager->isQuickmodInstalled(mod->uid());
		if (processMod)
		{
			mods.append(mod->uid());
		}
		iter->next();
	}

	if (mods.isEmpty())
	{
		emitSucceeded();
		return;
	}

	bool ok = false;
	QList<QuickModVersionPtr> installedVersions =
		wait<QList<QuickModVersionPtr>>("QuickMods.InstallMods", m_instance, mods, &ok);
	if (ok)
	{
		auto inst_mods = m_instance->installedMods();
		QMap<QuickModRef, QPair<QuickModVersionRef, bool>> mods;
		for (const auto version : installedVersions)
		{
			const auto uid = version->mod->uid();
			bool isManualInstall = inst_mods->installedQuickIsHardDep(uid);
			mods.insert(version->mod->uid(), qMakePair(version->version(), isManualInstall));
		}
		try
		{
			m_instance->installedMods()->setQuickModVersions(mods);
		}
		catch (...)
		{
			QLOG_ERROR() << "This means that stuff will be downloaded on every instance launch";
		}
		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Failure downloading QuickMods"));
	}
}


