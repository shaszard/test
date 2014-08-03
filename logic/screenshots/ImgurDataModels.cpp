#include "ImgurDataModels.h"

#include <QDateTime>

#include "logic/MMCJson.h"

using namespace MMCJson;

namespace Imgur
{
namespace Detail
{
int parseProExpiration(const QJsonValue &value)
{
	if (value.isBool())
	{
		return ensureBoolean(value) ? 0 : -1;
	}
	else
	{
		return ensureInteger(value);
	}
}
const BaseResponse BaseResponse::read(const QByteArray &raw)
{
	const QJsonObject obj = ensureObject(parseDocument(raw, "imgur response"), "imgur response");
	BaseResponse resp;
	resp.success = ensureBoolean(obj.value("success"));
	resp.status = ensureInteger(obj.value("status"));
	resp.data = obj.value("data");
	return resp;
}
}

const Token Token::read(const QJsonValue &raw)
{
	const QJsonObject data = ensureObject(raw);
	Token token;
	token.accessToken = ensureString(data.value("access_token"));
	token.refreshToken = ensureString(data.value("refresh_token"));
	token.username = ensureString(data.value("account_username"));
	token.expires_in = ensureInteger(data.value("expires_in"), "expiration time");
	return token;
}
const Account Account::read(const QJsonValue &raw)
{
	const QJsonObject data = ensureObject(ensureObject(raw).value("data"));
	Account account;
	account.id = ensureInteger(data.value("id"));
	account.url = ensureString(data.value("url"));
	account.bio = ensureString(data.value("bio"));
	account.reputation = ensureDouble(data.value("reputation"));
	account.created = QDateTime::fromMSecsSinceEpoch(ensureInteger(data.value("created")));
	account.pro_expiration = Detail::parseProExpiration(data.value("pro_expiration"));
	return account;
}
const AccountSettings AccountSettings::read(const QJsonValue &raw)
{
	const QJsonObject data = ensureObject(ensureObject(raw).value("data"));
	AccountSettings settings;
	settings.email = ensureString(data.value("email"));
	settings.high_quality = ensureBoolean(data.value("high_quality"));
	settings.public_images = ensureBoolean(data.value("public_images"));
	settings.album_privacy = ensureString(data.value("album_privacy"));
	settings.pro_expiration = Detail::parseProExpiration(data.value("pro_expiration"));
	settings.accepted_gallery_terms = ensureBoolean(data.value("accepted_gallery_terms"));
	settings.active_emails = ensureStringList(data.value("active_emails"));
	settings.messaging_enabled = ensureBoolean(data.value("messaging_enabled"));
	for (const auto val : ensureArray(data.value("blocked_users")))
	{
		const QJsonObject obj = ensureObject(val);
		BlockedUser user;
		user.blocked_id = ensureInteger(obj.value("blocked_id"));
		user.blocked_url = ensureString(obj.value("blocked_url"));
		settings.blocked_users.append(user);
	}
	return settings;
}
const Image Image::read(const QJsonValue &raw)
{
	const QJsonObject data = ensureObject(ensureObject(raw).value("data"));
	Image image;
	image.id = ensureString(data.value("id"));
	image.title = data.value("title").isNull() ? QString() : ensureString(data.value("title"));
	image.description = data.value("description").isNull() ? QString() : ensureString(data.value("description"));
	image.datetime = QDateTime::fromMSecsSinceEpoch(ensureInteger(data.value("datetime")));
	image.type = ensureString(data.value("type"));
	image.animated = ensureBoolean(data.value("animated"));
	image.width = ensureInteger(data.value("width"));
	image.height = ensureInteger(data.value("height"));
	image.size = ensureInteger(data.value("size"));
	image.views = ensureInteger(data.value("views"));
	image.bandwidth = ensureInteger(data.value("bandwidth"));
	image.deletehash = data.value("deletehash").isNull() ? QString() : ensureString(data.value("deletehash"));
	image.section = data.value("section").isNull() ? QString() : ensureString(data.value("section"));
	image.link = ensureUrl(data.value("link"));
	image.favorite = ensureBoolean(data.value("favorite"));
	image.nsfw = ensureBoolean(data.value("nsfw"));
	image.vote = ensureString(data.value("vote"));
	return image;
}
const Album Album::read(const QJsonValue &raw)
{
	const QJsonObject data = ensureObject(ensureObject(raw).value("data"));
	Album album;
	album.id = ensureString(data.value("id"));
	album.title = data.value("title").isNull() ? QString() : ensureString(data.value("title"));
	album.description = data.value("description").isNull() ? QString() : ensureString(data.value("description"));
	album.datetime = QDateTime::fromMSecsSinceEpoch(ensureInteger(data.value("datetime")));
	album.cover = ensureString(data.value("cover"));
	album.cover_width = ensureInteger(data.value("cover_width"));
	album.cover_height = ensureInteger(data.value("cover_height"));
	album.account_url = data.value("account_url").isNull() ? QString() : ensureString(data.value("account_url"));
	album.privacy = ensureString(data.value("privacy"));
	album.layout = ensureString(data.value("layout"));
	album.views = ensureInteger(data.value("views"));
	album.link = ensureUrl(data.value("link"));
	album.favorite = ensureBoolean(data.value("favorite"));
	album.nsfw = ensureBoolean(data.value("nsfw"));
	album.section = data.value("section").isNull() ? QString() : ensureString(data.value("section"));
	album.order = ensureInteger(data.value("order"));
	album.deletehash = data.value("deletehash").isNull() ? QString() : ensureString(data.value("deletehash"));
	album.images_count = ensureInteger(data.value("images_count"));
	for (const auto val : ensureArray(data.value("images")))
	{
		album.images.append(Image::read(val));
	}
	if (album.images_count != album.images.size())
	{
		throw MMCError("Expected amount of images is not equal actual amount of images");
	}
	return album;
}
const Basic Basic::read(const QJsonValue &raw)
{
	Basic basic;
	if (raw.isNull())
	{
		basic.type = Null;
	}
	else if (raw.isBool())
	{
		basic.type = Boolean;
		basic.boolean = raw.toBool();
	}
	else
	{
		basic.type = Integer;
		basic.integer = ensureInteger(raw);
	}
	return basic;
}

ImgurError ImgurError::read(const QJsonValue &value, const int status)
{
	const QJsonObject obj = ensureObject(value, "imgur response error body");
	return ImgurError((Error)status, ensureString(obj.value("error")), ensureUrl(obj.value("request")));
}

template<>
const Response<Token> parseImgurResponse<Token>(const QByteArray &data)
{
	Response<Token> resp;
	resp.status = 200;
	resp.success = true;
	resp.data = Token::read(parseDocument(data, "imgur token response").object());
	return resp;
}

}
