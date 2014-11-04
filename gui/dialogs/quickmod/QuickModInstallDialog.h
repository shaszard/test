/* Copyright 2013 MultiMC Contributors
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

#include <QDialog>
#include <memory>

namespace Ui
{
class QuickModInstallDialog;
}

class TransactionModel;
class Transaction;
class OneSixInstance;

class QuickModInstallDialog : public QDialog
{
	Q_OBJECT

public:
	explicit QuickModInstallDialog(std::shared_ptr<OneSixInstance> instance,
								   std::shared_ptr<Transaction> transaction,
								   QWidget *parent = 0);
	~QuickModInstallDialog();

public slots:
	int exec() override;

private slots:
	/** Checks if all downloads are finished and returns true if they are.
	 *  Note that this function also updates the state of the "finish" button.
	 */
	bool checkIsDone();

	/// Sets whether the web view is shown or not.
	void setWebViewShown(bool shown);

	void contextMenuRequested(const QPoint &pos);
	void hidePage();
	void showPageOfRow(int);

private: /* data */
	Ui::QuickModInstallDialog *ui;
	std::shared_ptr<TransactionModel> m_model;
};
