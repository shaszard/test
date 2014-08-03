#pragma once

#include <QString>
#include <QJsonValue>
#include <QUrl>
#include <QDateTime>

#include "MMCError.h"

// variable naming uses underscores ('_') to stay closer to the imgur api
namespace Imgur
{
namespace Detail
{
struct BaseResponse
{
	bool success;
	int status;
	QJsonValue data;

	static const BaseResponse read(const QByteArray &raw);
};
}

class ImgurError : public MMCError
{
public:
	enum Error
	{
		Ok = 200,
		InvalidRequest = 400,
		RequiredAuth = 401,
		Forbidden = 403,
		DoesNotExist = 404,
		RateLimited = 429,
		ServerError = 500
	};
private:
	explicit ImgurError(const Error error, const QString &message, const QUrl &request)
		: MMCError(QObject::tr("Imgur returned %1: %2").arg(errorString(error), message)), error(error), message(message), request(request)
	{
	}
public:
	~ImgurError() noexcept {}
	static ImgurError read(const QJsonValue &value, const int status);

	Error error;
	QString message;
	QUrl request;

	static QString errorString(const Error error)
	{
		switch (error)
		{
		case Ok: return QObject::tr("Ok");
		case InvalidRequest: return QObject::tr("InvalidRequest");
		case RequiredAuth: return QObject::tr("RequiredAuth");
		case Forbidden: return QObject::tr("Forbidden");
		case DoesNotExist: return QObject::tr("DoesNotExist");
		case RateLimited: return QObject::tr("RateLimited");
		case ServerError: return QObject::tr("ServerError");
		default: return QObject::tr("UnknownError");
		}
	}
};

struct Token
{
	QString accessToken;
	QString refreshToken;
	QString username;
	int expires_in;

	static const Token read(const QJsonValue &raw);
};

struct Account
{
	int id;
	QString url;
	QString bio;
	float reputation;
	QDateTime created;
	int pro_expiration;

	static const Account read(const QJsonValue &raw);
};
struct AccountSettings
{
	QString email;
	bool high_quality;
	bool public_images;
	QString album_privacy;
	int pro_expiration;
	bool accepted_gallery_terms;
	QStringList active_emails;
	bool messaging_enabled;
	struct BlockedUser
	{
		int blocked_id;
		QString blocked_url;
	};
	QList<BlockedUser> blocked_users;

	static const AccountSettings read(const QJsonValue &raw);
};

struct Image
{
	QString id;
	QString title;
	QString description;
	QDateTime datetime;
	QString type;
	bool animated;
	int width;
	int height;
	int size;
	int views;
	int bandwidth;
	QString deletehash;
	QString section;
	QUrl link;
	bool favorite;
	bool nsfw;
	QString vote;

	static const Image read(const QJsonValue &raw);
};
struct Album
{
	QString id;
	QString title;
	QString description;
	QDateTime datetime;
	QString cover;
	int cover_width;
	int cover_height;
	QString account_url;
	QString privacy;
	QString layout;
	int views;
	QUrl link;
	bool favorite;
	bool nsfw;
	QString section;
	int order;
	QString deletehash;
	int images_count;
	QList<Image> images;

	static const Album read(const QJsonValue &raw);
};

struct Basic
{
	enum Type
	{
		Null, Boolean, Integer
	} type;
	bool boolean;
	int integer;

	static const Basic read(const QJsonValue &raw);
};

template<typename T>
struct Response
{
	bool success;
	int status;
	T data;

	operator T() { return data; }
};
template<typename T>
const Response<T> parseImgurResponse(const QByteArray &data)
{
	const Detail::BaseResponse base = Detail::BaseResponse::read(data);
	if (!base.success)
	{
		throw ImgurError::read(base.data, base.status);
	}

	Response<T> resp;
	resp.success = base.success;
	resp.status = base.status;
	resp.data = T::read(base.data);
	return resp;
}
template<>
const Response<Token> parseImgurResponse<Token>(const QByteArray &data);

}
