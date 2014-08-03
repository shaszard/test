#pragma once

#include "logic/screenshots/ImgurState.h"

class QUrlQuery;

namespace Imgur
{

class AuthTask : public BaseTask
{
	Q_OBJECT
public:
	explicit AuthTask(QObject *parent = 0);

protected:
	void executeTask() override;
	void done() override;
	bool retry() override;

private:
	void requestNewToken();
	void requestRefreshToken();
	void requestToken(const QUrlQuery &query);
	QString askForPin(const QUrl &url) const;

	bool m_isRefresh;
};

}
