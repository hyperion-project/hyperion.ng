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

std::string decipher_trace(const std::string &trace)
{
	std::string result;

	if(trace.empty())
	{
		result += "??\n";
		return result;
	}

	auto* begin = strchr(trace.c_str(), '(') + 1;
	auto* end = strchr(begin, '+');

	if(!end)
		end = strchr(begin, ')');

	std::string mangled_name(begin, end);

	int status;
	char * realname = abi::__cxa_demangle(mangled_name.c_str(), 0, 0, &status);
	result.insert(result.end(), trace.c_str(), begin);

	if(realname)
		result += realname;
	else
		result.insert(result.end(), begin, end);

	free(realname);
	result.insert(result.size(), end);

	return result;
}

void print_trace()
{
	const int MAX_SIZE = 50;
	void * addresses[MAX_SIZE];
	int size = backtrace(addresses, MAX_SIZE);

	if (!size)
		return;

	Logger* log = Logger::getInstance("CORE");
	char ** symbols = backtrace_symbols(addresses, size);
	for (int i = 0; i < size; ++i)
	{
		std::string line = "\t" + decipher_trace(symbols[i]);
		Error(log, line.c_str());
	}

	free(symbols);
}

/* Note that this signal handler is not async signal safe !
 * Ideally a signal handler should only flip a bit and defer
 * heavy work to some kind of bottom-half processing. */
void signal_handler(int signum, siginfo_t * /*info*/, void * /*context*/)
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
	case SIGFPE :
		print_trace();
		exit(1);
	case SIGINT :
	case SIGTERM:
	case SIGPIPE:
	default:
		/* If the signal_handler is hit before the event loop is started,
		 * following call will do nothing. So we queue the call. */

		// QCoreApplication::quit();
		QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);

		// Reset signal handler to default (in case this handler is not capable of stopping)
		struct sigaction action{};
		action.sa_handler = SIG_DFL;
		sigaction(signum, &action, nullptr);

	}
}

} // namespace DefaultSignalHandler
#endif // _WIN32

namespace DefaultSignalHandler
{
void install()
{
#ifndef _WIN32
	struct sigaction action{};
	action.sa_sigaction = signal_handler;
	action.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGHUP , &action, nullptr);
	sigaction(SIGFPE , &action, nullptr);
	sigaction(SIGINT , &action, nullptr);
	sigaction(SIGTERM, &action, nullptr);
	sigaction(SIGABRT, &action, nullptr);
	sigaction(SIGSEGV, &action, nullptr);
	sigaction(SIGPIPE, &action, nullptr);
#endif // _WIN32
}
} // namespace DefaultSignalHandler
