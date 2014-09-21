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

#pragma once

#include <QWidget>

#include "logic/OneSixInstance.h"
#include "logic/net/NetJob.h"
#include <logic/quickmod/InstancePackageModel.h>
#include "BasePage.h"

class EnabledItemFilter;
class InstancePackageList;
namespace Ui
{
class PackagesPage;
}

class PackagesPage : public QWidget, public BasePage
{
	Q_OBJECT
private:
	int selectedRow();

public:
	explicit PackagesPage(std::shared_ptr<OneSixInstance> instance, QWidget *parent = 0);
	virtual ~PackagesPage();

	QString id() const override
	{
		return "instance-packages";
	}
	QString displayName() const override
	{
		return tr("Packages");
	}
	QIcon icon() const override
	{
		return QIcon::fromTheme("quickmod");
	}
	bool apply() override;
	bool shouldDisplay() const override;
	void opened() override;

protected:
	bool eventFilter(QObject *obj, QEvent *ev);

protected:
	BaseInstance *m_inst;

private:
	Ui::PackagesPage *ui;
	std::shared_ptr<OneSixInstance> m_instance;
	InstancePackageModel *m_model;

public slots:
	void packageCurrent(const QModelIndex &current, const QModelIndex &previous);

private slots:
	void on_install_clicked();
	void on_changeVersion_clicked();
	void on_revert_clicked();
	void on_remove_clicked();

	void on_applyTransaction_clicked();
	void on_revertTransaction_clicked();
};
