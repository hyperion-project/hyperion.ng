#pragma once

#ifndef _WIN32
#include <string>
#endif

namespace DefaultSignalHandler
{
	void install();

#ifndef _WIN32
	// Demangles the symbol name in a line returned by backtrace_symbols()
	std::string decipher_trace(const std::string& trace);
#endif
}
