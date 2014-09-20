#include "QuickModRepoDialog.h"
#include "ui_QuickModRepoDialog.h"

#include "logic/quickmod/QuickModModel.h"
#include "logic/quickmod/QuickModIndexModel.h"
#include "MultiMC.h"

QuickModRepoDialog::QuickModRepoDialog(QWidget *parent)
	: QDialog(parent), ui(new Ui::QuickModRepoDialog)
{
	ui->setupUi(this);
	
	m_list = new QuickModIndexModel(this);
	ui->treeView->setModel(m_list);
	ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->treeView->setSelectionMode(QTreeView::MultiSelection);
	ui->treeView->setSelectionBehavior(QTreeView::SelectRows);
}

QuickModRepoDialog::~QuickModRepoDialog()
{
	delete ui;
}

void QuickModRepoDialog::on_removeBtn_clicked()
{
	QModelIndexList indexes = ui->treeView->selectionModel()->selectedRows();
	QList<QuickModMetadataPtr> mods;
	for (const auto index : indexes)
	{
		auto uid = QuickModRef(index.data(Qt::UserRole).toString());
		mods.append(MMC->qmdb()->someModMetadata(uid));
	}
	mods.removeAll(nullptr);
	for (const auto mod : mods)
	{
		// FIXME MMC->quickmodslist()->unregisterMod(mod);
	}
	m_list->reload();
}
