#include "ImgurState.h"

#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "logic/MMCJson.h"
#include "logic/screenshots/ImgurAuthTask.h"
#include "BuildConfig.h"
#include "MultiMC.h"

namespace Imgur
{

State::State() : QObject(0)
{
}

State *State::instance()
{
	static State state;
	return &state;
}

void State::save()
{
	if (!m_isLoaded)
	{
		return;
	}

	using namespace MMCJson;
	QJsonObject obj;
	writeString(obj, "refreshToken", m_refreshToken);
	writeFile("imgur.json", QJsonDocument(obj));
}

QString State::accessToken()
{
	if (m_tokenValidUntil.isNull() || QDateTime::currentDateTimeUtc() >= m_tokenValidUntil)
	{
		const bool changed = !m_username.isEmpty() || !m_accessToken.isEmpty();
		m_username.clear();
		m_accessToken.clear();
		if (changed)
		{
			emit loginStatusChanged();
		}
	}
	return m_accessToken;
}
void State::setAccessToken(const QString &token, const QString &username, const int expiration)
{
	m_username = username;
	m_accessToken = token;
	m_tokenValidUntil = QDateTime::currentDateTimeUtc().addSecs(expiration);

	emit loginStatusChanged();
}
QString State::refreshToken()
{
	ensureLoaded();
	return m_refreshToken;
}
void State::setRefreshToken(const QString &token)
{
	ensureLoaded();
	m_refreshToken = token;
	m_accessToken.clear();
	save();
}

Task *State::login()
{
	return new AuthTask;
}
void State::logout()
{
	setAccessToken(QString(), QString(), -1);
	setRefreshToken(QString());
}

void State::ensureLoaded()
{
	if (m_isLoaded)
	{
		return;
	}

	if (QFile::exists("imgur.json"))
	{
		using namespace MMCJson;
		const QJsonObject obj = ensureObject(parseFile("imgur.json", "imgur json file"), "imgur json file");
		m_refreshToken = ensureString(obj.value("refreshToken"));
	}
	m_isLoaded = true;
}

BaseTask::BaseTask(const QString &endpoint, const bool isAuthenticated, const int verb, const QByteArray &payload, QObject *parent)
	: Task(parent), m_endpoint(endpoint), m_isAuthenticated(isAuthenticated), m_verb(verb), m_payload(payload)
{
}

void BaseTask::executeTask()
{
	if (m_isAuthenticated)
	{
		AuthTask *authTask = new AuthTask(this);
		connect(authTask, &AuthTask::succeeded, this, &BaseTask::actualTask);
		connect(authTask, &AuthTask::failed, this, &BaseTask::emitFailed);
		connect(authTask, &AuthTask::status, this, &BaseTask::setStatus);
		connect(authTask, &AuthTask::progress, this, &BaseTask::setProgress2);
		authTask->start();
	}
	else
	{
		actualTask();
	}
}
void BaseTask::actualTask()
{
	QNetworkRequest req = QNetworkRequest(m_endpoint);
	if (!m_isAuthenticated)
	{
		req.setRawHeader("Authorization", QString("Client-ID " + BuildConfig.IMGUR_CLIENT_ID).toLatin1());
	}
	else
	{
		req.setRawHeader("Authorization", QString("Bearer " + State::instance()->accessToken()).toLatin1());
	}

	if (m_contentType.isEmpty() && (!m_payload.isEmpty() || m_verb == Post || m_verb == Put))
	{
		req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	}
	else if (!m_contentType.isEmpty())
	{
		req.setHeader(QNetworkRequest::ContentTypeHeader, m_contentType);
	}

	QNetworkReply *reply;
	switch (m_verb)
	{
	case Get: reply = MMC->qnam()->get(req); break;
	case Post: reply = MMC->qnam()->post(req, m_payload); break;
	case Put: reply = MMC->qnam()->put(req, m_payload); break;
	case Delete: reply = MMC->qnam()->deleteResource(req); break;
	}
	connect(reply, &QNetworkReply::finished, this, &BaseTask::networkFinished);
	connect(reply, &QNetworkReply::downloadProgress, this, &BaseTask::setProgress);
}

void BaseTask::networkFinished()
{
	setStatus(tr("Parsing response..."));

	using namespace MMCJson;

	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

	auto handleError = [reply, this]()
	{
		if (!retry())
		{
			try
			{
				try
				{
					parseImgurResponse<Token>(reply->readAll());
				}
				catch (ImgurError &e)
				{
					emitFailed(e.cause());
				}
			}
			catch (...)
			{
				emitFailed(tr("Failed to execute Imgur request: %1").arg(reply->errorString()));
			}
		}
		else
		{
			executeTask();
		}
	};

	if (reply->error() != QNetworkReply::NoError)
	{
		handleError();
	}
	else
	{
		m_response = reply->readAll();
		done();
	}

	reply->deleteLater();

	if (isRunning())
	{
		emitSucceeded();
	}
}

}
