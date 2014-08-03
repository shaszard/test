#include "ImgurAuthTask.h"

#include <QUrl>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QInputDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "logic/settings/SettingsObject.h"
#include "logic/screenshots/ImgurEndpoints.h"
#include "logic/screenshots/ImgurState.h"
#include "logic/screenshots/ImgurDataModels.h"
#include "logic/MMCJson.h"
#include "BuildConfig.h"
#include "MultiMC.h"

namespace Imgur
{

AuthTask::AuthTask(QObject *parent) : BaseTask(Endpoints::token, false, Post, QByteArray(), parent)
{
}

void AuthTask::executeTask()
{
	try
	{
		if (!State::instance()->accessToken().isEmpty())
		{
			emitSucceeded();
			return;
		}

		if (State::instance()->refreshToken().isEmpty())
		{
			requestNewToken();
		}
		else
		{
			requestRefreshToken();
		}
	}
	catch (MMCError &e)
	{
		emitFailed(e.cause());
	}
}

void AuthTask::done()
{
	Token token = get<Token>();
	State::instance()->setAccessToken(token.accessToken, token.username, token.expires_in);
	State::instance()->setRefreshToken(token.refreshToken);
}

bool AuthTask::retry()
{
	if (!m_isRefresh)
	{
		return false;
	}
	State::instance()->setAccessToken(QString(), QString(), 0);
	State::instance()->setRefreshToken(QString());
	return true;
}

void AuthTask::requestNewToken()
{
	m_isRefresh = false;

	setStatus(tr("Waiting for pin..."));

	QUrl authorize(Endpoints::authorize);
	{
		QUrlQuery query;
		query.addQueryItem("response_type", "pin");
		query.addQueryItem("client_id", BuildConfig.IMGUR_CLIENT_ID);
		authorize.setQuery(query);
	}

	const QString pin = askForPin(authorize);
	if (pin.isNull())
	{
		emitFailed(tr("You didn't enter a PIN"));
		return;
	}

	QUrlQuery query;
	query.addQueryItem("client_id", BuildConfig.IMGUR_CLIENT_ID);
	query.addQueryItem("client_secret", BuildConfig.IMGUR_CLIENT_SECRET);
	query.addQueryItem("grant_type", "pin");
	query.addQueryItem("pin", pin);
	requestToken(query);
}
void AuthTask::requestRefreshToken()
{
	m_isRefresh = true;

	QUrlQuery query;
	query.addQueryItem("client_id", BuildConfig.IMGUR_CLIENT_ID);
	query.addQueryItem("client_secret", BuildConfig.IMGUR_CLIENT_SECRET);
	query.addQueryItem("grant_type", "refresh_token");
	query.addQueryItem("refresh_token", State::instance()->refreshToken());
	requestToken(query);
}

void AuthTask::requestToken(const QUrlQuery &query)
{
	setStatus(tr("Fetching access token..."));
	m_payload = query.toString(QUrl::FullyEncoded).toLatin1();
	m_contentType = "application/x-www-form-urlencoded";
	BaseTask::executeTask();
}

QString AuthTask::askForPin(const QUrl &url) const
{
	// TODO replace this with LogicalGui
	QDesktopServices::openUrl(url);
	return QInputDialog::getText(
				MMC->topLevelWidget(), tr("Enter PIN"),
				tr("The following link has opened in your browser: <a href=\"%1\">%1</a>.\nPlease "
				   "follow the steps to authenticate against Imgur and then enter the PIN you get "
				   "here:").arg(url.toString(QUrl::PrettyDecoded)));
}
}
