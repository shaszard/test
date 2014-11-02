#include "HandleCrash.h"

#include <pathutils.h>

#include <client/linux/handler/exception_handler.h>

namespace HandleCrash
{

static bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
						 void *context,
						 bool succeeded)
{
	return succeeded;
}

void init()
{
	ensureFolderPathExists("/tmp/multimc/crashes");
	google_breakpad::MinidumpDescriptor descriptor("/tmp/multimc/crashes");
	google_breakpad::ExceptionHandler *handler = new google_breakpad::ExceptionHandler(descriptor, nullptr, &dumpCallback, nullptr, true, -1);
}

}
