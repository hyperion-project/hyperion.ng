#pragma once

#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <map>

// standard log messages
//#define _FUNCNAME_ __PRETTY_FUNCTION__
#define _FUNCNAME_ __FUNCTION__

#define Debug(logger, ...)   { (logger)->Message(Logger::DEBUG  , __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__); }
#define Info(logger, ...)    { (logger)->Message(Logger::INFO   , __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__); }
#define Warning(logger, ...) { (logger)->Message(Logger::WARNING, __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__); }
#define Error(logger, ...)   { (logger)->Message(Logger::ERROR  , __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__); }

// conditional log messages
#define DebugIf(condition, logger, ...)   { if (condition) {(logger)->Message(Logger::DEBUG   , __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__);} }
#define InfoIf(condition, logger, ...)    { if (condition) {(logger)->Message(Logger::INFO    , __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__);} }
#define WarningIf(condition, logger, ...) { if (condition) {(logger)->Message(Logger::WARNING , __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__);} }
#define ErrorIf(condition, logger, ...)   { if (condition) {(logger)->Message(Logger::ERROR   , __FILE__, _FUNCNAME_, __LINE__, __VA_ARGS__);} }

// ================================================================

class Logger
{
public:
	enum LogLevel { DEBUG=0, INFO=1,WARNING=2,ERROR=3 };

	static Logger* getInstance(std::string name="", LogLevel minLevel=Logger::INFO);

	void Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...);
	void setMinLevel(LogLevel level) { _minLevel = level; };

protected:
	Logger( std::string name="", LogLevel minLevel=INFO);
	~Logger();

private:
	static std::map<std::string,Logger*> *LoggerMap;

	std::string _name;
	std::string _appname;
	LogLevel    _minLevel;
	bool        _syslogEnabled;
	unsigned int _loggerId;
};

