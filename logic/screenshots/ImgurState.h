#pragma once

#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <memory>

#include "logic/tasks/Task.h"
#include "logic/screenshots/ImgurDataModels.h"

namespace Imgur
{

class State : public QObject
{
	Q_OBJECT

public:
	static State *instance();

	void save();

	QString accessToken();
	void setAccessToken(const QString &token, const QString &username, const int expiration);
	QString refreshToken();
	void setRefreshToken(const QString &token);

	Task *login();
	void logout();

	bool isLoggedIn()
	{
		return !accessToken().isEmpty();
	}
	QString username()
	{
		accessToken();
		return m_username;
	}

signals:
	void loginStatusChanged();

private:
	State();

	void ensureLoaded();

	bool m_isLoaded = false;
	QString m_refreshToken;

	QString m_accessToken;
	QString m_username;
	QDateTime m_tokenValidUntil;
};

class BaseTask : public Task
{
	Q_OBJECT
public:
	virtual ~BaseTask() {}

protected:
	enum { Get, Post, Put, Delete };

	explicit BaseTask(const QString &endpoint, const bool isAuthenticated, const int verb, const QByteArray &payload, QObject *parent = 0);
	void executeTask() override;
	virtual void done() {}
	virtual bool retry() { return false; }

	template<typename Model>
	Model get()
	{
		Q_ASSERT(isRunning());
		try
		{
			return parseImgurResponse<Model>(m_response).data;
		}
		catch (MMCError &e)
		{
			emitFailed(tr("Failed to parse Imgur response: %1").arg(e.cause()));
		}
		return Model();
	}

	QUrl m_endpoint;
	bool m_isAuthenticated;
	int m_verb;
	QByteArray m_payload;
	QString m_contentType;

private:

	QByteArray m_response;

private slots:
	void actualTask();
	void networkFinished();
};

}
