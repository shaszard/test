#include "TasksDialog.h"

#include <QHBoxLayout>

#include "gui/widgets/TasksWidget.h"

TasksDialog::TasksDialog(QWidget *parent)
	: QDialog(parent)
{
	m_widget = new TasksWidget(this);
	m_layout = new QHBoxLayout(this);
	m_layout->addWidget(m_widget);
	setLayout(m_layout);
}
