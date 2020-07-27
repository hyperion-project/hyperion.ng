#pragma once

// QT includes
#include <QObject>
#include <QString>
#include <QMap>
#include <QAtomicInteger>
#include <QList>
#include <QMutex>

// stl includes
#include <stdio.h>
#include <stdarg.h>
#ifdef _WIN32
#include <stdexcept>
#endif

#include <utils/global_defines.h>

#define LOG_MESSAGE(severity, logger, ...)   (logger)->Message(severity, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

// standard log messages
#define Debug(logger, ...)   LOG_MESSAGE(Logger::DEBUG  , logger, __VA_ARGS__)
#define Info(logger, ...)    LOG_MESSAGE(Logger::INFO   , logger, __VA_ARGS__)
#define Warning(logger, ...) LOG_MESSAGE(Logger::WARNING, logger, __VA_ARGS__)
#define Error(logger, ...)   LOG_MESSAGE(Logger::ERRORR , logger, __VA_ARGS__)

// conditional log messages
#define DebugIf(condition, logger, ...)   if (condition) Debug(logger,   __VA_ARGS__)
#define InfoIf(condition, logger, ...)    if (condition) Info(logger,    __VA_ARGS__)
#define WarningIf(condition, logger, ...) if (condition) Warning(logger, __VA_ARGS__)
#define ErrorIf(condition, logger, ...)   if (condition) Error(logger,   __VA_ARGS__)

// ================================================================

class Logger : public QObject
{
	Q_OBJECT

public:
	enum LogLevel {
		UNSET    = 0,
		DEBUG    = 1,
		INFO     = 2,
		WARNING  = 3,
		ERRORR   = 4,
		OFF      = 5
	};

	struct T_LOG_MESSAGE
	{
		QString      appName;
		QString      loggerName;
		QString      function;
		unsigned int line;
		QString      fileName;
		uint64_t     utime;
		QString      message;
		LogLevel     level;
		QString      levelString;
	};

	static Logger*  getInstance(const QString & name = "", LogLevel minLevel=Logger::INFO);
	static void     deleteInstance(const QString & name = "");
	static void     setLogLevel(LogLevel level, const QString & name = "");
	static LogLevel getLogLevel(const QString & name = "");

	void     Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...);
	void     setMinLevel(LogLevel level) { _minLevel = static_cast<int>(level); }
	LogLevel getMinLevel() { return static_cast<LogLevel>(int(_minLevel)); }
	QString  getName() const { return _name; }
	QString  getAppName() const { return _appname; }

signals:
	void newLogMessage(Logger::T_LOG_MESSAGE);

protected:
	Logger(const QString & name="", LogLevel minLevel = INFO);
	~Logger();

private:
	void write(const Logger::T_LOG_MESSAGE & message);

	static QMutex                MapLock;
	static QMap<QString,Logger*> LoggerMap;
	static QAtomicInteger<int>   GLOBAL_MIN_LOG_LEVEL;

	const QString                _name;
	const QString                _appname;
	const bool                   _syslogEnabled;
	const unsigned               _loggerId;

	/* Only non-const member, hence the atomic */
	QAtomicInteger<int> _minLevel;
};

class LoggerManager : public QObject
{
	Q_OBJECT

public:
	static LoggerManager* getInstance();
	const QList<Logger::T_LOG_MESSAGE>* getLogMessageBuffer() const { return &_logMessageBuffer; }

public slots:
	void handleNewLogMessage(const Logger::T_LOG_MESSAGE&);

signals:
	void newLogMessage(const Logger::T_LOG_MESSAGE&);

protected:
	LoggerManager();

	QList<Logger::T_LOG_MESSAGE>   _logMessageBuffer;
	const int                      _loggerMaxMsgBufferSize;
};

Q_DECLARE_METATYPE(Logger::T_LOG_MESSAGE)
