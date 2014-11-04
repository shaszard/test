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
#include "logic/quickmod/QuickModDownloadTask.h"
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
	if (currentTransaction()->getActions().isEmpty())
	{
		return;
	}

	QuickModDownloadTask task(m_instance);
	task.start();
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
	
	currentTransaction()->setComponentVersion(uid.toString(), version->descriptor(), version->mod->repo());
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

	currentTransaction()->removeComponent(uid.toString());
}

void PackagesPage::on_revert_clicked()
{
	auto row = selectedRow();
	if (row == -1)
		return;

	auto uid = m_model->data(m_model->index(row), InstancePackageModel::UidRole);

	currentTransaction()->resetComponentVersion(uid.toString());
}

void PackagesPage::on_revertTransaction_clicked()
{
	currentTransaction()->reset();
}

std::shared_ptr<Transaction> PackagesPage::currentTransaction()
{
	return m_instance->installedPackages()->getTransaction();
}

void PackagesPage::packageCurrent(const QModelIndex &current, const QModelIndex &previous)
{
}
//END
