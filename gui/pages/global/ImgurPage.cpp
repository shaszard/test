#include "ImgurPage.h"
#include "ui_ImgurPage.h"

#include "logic/screenshots/ImgurState.h"
#include "logic/screenshots/ImgurAuthTask.h"

ImgurPage::ImgurPage(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ImgurPage)
{
	ui->setupUi(this);
	ui->progressWidget->setVisible(false);

	connect(Imgur::State::instance(), &Imgur::State::loginStatusChanged, this, &ImgurPage::updateUi);
}

ImgurPage::~ImgurPage()
{
	delete ui;
}

void ImgurPage::on_loginLogoutBtn_clicked()
{
	if (Imgur::State::instance()->isLoggedIn())
	{
		Imgur::State::instance()->logout();
	}
	else
	{
		ui->progressWidget->exec(Imgur::State::instance()->login());
	}
}
void ImgurPage::on_removeBtn_clicked()
{

}
void ImgurPage::on_openBtn_clicked()
{

}

void ImgurPage::updateUi()
{
	auto state = Imgur::State::instance();

	ui->usernameLabel->setText(state->username());
	ui->loginLogoutBtn->setText(state->isLoggedIn() ? tr("Logout") : tr("Login"));
}

void ImgurPage::opened()
{
}
