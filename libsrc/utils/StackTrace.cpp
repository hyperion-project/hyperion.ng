#include <utils/StackTrace.h>
#include <utils/Logger.h>

#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>

#include <ostream>
#include <regex>

namespace StackTrace
{

std::string beautify_trace(const std::string & line)
{
        /* Examples :
	 *	/opt/Qt/5.15.0/gcc_64/lib/libQt5Core.so.5(_ZN7QThread4execEv+0x84) [0x7f58ddcc0134]
	 *	/opt/Qt/5.15.0/gcc_64/lib/libQt5Core.so.5(+0xb3415) [0x7f58ddcc1415]
        */

        std::ostringstream out;
        std::string regex = R"((.*)\((.*)\+(.*))";
        std::string source = line;
        std::smatch match;
        if (std::regex_match(source, match, std::regex(regex)) && match.size() == 4)
        {
                out << match[1].str();
                out << " : ";

                int status = 1;
                char * name = abi::__cxa_demangle(match[2].str().c_str(), nullptr, nullptr, &status);

                out << ((status == 0) ? name : match[2].str());
        }
        else {
                out << source;
        }

        return out.str();
}

void print_trace()
{
        const int MAX_SIZE = 50;
        void * addresses[MAX_SIZE];
        int size = backtrace(addresses, MAX_SIZE);
        char ** symbols = backtrace_symbols(addresses, size);

	Logger* log = Logger::getInstance("StackTrace");
        for (int i = 0; i < size; ++i)
        {
		Error(log, std::string("[CORE] " + std::to_string(i) + " " + beautify_trace(symbols[i])).c_str());
        }

        free(symbols);
}

} // namespace StackTrace
