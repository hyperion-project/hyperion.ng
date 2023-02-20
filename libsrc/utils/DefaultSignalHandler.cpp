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
struct Signal
{
	int          number;
	const char * name;
};

const Signal ALL_SIGNALS[] = {
	{ SIGABRT, "SIGABRT" },
	{ SIGBUS,  "SIGBUS"  },
	{ SIGFPE,  "SIGFPE"  },
	{ SIGSEGV, "SIGSEGV" },
	{ SIGTERM, "SIGTERM" },
	{ SIGHUP,  "SIGHUP"  },
	{ SIGINT,  "SIGINT"  },
	{ SIGPIPE, "SIGPIPE" },
};

void write_to_stderr(const char* data, size_t size)
{
	int res = write(STDERR_FILENO, data, size);

	Q_UNUSED(res);
}

void write_to_stderr(const char* data)
{
	write_to_stderr(data, strlen(data));
}

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

	/* Skip first 2 frames as they are signal
	 * handler and print_trace functions. */
	for (int i = 2; i < size; ++i)
	{
		const std::string line = "\t" + decipher_trace(symbols[i]);
		Error(log, "%s", line.c_str());
	}

	free(symbols);
}

void install_default_handler(int signum)
{
	struct sigaction action{};
	sigemptyset(&action.sa_mask);
	action.sa_handler = SIG_DFL;
	(void)sigaction(signum, &action, nullptr);
}

/* Note that this signal handler is not async signal safe !
 * Ideally a signal handler should only flip a bit and defer
 * heavy work to some kind of bottom-half processing. */
void signal_handler(int signum, siginfo_t * /*info*/, void * /*context*/)
{
	const char * name = "UNKNOWN SIGNAL";

	for (const auto& s : ALL_SIGNALS) {
		if (s.number == signum) {
			name = s.name;
			break;
		}
	}

	write_to_stderr("\n");
	write_to_stderr("Hyperion caught signal :");
	write_to_stderr(name);
	write_to_stderr("\n");

	/* Anything below here is unsafe ! */

	switch(signum)
	{
	case SIGBUS:
	case SIGSEGV:
	case SIGABRT:
	case SIGFPE :
		print_trace();

		/* Don't catch our own signal */
		install_default_handler(signum);

		kill(getpid(), signum);
		return;
	case SIGINT :
	case SIGTERM:
	case SIGPIPE:
	default:
		/* If the signal_handler is hit before the event loop is started,
		 * following call will do nothing. So we queue the call. */
		QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);

		// Reset signal handler to default (in case this handler is not capable of stopping)
		install_default_handler(signum);
	}
}

} // namespace DefaultSignalHandler
#endif // _WIN32

namespace DefaultSignalHandler
{
void install()
{
#ifndef _WIN32
	Logger* log = Logger::getInstance("CORE");

	struct sigaction action{};
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = signal_handler;
	action.sa_flags |= SA_SIGINFO;

	for (const auto& s : ALL_SIGNALS)
	{
		if (sigaction(s.number, &action, nullptr)!= 0)
		{
			Error(log, "Failed to install handler for %s]\n", s.name);
		}
	}
#endif // _WIN32
}
} // namespace DefaultSignalHandler
