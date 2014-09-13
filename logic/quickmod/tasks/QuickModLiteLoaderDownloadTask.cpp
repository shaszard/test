/* Copyright 2014 MultiMC Contributors
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

#include "QuickModLiteLoaderDownloadTask.h"

#include "logic/liteloader/LiteLoaderInstaller.h"
#include "logic/liteloader/LiteLoaderVersionList.h"
#include "logic/quickmod/QuickModsList.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/InstalledMod.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

QuickModLiteLoaderDownloadTask::QuickModLiteLoaderDownloadTask(std::shared_ptr<OneSixInstance> instance, Bindable *parent)
	: Task(parent), m_instance(instance)
{
}

void QuickModLiteLoaderDownloadTask::executeTask()
{
	auto iter = m_instance->installedMods()->iterateQuickMods();
	if (!iter->isValid())
	{
		emitSucceeded();
		return;
	}

	LiteLoaderInstaller *installer = new LiteLoaderInstaller;
	if (QDir(m_instance->instanceRoot()).exists("patches/" + installer->id() + ".json"))
	{
		emitSucceeded();
		return;
	}

	QStringList versionFilters;
	while (iter->isValid())
	{
		QuickModVersionPtr version = MMC->qmdb()->version(iter->version());
		if (!version)
		{
			iter->next();
			continue;
		}
		if (!version->liteloaderVersionFilter.isEmpty())
		{
			versionFilters.append(version->liteloaderVersionFilter);
		}
		iter->next();
	}
	if (versionFilters.isEmpty())
	{
		emitSucceeded();
		return;
	}

	LiteLoaderVersionPtr liteloaderVersion = wait<LiteLoaderVersionPtr>(
		"QuickMods.GetLiteLoaderVersion", m_instance, versionFilters);
	if (liteloaderVersion)
	{
		auto task = installer->createInstallTask(m_instance.get(), liteloaderVersion, this);
		connect(task, &Task::progress, [this](qint64 current, qint64 total)
				{
			setProgress(100 * current / qMax((qint64)1, total));
		});
		connect(task, &Task::status, [this](const QString &msg)
				{
			setStatus(msg);
		});
		connect(task, &Task::failed, [this, installer](const QString &reason)
				{
			delete installer;
			emitFailed(reason);
		});
		connect(task, &Task::succeeded, [this, installer]()
				{
			delete installer;
			emitSucceeded();
		});
		task->start();
	}
	else
	{
		emitFailed(tr("No LiteLoader version selected"));
	}
}
