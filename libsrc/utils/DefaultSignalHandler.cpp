#include <utils/DefaultSignalHandler.h>
#include <utils/Logger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QCoreApplication>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <signal.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <ctype.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#endif

namespace DefaultSignalHandler {

#ifdef _WIN32
	void write_to_stderr(const char* data) {
		DWORD written;
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), data, strlen(data), &written, NULL);
	}
#else
	void write_to_stderr(const char* data, size_t size) {
		int res = write(STDERR_FILENO, data, size);
		Q_UNUSED(res);
	}

	void write_to_stderr(const char* data) {
		write_to_stderr(data, strlen(data));
	}
#endif

#ifndef _WIN32
	std::string decipher_trace(const std::string& trace) {
		std::string result;

		if (trace.empty()) {
			result += "??\n";
			return result;
		}

		auto* begin = strchr(trace.c_str(), '(') + 1;
		auto* end = strchr(begin, '+');
		if (!end) end = strchr(begin, ')');

		std::string mangled_name(begin, end);
		int status;
		char* realname = abi::__cxa_demangle(mangled_name.c_str(), 0, 0, &status);

		result.insert(result.end(), trace.c_str(), begin);
		if (realname) result += realname;
		else result.insert(result.end(), begin, end);
		free(realname);
		result.insert(result.size(), end);

		return result;
	}
#endif

	void print_trace() {
		Logger* log = Logger::getInstance("CORE");

#ifdef _WIN32
		void* stack[50];
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);

		USHORT frames = CaptureStackBackTrace(0, 50, stack, NULL);
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256);
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		for (USHORT i = 2; i < frames; ++i) {
			if (SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol)) {
				Error(log, "\t%s\n", symbol->Name);
			}
		}

		free(symbol);
#else
		const int MAX_SIZE = 50;
		void* addresses[MAX_SIZE];
		int size = backtrace(addresses, MAX_SIZE);

		if (!size) return;

		char** symbols = backtrace_symbols(addresses, size);
		for (int i = 2; i < size; ++i) {
			const std::string line = "\t" + decipher_trace(symbols[i]);
			Error(log, "%s", line.c_str());
		}

		free(symbols);
#endif
	}

#ifdef _WIN32
	void signal_handler(int signum) {
		const char* name = "UNKNOWN SIGNAL";

		switch (signum) {
		case SIGABRT: name = "SIGABRT"; break;
		case SIGFPE:  name = "SIGFPE";  break;
		case SIGSEGV: name = "SIGSEGV"; break;
		case SIGINT:  name = "SIGINT";  break;
		case SIGTERM: name = "SIGTERM"; break;
		}

		write_to_stderr("\n");
		write_to_stderr(QCoreApplication::applicationName().toLocal8Bit());
		write_to_stderr(" caught signal: ");
		write_to_stderr(name);
		write_to_stderr("\n");

		print_trace();

		QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
	}
#else
	struct Signal {
		int number;
		const char* name;
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

	void install_default_handler(int signum) {
		struct sigaction action {};
		sigemptyset(&action.sa_mask);
		action.sa_handler = SIG_DFL;
		(void)sigaction(signum, &action, nullptr);
	}

	void signal_handler(int signum, siginfo_t*, void*) {
		const char* name = "UNKNOWN SIGNAL";

		for (const auto& s : ALL_SIGNALS) {
			if (s.number == signum) {
				name = s.name;
				break;
			}
		}

		write_to_stderr("\n");
		write_to_stderr(QCoreApplication::applicationName().toLocal8Bit());
		write_to_stderr(" caught signal: ");
		write_to_stderr(name);
		write_to_stderr("\n");

		switch (signum) {
		case SIGBUS:
		case SIGSEGV:
		case SIGABRT:
		case SIGFPE:
			print_trace();
			install_default_handler(signum);
			kill(getpid(), signum);
			return;
		default:
			QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
			install_default_handler(signum);
		}
	}
#endif

	void install() {
		Logger* log = Logger::getInstance("CORE");

#ifdef _WIN32
		signal(SIGABRT, signal_handler);
		signal(SIGFPE, signal_handler);
		signal(SIGSEGV, signal_handler);
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);
#else
		struct sigaction action {};
		sigemptyset(&action.sa_mask);
		action.sa_sigaction = signal_handler;
		action.sa_flags |= SA_SIGINFO;

		for (const auto& s : ALL_SIGNALS) {
			if (sigaction(s.number, &action, nullptr) != 0) {
				Error(log, "Failed to install handler for %s\n", s.name);
			}
		}
#endif
	}

} // namespace DefaultSignalHandler
