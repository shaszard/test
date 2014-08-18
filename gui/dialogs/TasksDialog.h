#pragma once

#include <QDialog>

class TasksWidget;
class QHBoxLayout;

class TasksDialog : public QDialog
{
	Q_OBJECT
public:
	explicit TasksDialog(QWidget *parent = 0);

private:
	TasksWidget *m_widget;
	QHBoxLayout *m_layout;
};
