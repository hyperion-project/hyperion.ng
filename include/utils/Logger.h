#pragma once

// QT includes
#include <QObject>
#include <QString>

// stl includes
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <QVector>



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

class Logger : public QObject
{
	Q_OBJECT

public:
	enum LogLevel { UNSET=0,DEBUG=1, INFO=2,WARNING=3,ERROR=4,OFF=5 };

	typedef struct
	{
		QString      appName;
		QString      loggerName;
		QString      function;
		unsigned int line;
		QString      fileName;
		time_t       utime;
		QString      message;
		LogLevel     level;
		QString      levelString;
	} T_LOG_MESSAGE;

	static Logger*  getInstance(std::string name="", LogLevel minLevel=Logger::INFO);
	static void     deleteInstance(std::string name="");
	static void     setLogLevel(LogLevel level,std::string name="");
	static LogLevel getLogLevel(std::string name="");
	static QVector<Logger::T_LOG_MESSAGE>* getGlobalLogMessageBuffer() { return GlobalLogMessageBuffer; };

	void     Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...);
	void     setMinLevel(LogLevel level) { _minLevel = level; };
	LogLevel getMinLevel() { return _minLevel; };

signals:
	void newLogMessage(Logger::T_LOG_MESSAGE);

protected:
	Logger( std::string name="", LogLevel minLevel=INFO);
	~Logger();

private:
	static std::map<std::string,Logger*> *LoggerMap;
	static LogLevel GLOBAL_MIN_LOG_LEVEL;
	static QVector<T_LOG_MESSAGE> *GlobalLogMessageBuffer;
	static QVector<T_LOG_MESSAGE> *LogCallacks;

	std::string _name;
	std::string _appname;
	LogLevel    _minLevel;
	bool        _syslogEnabled;
	unsigned int _loggerId;
};

class LoggerNotifier : public QObject
{
	Q_OBJECT

public:
	static LoggerNotifier* getInstance();

protected:
	LoggerNotifier();
	~LoggerNotifier();

	static LoggerNotifier* instance;
public slots:
	void handleNewLogMessage(Logger::T_LOG_MESSAGE);

signals:
	void newLogMessage(Logger::T_LOG_MESSAGE);
};
