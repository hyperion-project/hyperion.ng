#include <utils/Logger.h>
#include <utils/FileUtils.h>

#include <iostream>
#include <algorithm>

#ifndef _WIN32
#include <syslog.h>
#elif _WIN32
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif
#include <QDateTime>
#include <QFileInfo>
#include <QMutexLocker>
#include <QThreadStorage>
#include <time.h>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	QRecursiveMutex        Logger::MapLock;
#else
	QMutex                 Logger::MapLock{ QMutex::Recursive };
#endif

QMap<QString,Logger*>  Logger::LoggerMap            { };
QAtomicInteger<int>    Logger::GLOBAL_MIN_LOG_LEVEL { static_cast<int>(Logger::UNSET)};

namespace
{
const char * LogLevelStrings[]   = { "", "DEBUG", "INFO", "WARNING", "ERROR", "OFF" };
#ifndef _WIN32
const int    LogLevelSysLog[]    = { LOG_DEBUG, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR };
#endif

const size_t MAX_IDENTIFICATION_LENGTH = 22;

QAtomicInteger<unsigned int> LoggerCount = 0;
QAtomicInteger<unsigned int> LoggerId    = 0;

const int MaxRepeatCountSize = 200;
QThreadStorage<int> RepeatCount;
QThreadStorage<Logger::T_LOG_MESSAGE> RepeatMessage;
} // namespace

Logger* Logger::getInstance(const QString & name, const QString & subName, Logger::LogLevel minLevel)
{
	QMutexLocker lock(&MapLock);

	Logger* log = LoggerMap.value(name + subName, nullptr);
	if (log == nullptr)
	{
		log = new Logger(name, subName, minLevel);
		LoggerMap.insert(name, log); // compat version, replace it with following line if we have 100% c++11
		//LoggerMap.emplace(name, log);  // not compat with older linux distro's e.g. wheezy
		connect(log, &Logger::newLogMessage, LoggerManager::getInstance(), &LoggerManager::handleNewLogMessage);
	}

	return log;
}

void Logger::deleteInstance(const QString & name, const QString & subName)
{
	QMutexLocker lock(&MapLock);

	if (name.isEmpty())
	{
		for (auto *logger : qAsConst(LoggerMap)) {
			delete logger;
		}

		LoggerMap.clear();
	}
	else
	{
		delete LoggerMap.value(name + subName, nullptr);
		LoggerMap.remove(name + subName);
	}
}

void Logger::setLogLevel(LogLevel level, const QString & name, const QString & subName)
{
	if (name.isEmpty())
	{
		GLOBAL_MIN_LOG_LEVEL = static_cast<int>(level);
	}
	else
	{
		Logger* log = Logger::getInstance(name, subName, level);
		log->setMinLevel(level);
	}
}

Logger::LogLevel Logger::getLogLevel(const QString & name, const QString & subName)
{
	if (name.isEmpty())
	{
		return static_cast<Logger::LogLevel>(int(GLOBAL_MIN_LOG_LEVEL));
	}

	const Logger* log = Logger::getInstance(name + subName);
	return log->getMinLevel();
}

Logger::Logger (const QString & name, const QString & subName, LogLevel minLevel)
	: QObject()
	, _name(name)
	, _subname(subName)
	, _syslogEnabled(true)
	, _loggerId(LoggerId++)
	, _minLevel(static_cast<int>(minLevel))
{
	qRegisterMetaType<Logger::T_LOG_MESSAGE>();

	if (LoggerCount.fetchAndAddOrdered(1) == 1)
	{
#ifndef _WIN32
		if (_syslogEnabled)
		{
			openlog (nullptr, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
		}
#endif
	}
}

Logger::~Logger()
{
	if (LoggerCount.fetchAndSubOrdered(1) == 0)
	{
#ifndef _WIN32
		if (_syslogEnabled)
		{
			closelog();
		}
#endif
	}
}

void Logger::write(const Logger::T_LOG_MESSAGE & message)
{
	QString location;
	if (message.level == Logger::DEBUG)
	{
		location = QString("%1:%2:%3() | ")
			.arg(message.fileName)
			.arg(message.line)
			.arg(message.function);
	}

	QString name = "|" + message.loggerSubName + "| " + message.loggerName;
	name.resize(MAX_IDENTIFICATION_LENGTH, ' ');

	const QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(message.utime);

	std::cout << QString("%1 %2 : <%3> %4%5")
			.arg(timestamp.toString("yyyy-MM-ddThh:mm:ss.zzz"))
			.arg(name)
			.arg(LogLevelStrings[message.level])
			.arg(location)
			.arg(message.message)
		.toStdString()
	<< std::endl;

	newLogMessage(message);
}

void Logger::Message(LogLevel level, const char* sourceFile, const char* func, unsigned int line, const char* fmt, ...)
{
	Logger::LogLevel globalLevel = static_cast<Logger::LogLevel>(int(GLOBAL_MIN_LOG_LEVEL));

	if ( (globalLevel == Logger::UNSET && level < _minLevel) // no global level, use level from logger
	  || (globalLevel > Logger::UNSET && level < globalLevel) ) // global level set, use global level
		return;

	const size_t max_msg_length = 1024;
	char msg[max_msg_length];
	va_list args;
	va_start (args, fmt);
	vsnprintf (msg, max_msg_length, fmt, args);
	va_end (args);

	const auto repeatedSummary = [&]
	{
		Logger::T_LOG_MESSAGE repMsg = RepeatMessage.localData();
		repMsg.message = "Previous line repeats " + QString::number(RepeatCount.localData()) + " times";
		repMsg.utime   = QDateTime::currentMSecsSinceEpoch();

		write(repMsg);
#ifndef _WIN32
		if ( _syslogEnabled && repMsg.level >= Logger::WARNING )
			syslog (LogLevelSysLog[repMsg.level], "Previous line repeats %d times", RepeatCount.localData());
#endif

		RepeatCount.setLocalData(0);
	};

	if (RepeatMessage.localData().loggerName == _name  &&
		RepeatMessage.localData().loggerSubName == _subname  &&
		RepeatMessage.localData().function == func &&
		RepeatMessage.localData().message == msg   &&
		RepeatMessage.localData().line == line)
	{
		if (RepeatCount.localData() >= MaxRepeatCountSize)
			repeatedSummary();
		else
			RepeatCount.setLocalData(RepeatCount.localData() + 1);
	}
	else
	{
		if (RepeatCount.localData())
			repeatedSummary();

		Logger::T_LOG_MESSAGE logMsg;

		logMsg.loggerName  = _name;
		logMsg.loggerSubName  = _subname;
		logMsg.function    = QString(func);
		logMsg.line        = line;
		logMsg.fileName    = FileUtils::getBaseName(sourceFile);
		logMsg.utime       = QDateTime::currentMSecsSinceEpoch();
		logMsg.message     = QString(msg);
		logMsg.level       = level;
		logMsg.levelString = LogLevelStrings[level];

		write(logMsg);
#ifndef _WIN32
		if ( _syslogEnabled && level >= Logger::WARNING )
			syslog (LogLevelSysLog[level], "%s", msg);
#endif
		RepeatMessage.setLocalData(logMsg);
	}
}

LoggerManager::LoggerManager()
	: QObject()
	, _loggerMaxMsgBufferSize(200)
{
	_logMessageBuffer.reserve(_loggerMaxMsgBufferSize);
}

void LoggerManager::handleNewLogMessage(const Logger::T_LOG_MESSAGE & msg)
{
	_logMessageBuffer.push_back(msg);
	if (_logMessageBuffer.length() > _loggerMaxMsgBufferSize)
	{
		_logMessageBuffer.pop_front();
	}

	emit newLogMessage(msg);
}

LoggerManager* LoggerManager::getInstance()
{
	static LoggerManager instance;
	return &instance;
}
