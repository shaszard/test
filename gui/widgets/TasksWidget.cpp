#include "TasksWidget.h"

#include <QTreeView>
#include <QPushButton>
#include <QGridLayout>

#include "logic/tasks/TasksModel.h"
#include "MultiMC.h"

TasksWidget::TasksWidget(QWidget *parent) : QWidget(parent)
{
	m_view = new QTreeView(this);
	m_view->setModel(MMC->tasksModel().get());

	m_cancelButton = new QPushButton(tr("Cancel"), this);
	m_closeButton = new QPushButton(tr("Close"), this);

	m_layout = new QGridLayout(this);
	m_layout->addWidget(m_view, 0, 0, 1, 3);
	m_layout->addWidget(m_cancelButton, 1, 0, 1, 1);
	m_layout->addWidget(m_closeButton, 1, 2, 1, 1);
	m_layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Maximum), 1, 1, 1, 1);
	setLayout(m_layout);
}
