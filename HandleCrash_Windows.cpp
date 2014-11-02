#include "HandleCrash.h"

#include <pathutils.h>

#include <client/windows/handler/exception_handler.h>

namespace HandleCrash
{
void init()
{
	const QString path = PathCombine(QDir::current(), "crashes");
	ensureFolderPathExists(path);
	google_breakpad::MinidumpDescriptor descriptor(path.toStdString());
	google_breakpad::ExceptionHandler *handler = new google_breakpad::ExceptionHandler(path.toStdWString(), nullptr, nullptr, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
}
}
