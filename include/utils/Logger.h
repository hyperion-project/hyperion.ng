#pragma once

// QT includes
#include <QObject>
#include <QString>

// stl includes
#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <QVector>

#include <utils/global_defines.h>

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

	static Logger*  getInstance(QString name="", LogLevel minLevel=Logger::INFO);
	static void     deleteInstance(QString name="");
	static void     setLogLevel(LogLevel level, QString name="");
	static LogLevel getLogLevel(QString name="");

	void     Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...);
	void     setMinLevel(LogLevel level) { _minLevel = level; };
	LogLevel getMinLevel() { return _minLevel; };

signals:
	void newLogMessage(Logger::T_LOG_MESSAGE);

protected:
	Logger( QString name="", LogLevel minLevel=INFO);
	~Logger();

private:
	static std::map<QString,Logger*> *LoggerMap;
	static LogLevel GLOBAL_MIN_LOG_LEVEL;

	QString  _name;
	QString  _appname;
	LogLevel _minLevel;
	bool     _syslogEnabled;
	unsigned _loggerId;
};


class LoggerManager : public QObject
{
	Q_OBJECT

public:
	static LoggerManager* getInstance();
	QVector<Logger::T_LOG_MESSAGE>* getLogMessageBuffer() { return &_logMessageBuffer; };

public slots:
	void handleNewLogMessage(const Logger::T_LOG_MESSAGE&);

signals:
	void newLogMessage(Logger::T_LOG_MESSAGE);

protected:
	LoggerManager();

	static LoggerManager*          _instance;
	QVector<Logger::T_LOG_MESSAGE> _logMessageBuffer;
	const int                      _loggerMaxMsgBufferSize;
};

Q_DECLARE_METATYPE(Logger::T_LOG_MESSAGE);
