#pragma once

#include <QWidget>

#include "gui/pages/BasePage.h"

namespace Ui {
class ImgurPage;
}

class ImgurPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit ImgurPage(QWidget *parent = 0);
	~ImgurPage();

	QString id() const override { return "imgur-global"; }
	QString displayName() const override { return tr("Imgur"); }
	QIcon icon() const override { return QIcon::fromTheme("imgur"); }
	QString helpPage() const override { return "Screenshots"; }
	void opened() override;

private slots:
	void on_loginLogoutBtn_clicked();
	void on_removeBtn_clicked();
	void on_openBtn_clicked();
	void updateUi();

private:
	Ui::ImgurPage *ui;
};
