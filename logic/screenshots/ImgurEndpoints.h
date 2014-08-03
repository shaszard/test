#pragma once

namespace Imgur
{
namespace Endpoints
{
static const char *authorize = "https://api.imgur.com/oauth2/authorize";
static const char *token = "https://api.imgur.com/oauth2/token";
static const char *account_base = "https://api.imgur.com/3/account/%1";
static const char *account_settings = "https://api.imgur.com/3/account/%1/settings";
static const char *account_album_ids = "https://api.imgur.com/3/account/%1/albums/ids";
static const char *account_image_ids = "https://api.imgur.com/3/account/%1/images/ids";
static const char *album = "https://api.imgur.com/3/album/%1";
static const char *album_images = "https://api.imgur.com/3/album/%1/images";
static const char *album_creation = "https://api.imgur.com/3/album/";
static const char *album_update = "https://api.imgur.com/3/album/%1";
static const char *album_delete = "https://api.imgur.com/3/album/%1";
static const char *album_images_add = "https://api.imgur.com/3/album/%1/add";
static const char *album_images_remove = "https://api.imgur.com/3/album/%1/remove_images";
static const char *image = "https://api.imgur.com/3/image/%1";
static const char *image_upload = "https://api.imgur.com/3/image";
static const char *image_delete = "https://api.imgur.com/3/image/%1";
static const char *image_update = "https://api.imgur.com/3/image/%1";
}
}
