/* Copyright 2014 MultiMC Contributors
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

#include "ProgressWidget.h"

#include <QKeyEvent>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

#include "logic/tasks/Task.h"
#include "gui/Platform.h"

ProgressWidget::ProgressWidget(QWidget *parent) : QWidget(parent)
{
	MultiMCPlatform::fixWM_CLASS(this);

	setMinimumWidth(400);
	setMaximumWidth(600);
	setWindowTitle(tr("Please wait..."));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setVisible(false);

	m_gridLayout = new QGridLayout(this);
	m_statusLabel = new QLabel(tr("Task Status..."), this);
	m_taskProgressBar = new QProgressBar(this);
	m_skipButton = new QPushButton(tr("Skip"), this);

	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_skipButton->sizePolicy().hasHeightForWidth());
	m_skipButton->setSizePolicy(sizePolicy);
	m_statusLabel->setWordWrap(true);
	m_taskProgressBar->setTextVisible(false);

	m_gridLayout->addWidget(m_statusLabel, 0, 0, 1, 1);
	m_gridLayout->addWidget(m_taskProgressBar, 1, 0, 1, 1);
	m_gridLayout->addWidget(m_skipButton, 2, 0, 1, 1);

	connect(m_skipButton, &QPushButton::clicked, [this](bool)
			{
		task->abort();
	});

	setSkipButton(false);
	changeProgress(0, 100);
}
ProgressWidget::~ProgressWidget()
{
}

void ProgressWidget::setSkipButton(bool present, const QString &label)
{
	m_skipButton->setVisible(present);
	m_skipButton->setText(label);
	updateGeometry();
}

QSize ProgressWidget::sizeHint() const
{
	return QSize(480, minimumSizeHint().height());
}

void ProgressWidget::exec(ProgressProvider *task)
{
	this->task = task;

	// Connect signals.
	connect(task, &Task::status, this, &ProgressWidget::changeStatus);
	connect(task, &Task::progress, this, &ProgressWidget::changeProgress);
	connect(task, &Task::succeeded, this, &ProgressWidget::hide);
	connect(task, &Task::failed, this, &ProgressWidget::hide);
	connect(task, &Task::started, this, &ProgressWidget::show);

	// if this didn't connect to an already running task, invoke start
	if (!task->isRunning())
	{
		task->start();
	}
}

ProgressProvider *ProgressWidget::getTask()
{
	return task;
}

void ProgressWidget::changeStatus(const QString &status)
{
	m_statusLabel->setText(status);
	updateGeometry();
}
void ProgressWidget::changeProgress(qint64 current, qint64 total)
{
	m_taskProgressBar->setMaximum(total);
	m_taskProgressBar->setValue(current);
}

void ProgressWidget::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
	{
		return;
	}
	QWidget::keyPressEvent(e);
}
