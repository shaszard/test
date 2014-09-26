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

#include "gui/widgets/WebDownloadNavigator.h"
#include "gui/dialogs/ProgressDialog.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/quickmod/QuickModVerifyModsDialog.h"
#include "gui/dialogs/quickmod/QuickModDownloadSelectionDialog.h"
#include "gui/GuiUtil.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "logic/quickmod/QuickModMetadata.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/OneSixInstance.h"
#include "logic/net/ByteArrayDownload.h"
#include "logic/net/NetJob.h"
#include "MultiMC.h"
#include "logic/settings/SettingsObject.h"
#include "depends/quazip/JlCompress.h"
#include <pathutils.h>
#include "logic/quickmod/QuickModVersionModel.h"
#include "logic/quickmod/InstancePackageList.h"

Q_DECLARE_METATYPE(QTreeWidgetItem *)

// TODO load parallel versions in parallel (makes sense, right?)

struct ExtraRoles
{
	enum
	{
		ProgressRole = Qt::UserRole,
		TotalRole,
		IgnoreRole
	};
};

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

		if (!index.data(ExtraRoles::IgnoreRole).toBool())
		{
			qlonglong progress = index.data(ExtraRoles::ProgressRole).toLongLong();
			qlonglong total = index.data(ExtraRoles::TotalRole).toLongLong();

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
};

QuickModInstallDialog::QuickModInstallDialog(std::shared_ptr<OneSixInstance> instance,
											 QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModInstallDialog), m_instance(instance)
{
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);

	setWebViewShown(false);

	ui->progressList->setItemDelegateForColumn(4, new ProgressItemDelegate(this));

	ui->webModsProgressBar->setMaximum(0);
	ui->webModsProgressBar->setValue(0);

	// Set the URL column's width.
	ui->progressList->setColumnWidth(2, 420);

	connect(ui->progressList, &QWidget::customContextMenuRequested, this,
			&QuickModInstallDialog::contextMenuRequested);
	ui->progressList->setContextMenuPolicy(Qt::CustomContextMenu);
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

	//FIXME: move to a Task
	/*
	/// turn actions into extended actions, fit for processing
	auto actions = m_instance->installedPackages()->getTransaction()->getActions();
	for (const auto action : actions)
	{
		ExtendedAction ext_action(action);
		if (action.type == Transaction::Action::Add || action.type == Transaction::Action::ChangeVersion)
		{
			ext_action.version = MMC->qmdb()->version(action.uid, action.targetVersion, action.targetRepo);
			if(!ext_action.version)
			{
				ext_action.status = ExtendedAction::Failed;
				ext_action.message = tr("Mod not in database");
			}
		}
		m_actions.append(ext_action);
	}
	*/
	checkIsDone();
	return QDialog::exec();
}

// FIXME: left only as a 'howto'
/*

void QuickModInstallDialog::selectDownloadUrls()
{
	for(auto & action: m_actions)
	{
		QuickModVersionPtr version = action.version;

		const auto download = QuickModDownloadSelectionDialog::select(version, this);
		m_selectedDownloadUrls.insert(version, download);
	}
}

void QuickModInstallDialog::runWebDownload(QuickModVersionPtr version)
{
	const QUrl url = QUrl(m_selectedDownloadUrls[version].url);
	QLOG_INFO() << "Downloading " << version->name() << "(" << url.toString() << ")";

	auto navigator = new WebDownloadNavigator(this);
	navigator->load(url);
	ui->webTabView->addTab(navigator,
						   (QString("%1 %2").arg(version->mod->name(), version->name())));

	navigator->setProperty("version", QVariant::fromValue(version));
	connect(navigator, &WebDownloadNavigator::caughtUrl, [this, navigator](QNetworkReply *reply)
			{
		ui->webModsProgressBar->setValue(ui->webModsProgressBar->value() + 1);
		urlCaught(reply, navigator);
	});

	setWebViewShown(true);
}

void QuickModInstallDialog::urlCaught(QNetworkReply *reply, WebDownloadNavigator *navigator)
{
	// TODO: Do more to verify that the correct file is being downloaded.
	if (reply->url().path().endsWith(".exe"))
	{
		// because bad things
		QLOG_WARN() << "Caught .exe from" << reply->url().toString();
		return;
	}
	QLOG_INFO() << "Caught " << reply->url().toString();
	downloadNextMod();

	if (!navigator)
	{
		navigator = qobject_cast<WebDownloadNavigator *>(sender());
	}

	processReply(reply, navigator->property("version").value<QuickModVersionPtr>());

	ui->webTabView->removeTab(ui->webTabView->indexOf(navigator));
	navigator->deleteLater();
	if (ui->webTabView->count() == 0)
	{
		setWebViewShown(false);
	}
}
*/
#include "QuickModInstallDialog.moc"
