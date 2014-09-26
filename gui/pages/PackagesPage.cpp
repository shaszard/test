#include "gui/pages/PackagesPage.h"
#include "ui_PackagesPage.h"

#include <QMessageBox>

#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/widgets/PageContainer.h"
#include "gui/dialogs/quickmod/QuickModInstallDialog.h"

#include "logic/quickmod/InstancePackageList.h"
#include "logic/quickmod/QuickModVersionModel.h"
#include "logic/quickmod/Transaction.h"
#include "logic/quickmod/QuickModVersion.h"
#include "logic/quickmod/QuickModDatabase.h"
#include "MultiMC.h"

//BEGIN *struction
PackagesPage::PackagesPage(std::shared_ptr<OneSixInstance> instance, QWidget *parent)
	: QWidget(parent), ui(new Ui::PackagesPage), m_instance(instance)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	// FIXME: add a sorting proxy
	m_model = new InstancePackageModel(m_instance->installedPackages());
	ui->modTreeView->setModel(m_model);

	ui->modTreeView->setDragDropMode(QAbstractItemView::NoDragDrop);
	ui->modTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->modTreeView->setSelectionBehavior(QTreeView::SelectRows);
	ui->modTreeView->installEventFilter(this);

	// catch selection changes
	auto smodel = ui->modTreeView->selectionModel();
	connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
			SLOT(packageCurrent(QModelIndex, QModelIndex)));

	// hide the progress bar until needed
	ui->progressBar->setHidden(true);
	ui->modTreeView->setUniformRowHeights( true );
	ui->modTreeView->setIconSize(QSize(24,24));
}

PackagesPage::~PackagesPage()
{
	delete m_model;
	delete ui;
}

//END

//BEGIN Page methods
bool PackagesPage::apply()
{
	return true;
}

bool PackagesPage::shouldDisplay() const
{
	return true;
}

void PackagesPage::opened()
{
	// populate models here
}
//END

int PackagesPage::selectedRow()
{
	if (ui->modTreeView->selectionModel()->selectedRows().isEmpty())
	{
		return -1;
	}
	return ui->modTreeView->selectionModel()->selectedRows().first().row();
}

//BEGIN Event handling
void PackagesPage::on_applyTransaction_clicked()
{
	auto transaction = m_instance->installedPackages()->getTransaction();

	if (transaction->getActions().isEmpty())
	{
		return;
	}

	// TODO dependency resolution
	// transaction->depres();

	// user confirmation
	{
		QStringList installs, removals, versionChanges;
		for (const auto action : transaction->getActions())
		{
			const QString uid = MMC->qmdb()->userFacingUid(action.uid);
			if (action.type == Transaction::Action::Add)
			{
				installs.append(QString("%1 (%2)").arg(uid, action.targetVersion));
			}
			else if (action.type == Transaction::Action::ChangeVersion)
			{
				versionChanges.append(QString("%1 (%2 -> %3").arg(uid, m_instance->installedPackages()->installedQuickModVersion(QuickModRef(action.uid)).userFacing(),
																  action.targetVersion));
			}
			else if (action.type == Transaction::Action::Remove)
			{
				removals.append(uid);
			}
		}

		QString msg = tr("The following actions will be taken:");
		if (!installs.isEmpty())
		{
			msg += '\n' + tr("Install: ") + installs.join(", ");
		}
		if (!removals.isEmpty())
		{
			msg += '\n' + tr("Removal: ") + removals.join(", ");
		}
		if (!versionChanges.isEmpty())
		{
			msg += '\n' + tr("Version changes: ") + versionChanges.join(", ");
		}
		msg += "\n\n" + tr("Proceed?");
		if (QMessageBox::question(this, tr("QuickMod Install"), msg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		{
			return;
		}
	}

	QuickModInstallDialog dlg(m_instance, this);
	dlg.exec();
}

void PackagesPage::on_changeVersion_clicked()
{
	auto row = selectedRow();
	if (row == -1)
		return;

	auto uid = m_model->data(m_model->index(row), InstancePackageModel::UidRole);
	auto ref = QuickModRef(uid.toString());

	//FIXME: copypasta in QuickModBrowsePage!!! REMOVE!!!
	VersionSelectDialog dialog(
		new QuickModVersionModel(ref, m_instance->intendedVersionId(), this),
		tr("Choose QuickMod version for %1").arg(ref.userFacing()), this);

	if (!dialog.exec())
		return;
	std::shared_ptr<QuickModVersion> version = std::dynamic_pointer_cast<QuickModVersion>(dialog.selectedVersion());
	if(!version)
		return;
	auto transaction = m_instance->installedPackages()->getTransaction();
	
	transaction->setComponentVersion(uid.toString(), version->descriptor(), version->mod->repo());
}

void PackagesPage::on_install_clicked()
{
	if(parent_container)
		parent_container->showPage("quickmod-browse");
}

void PackagesPage::on_remove_clicked()
{
	auto row = selectedRow();
	if (row == -1)
		return;

	auto uid = m_model->data(m_model->index(row), InstancePackageModel::UidRole);
	auto transaction = m_instance->installedPackages()->getTransaction();

	transaction->removeComponent(uid.toString());
}

void PackagesPage::on_revert_clicked()
{
	auto row = selectedRow();
	if (row == -1)
		return;

	auto uid = m_model->data(m_model->index(row), InstancePackageModel::UidRole);
	auto transaction = m_instance->installedPackages()->getTransaction();

	transaction->resetComponentVersion(uid.toString());
}

void PackagesPage::on_revertTransaction_clicked()
{
	auto transaction = m_instance->installedPackages()->getTransaction();

	transaction->reset();
}

bool PackagesPage::eventFilter(QObject *obj, QEvent *ev)
{
	return QObject::eventFilter(obj, ev);
}

void PackagesPage::packageCurrent(const QModelIndex &current, const QModelIndex &previous)
{
}
//END
