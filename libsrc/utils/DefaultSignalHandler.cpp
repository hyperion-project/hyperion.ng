#ifndef _WIN32
#include <utils/DefaultSignalHandler.h>
#include <utils/Logger.h>

#include <ctype.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>

namespace DefaultSignalHandler
{

void print_trace()
{
        const int MAX_SIZE = 50;
        void * addresses[MAX_SIZE];
        int size = backtrace(addresses, MAX_SIZE);

	if (!size)
		return;

        char ** symbols = backtrace_symbols(addresses, size);

        Logger* log = Logger::getInstance("CORE");
        for (int i = 0; i < size; ++i)
        {
	        /* Examples :
		 *      /opt/Qt/5.15.0/gcc_64/lib/libQt5Core.so.5(_ZN7QThread4execEv+0x84) [0x7f58ddcc0134]
		 *      /opt/Qt/5.15.0/gcc_64/lib/libQt5Core.so.5(+0xb3415) [0x7f58ddcc1415]
	        */;
		std::string result;
		auto* begin = strchr(symbols[i], '(') + 1;
		if(!symbols[i])
		{
			result += "??\n";
			continue;
		}
		auto* end = strchr(begin, '+');
		if(!end)
			end = strchr(begin, ')');

		std::string mangled_name(begin, end);

		int status;
		char * realname = abi::__cxa_demangle(mangled_name.c_str(), 0, 0, &status);
		result.insert(result.end(), symbols[i], begin);

		if(realname)
			result += realname;
		else
			result.insert(result.end(), begin, end);

		free(realname);
		result.insert(result.size(), end);

                Error(log, result.c_str());
        }

        free(symbols);
}

void signal_handler(const int signum)
{
	Logger* log = Logger::getInstance("SIGNAL");

	char *name = strsignal(signum);
	if (name)
	{
		Info(log, "Signal received : %s", name);
	}

	switch(signum)
	{
	case SIGSEGV:
	case SIGABRT:
	case SIGFPE:
		print_trace();
		exit(1);
	case SIGINT:
	case SIGTERM:
	case SIGPIPE:
	default:
		Info(log, "Quitting application");

		/* If the signal_handler is hit before the event loop is started,
		 * following call will do nothing. So we queue the call.
		 */
		// QCoreApplication::quit();
		QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);

		// Reset signal handler to default (in case this handler is not capable of stopping)
		signal(signum, SIG_DFL);
	}
}

void install()
{
	signal(SIGFPE,  signal_handler);
	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGPIPE, signal_handler);
};

} // namespace DefaultSignalHandler

#endif // _WIN32
