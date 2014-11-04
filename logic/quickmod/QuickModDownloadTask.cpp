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

#include "gui/dialogs/quickmod/QuickModInstallDialog.h"
#include "logic/quickmod/InstancePackageList.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

QuickModDownloadTask::QuickModDownloadTask(std::shared_ptr<OneSixInstance> instance,
										   Bindable *parent)
	: Task(parent), m_instance(instance)
{
}
void QuickModDownloadTask::executeTask()
{
	QuickModInstallDialog dlg(m_instance->installedPackages()->getTransaction());
	if (dlg.exec() == QDialog::Accepted)
	{
		m_instance->installedPackages()->transactionApplied();
		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Dialog closed prematurely"));
	}
}


