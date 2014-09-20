#include "gui/pages/PackagesPage.h"
#include "ui_PackagesPage.h"

#include "logic/quickmod/InstancePackageList.h"


//BEGIN *struction
PackagesPage::PackagesPage(std::shared_ptr<InstancePackageList> packages, QWidget *parent)
	: QWidget(parent), ui(new Ui::PackagesPage), m_packages(packages)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	// FIXME: add a sorting proxy
	m_model= new InstancePackageModel(m_packages);
	ui->modTreeView->setModel(m_model);

	ui->modTreeView->setDragDropMode(QAbstractItemView::NoDragDrop);
	ui->modTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->modTreeView->setSelectionBehavior(QTreeView::SelectRows);
	ui->modTreeView->installEventFilter(this);

	// catch selection changes
	auto smodel = ui->modTreeView->selectionModel();
	connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
			SLOT(packageCurrent(QModelIndex,QModelIndex)));

	// hide the progress bar until needed
	ui->progressBar->setHidden(true);
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


//BEGIN Event handling
void PackagesPage::on_applyTransaction_clicked()
{

}

void PackagesPage::on_changeVersion_clicked()
{

}

void PackagesPage::on_install_clicked()
{

}

void PackagesPage::on_remove_clicked()
{

}

void PackagesPage::on_revert_clicked()
{

}

void PackagesPage::on_revertTransaction_clicked()
{

}

bool PackagesPage::eventFilter(QObject* obj, QEvent* ev)
{
	return QObject::eventFilter(obj, ev);
}

void PackagesPage::packageCurrent(const QModelIndex& current, const QModelIndex& previous)
{

}
//END