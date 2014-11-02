#include "HandleCrash.h"

#include <pathutils.h>

#include <client/linux/handler/exception_handler.h>

namespace HandleCrash
{
void init()
{
	const QString path = PathCombine(QDir::currentPath(), "crashes");
	ensureFolderPathExists(path);
	google_breakpad::MinidumpDescriptor descriptor(path.toStdString());
	google_breakpad::ExceptionHandler *handler = new google_breakpad::ExceptionHandler(descriptor, nullptr, nullptr, nullptr, true, -1);
}
}
