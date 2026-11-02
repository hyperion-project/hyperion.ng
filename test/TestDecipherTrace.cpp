// STL includes
#include <iostream>
#include <string>

#include <execinfo.h>
#include <stdlib.h>

#include <utils/DefaultSignalHandler.h>

static int failures = 0;

void test(const std::string& name, const std::string& trace, const std::string& expected)
{
	const std::string result = DefaultSignalHandler::decipher_trace(trace);
	const bool isOk = (result == expected);
	if (!isOk)
	{
		++failures;
	}

	std::cout << (isOk ? "OK  " : "NOK ") << name << std::endl;
	if (!isOk)
	{
		std::cout << "    input:    [" << trace << "]" << std::endl;
		std::cout << "    expected: [" << expected << "]" << std::endl;
		std::cout << "    got:      [" << result << "]" << std::endl;
	}
}

int main()
{
	// glibc: "binary(mangled+offset) [address]"
	test("glibc frame",
		"./hyperiond(_ZN6Logger11getInstanceERK7QString+0x1c) [0x55d4c1e2a3b4]",
		"./hyperiond(Logger::getInstance(QString const&)+0x1c) [0x55d4c1e2a3b4]");

	test("glibc frame without offset",
		"./hyperiond(_ZN6Logger11getInstanceERK7QString) [0x55d4c1e2a3b4]",
		"./hyperiond(Logger::getInstance(QString const&)) [0x55d4c1e2a3b4]");

	// macOS: "index  binary  address  mangled + offset"
	test("macOS frame",
		"3   Hyperion                            0x0000000100abc123 _ZN6Logger11getInstanceERK7QString + 336",
		"3   Hyperion                            0x0000000100abc123 Logger::getInstance(QString const&) + 336");

	test("macOS frame with C symbol",
		"4   libsystem_c.dylib                   0x0000000185cd6a2c abort + 148",
		"4   libsystem_c.dylib                   0x0000000185cd6a2c abort + 148");

	test("unparsable frame is returned unchanged",
		"no symbol information",
		"no symbol information");

	test("empty frame",
		"",
		"??\n");

	// Every frame this platform's backtrace_symbols() returns must be handled
	void* addresses[10];
	const int size = backtrace(addresses, 10);
	char** symbols = backtrace_symbols(addresses, size);
	for (int i = 0; i < size; ++i)
	{
		std::cout << "    " << DefaultSignalHandler::decipher_trace(symbols[i]) << std::endl;
	}
	free(symbols);

	std::cout << (failures == 0 ? "All tests passed" : "Some tests failed") << std::endl;
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
