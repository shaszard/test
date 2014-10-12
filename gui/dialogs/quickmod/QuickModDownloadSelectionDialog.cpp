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

#include "QuickModDownloadSelectionDialog.h"
#include "ui_QuickModDownloadSelectionDialog.h"

#include "logic/quickmod/QuickModVersion.h"
#include "logic/settings/SettingsObject.h"
#include "MultiMC.h"

QuickModDownloadSelectionDialog::QuickModDownloadSelectionDialog(
	const QuickModVersionPtr version, QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModDownloadSelectionDialog), m_version(version)
{
	ui->setupUi(this);

	bool haveEncoded = false;

	for (const auto download : version->downloads)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(ui->list);
		switch (download.type)
		{
		case QuickModDownload::Direct:
			item->setText(0, tr("Direct"));
		case QuickModDownload::Sequential:
			item->setText(0, tr("Sequential"));
		case QuickModDownload::Parallel:
			item->setText(0, tr("Parallel"));
		case QuickModDownload::Encoded:
			item->setText(0, tr("Encoded"));
		}

		item->setText(1, QString::number(download.priority));
		item->setText(2, download.url);
		if (download.type == QuickModDownload::Encoded)
		{
			item->setText(3, download.hint);
			item->setText(4, download.group);
			haveEncoded = true;
		}
	}

	if (!haveEncoded)
	{
		ui->list->setColumnCount(3);
	}
}

QuickModDownloadSelectionDialog::~QuickModDownloadSelectionDialog()
{
	delete ui;
}

int QuickModDownloadSelectionDialog::selectedIndex() const
{
	return ui->list->currentIndex().row();
}
