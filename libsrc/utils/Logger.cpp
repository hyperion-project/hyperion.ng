#include "utils/Logger.h"

#include <iostream>
#include <algorithm>
#include <syslog.h>
#include <map>
#include <QFileInfo>
#include <QString>

std::string getBaseName( std::string sourceFile)
{
	QFileInfo fi( sourceFile.c_str() );
	return fi.fileName().toStdString();
}

static const char * LogLevelStrings[] = { "DEBUG", "INFO", "WARNING", "ERROR" };
static const int    LogLevelSysLog[]  = { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR };
static unsigned int loggerCount = 0;
static unsigned int loggerId = 0;
std::map<std::string,Logger*> *Logger::LoggerMap = nullptr;


Logger* Logger::getInstance(std::string name, Logger::LogLevel minLevel)
{
	if (Logger::LoggerMap == nullptr)
	{
		Logger::LoggerMap = new std::map<std::string,Logger*>;
	}
	
	if ( LoggerMap->find(name) == LoggerMap->end() )
	{
		Logger* log = new Logger(name,minLevel);
		Logger::LoggerMap->insert(std::pair<std::string,Logger*>(name,log)); // compat version, replace it with following line if we have 100% c++11
		//Logger::LoggerMap->emplace(name,log);  // not compat with older linux distro's e.g. wheezy
		return log;
	} 
	
	return Logger::LoggerMap->at(name);
}



Logger::Logger ( std::string name, LogLevel minLevel ):
	_name(name),
	_minLevel(minLevel),
	_syslogEnabled(true),
	_loggerId(loggerId++)
{
#ifdef GLIBC
	_appname = std::string(program_invocation_short_name);
#else
	_appname = std::string(getprogname());
#endif
	std::transform(_appname.begin(), _appname.end(),_appname.begin(), ::toupper);

	loggerCount++;
	
// 	if (pLoggerMap == NULL)
// 		pLoggerMap = new std::map<unsigned int,Logger*>;
// 
// 	
	
	if (_syslogEnabled && loggerCount == 1 )
	{
		openlog (program_invocation_short_name, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
	}
}

Logger::~Logger()
{
	//LoggerMap.erase(_loggerId);
	loggerCount--;
	if ( loggerCount == 0 )
		closelog();
}


void Logger::Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...)
{
	if ( level < _minLevel )
		return;

	char msg[512];
	va_list args;
	va_start (args, fmt);
	vsprintf (msg,fmt, args);
	va_end (args);

	std::string location;
	std::string function(func);
	if ( level == Logger::DEBUG )
	{
		location = "<" + getBaseName(sourceFile) + ":" + QString::number(line).toStdString()+":"+ function + "()> ";
	}
	
	std::cout
		<< "[" << _appname << " " << _name << "] <" 
		<< LogLevelStrings[level] << "> " << location << msg
		<< std::endl;

	if ( _syslogEnabled && level >= Logger::WARNING )
		syslog (LogLevelSysLog[level], "%s", msg);
}


