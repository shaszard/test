#pragma once

#include <QWidget>

class QTreeView;
class QPushButton;
class QGridLayout;

class TasksWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TasksWidget(QWidget *parent = 0);

private:
	QTreeView *m_view;
	QPushButton *m_cancelButton;
	QPushButton *m_closeButton;
	QGridLayout *m_layout;
};
