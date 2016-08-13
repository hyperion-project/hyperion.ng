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

static const char * LogLevelStrings[] = { "", "DEBUG", "INFO", "WARNING", "ERROR" };
static const int    LogLevelSysLog[]  = { LOG_DEBUG, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR };
static unsigned int loggerCount = 0;
static unsigned int loggerId = 0;

std::map<std::string,Logger*> *Logger::LoggerMap = nullptr;
Logger::LogLevel Logger::GLOBAL_MIN_LOG_LEVEL = Logger::UNSET;


Logger* Logger::getInstance(std::string name, Logger::LogLevel minLevel)
{
	if (LoggerMap == nullptr)
	{
		LoggerMap = new std::map<std::string,Logger*>;
	}
	
	if ( LoggerMap->find(name) == LoggerMap->end() )
	{
		Logger* log = new Logger(name,minLevel);
		LoggerMap->insert(std::pair<std::string,Logger*>(name,log)); // compat version, replace it with following line if we have 100% c++11
		//LoggerMap->emplace(name,log);  // not compat with older linux distro's e.g. wheezy
		return log;
	} 
	
	return LoggerMap->at(name);
}

void Logger::deleteInstance(std::string name)
{
	if (LoggerMap == nullptr)
		return;
	
	if ( name.empty() )
	{
		std::map<std::string,Logger*>::iterator it;
		for ( it=LoggerMap->begin(); it != LoggerMap->end(); it++)
		{
			delete it->second;
		}
		LoggerMap->clear();
	}
	else if (LoggerMap->find(name) != LoggerMap->end())
	{
		delete LoggerMap->at(name);
		LoggerMap->erase(name);
	}

}

void Logger::setLogLevel(LogLevel level,std::string name)
{
	if ( name.empty() )
	{
		GLOBAL_MIN_LOG_LEVEL = level;
	}
	else
	{
		Logger* log = Logger::getInstance(name,level);
		log->setMinLevel(level);
	}
}

Logger::LogLevel Logger::getLogLevel(std::string name)
{
	if ( name.empty() )
	{
		return GLOBAL_MIN_LOG_LEVEL;
	}

	Logger* log = Logger::getInstance(name);
	return log->getMinLevel();
}

Logger::Logger ( std::string name, LogLevel minLevel ):
	_name(name),
	_minLevel(minLevel),
	_syslogEnabled(true),
	_loggerId(loggerId++)
{
#ifdef __GLIBC__
    const char* _appname_char = program_invocation_short_name;
#else
    const char* _appname_char = getprogname();
#endif
	_appname = std::string(_appname_char);
	std::transform(_appname.begin(), _appname.end(),_appname.begin(), ::toupper);

	loggerCount++;

	if (_syslogEnabled && loggerCount == 1 )
	{
		openlog (_appname_char, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
	}
}

Logger::~Logger()
{
	Debug(this, "logger '%s' destroyed", _name.c_str() );
	loggerCount--;
	if ( loggerCount == 0 )
		closelog();
}


void Logger::Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...)
{
	if ( (GLOBAL_MIN_LOG_LEVEL == Logger::UNSET && level < _minLevel) // no global level, use level from logger
	  || (GLOBAL_MIN_LOG_LEVEL > Logger::UNSET && level < GLOBAL_MIN_LOG_LEVEL) ) // global level set, use global level
		return;

	const size_t max_msg_length = 1024;
	char msg[max_msg_length];
	va_list args;
	va_start (args, fmt);
	vsnprintf (msg, max_msg_length, fmt, args);
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


