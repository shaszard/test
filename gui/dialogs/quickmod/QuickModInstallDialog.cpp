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

#include "QuickModInstallDialog.h"
#include "ui_QuickModInstallDialog.h"

#include <QTreeWidgetItem>
#include <QNetworkReply>
#include <QStyledItemDelegate>
#include <QProgressBar>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QtMath>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QMenu>
#include <QAction>
#include <pathutils.h>

#include "gui/dialogs/quickmod/QuickModVerifyModsDialog.h"
#include "gui/GuiUtil.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModVersionModel.h"
#include "logic/quickmod/InstancePackageList.h"
#include "logic/quickmod/TransactionModel.h"

#include "logic/OneSixInstance.h"

#include "MultiMC.h"

class ProgressItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	ProgressItemDelegate(QObject *parent = 0) : QStyledItemDelegate(parent)
	{
	}
	void paint(QPainter *painter, const QStyleOptionViewItem &opt,
			   const QModelIndex &index) const
	{
		QStyledItemDelegate::paint(painter, opt, index);
		if (index.data(TransactionModel::EnabledRole).toBool())
		{
			qlonglong progress = index.data(TransactionModel::CurrentRole).toLongLong();
			qlonglong total = index.data(TransactionModel::TotalRole).toLongLong();

			int percent = total == 0 ? 0 : qFloor(progress * 100 / total);

			QStyleOptionProgressBar style;
			style.rect = opt.rect;
			style.minimum = 0;
			style.maximum = total;
			style.progress = progress;
			style.text = QString("%1 %").arg(percent);
			style.textVisible = true;

			QApplication::style()->drawControl(QStyle::CE_ProgressBar, &style, painter);
		}
	}
	virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		auto hint = QStyledItemDelegate::sizeHint(option, index);
		if(hint.width() < 150)
			hint.setWidth(150);
		return hint;
	}
};

QuickModInstallDialog::QuickModInstallDialog(std::shared_ptr<OneSixInstance> instance, std::shared_ptr<Transaction> transaction,
											 QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModInstallDialog)
{
	m_model = std::make_shared<TransactionModel>(instance, transaction);
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);

	setWebViewShown(false);

	ui->progressList->setModel(m_model.get());
	ui->progressList->setItemDelegateForColumn(2, new ProgressItemDelegate(this));

	connect(ui->progressList, &QWidget::customContextMenuRequested, this,
			&QuickModInstallDialog::contextMenuRequested);

	connect(m_model.get(), &TransactionModel::hidePage, this, &QuickModInstallDialog::hidePage);
	connect(m_model.get(), &TransactionModel::showPageOfRow, this, &QuickModInstallDialog::showPageOfRow);
	ui->progressList->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->webView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
	m_model->start();
}

QuickModInstallDialog::~QuickModInstallDialog()
{
	delete ui;
}

void QuickModInstallDialog::contextMenuRequested(const QPoint &pos)
{
	/*
	QTreeWidgetItem *item = ui->progressList->itemAt(pos);
	if (!item)
	{
		return;
	}
	const QuickModVersionPtr version = m_progressEntries.key(item);
	QMenu menu;
	QAction *openWebsite = menu.addAction(version->mod->icon(), tr("Open website"));
	QAction *copyDownload = menu.addAction(tr("Copy download link"));
	QAction *copyDonation = menu.addAction(tr("Copy donation link"));

	connect(version->mod.get(), &QuickModMetadata::iconUpdated, [openWebsite, version]()
			{
		openWebsite->setIcon(version->mod->icon());
	});
	connect(openWebsite, &QAction::triggered, [version]()
			{
		QDesktopServices::openUrl(version->mod->websiteUrl());
	});
	connect(copyDownload, &QAction::triggered, [item]()
			{
		GuiUtil::setClipboardText(item->text(2));
	});
	connect(copyDonation, &QAction::triggered, [item]()
			{
		GuiUtil::setClipboardText(item->text(3));
	});

	copyDownload->setVisible(!item->text(2).isEmpty());
	copyDonation->setVisible(!item->text(3).isEmpty());

	menu.exec(QCursor::pos());
	*/
}

bool QuickModInstallDialog::checkIsDone()
{
	/*
	if (m_downloadingUrls.isEmpty() && ui->webTabView->count() == 0 && m_modVersions.isEmpty())
	{
		ui->finishButton->setEnabled(true);
		return true;
	}
	else
	{
		ui->finishButton->setDisabled(true);
		return false;
	}
	*/
	ui->finishButton->setEnabled(true);
	return true;
}

void QuickModInstallDialog::setWebViewShown(bool shown)
{
	if (shown)
		ui->downloadSplitter->setSizes(QList<int>({100, 500}));
	else
		ui->downloadSplitter->setSizes(QList<int>({100, 0}));
}

int QuickModInstallDialog::exec()
{
	showMaximized();

	checkIsDone();
	return QDialog::exec();
}

void QuickModInstallDialog::hidePage()
{
	setWebViewShown(false);
}

void QuickModInstallDialog::showPageOfRow(int row)
{
	auto page = m_model->getPage(row);
	if(page)
	{
		ui->webView->setPage(page);
		setWebViewShown(true);
	}
}

#include "QuickModInstallDialog.moc"
