#include "HyperionConfig.h"
#include <utils/Profiler.h>
#include <utils/FileUtils.h>

#include <QFileInfo>
#include <QString>

struct StopWatchItem {
	const char* sourceFile;
	const char* func;
	unsigned int line;
	clock_t startTime;
};

static unsigned int blockCounter = 0;
static std::map<QString,StopWatchItem> GlobalProfilerMap;
Logger* Profiler::_logger = nullptr;

double getClockDelta(clock_t start)
{
	return ((double)(clock() - start) / CLOCKS_PER_SEC);
}

Profiler::Profiler(const char* sourceFile, const char* func, unsigned int line)
	: _file(sourceFile)
	, _func(func)
	, _line(line)
	, _blockId(blockCounter++)
	, _startTime(clock())
{
	Profiler::initLogger();
	_logger->Message(Logger::DEBUG,_file,_func,_line,">>> enter block %d", _blockId);
}

Profiler::~Profiler()
{
  	_logger->Message( Logger::DEBUG, _file,_func, _line, "<<< exit block %d, executed for %f s", _blockId, getClockDelta(_startTime));
}

void Profiler::initLogger()
{
	if (_logger == nullptr)
		_logger = Logger::getInstance("PROFILER", Logger::DEBUG);
}

void Profiler::TimerStart(const QString& timerName, const char* sourceFile, const char* func, unsigned int line)
{
	std::pair<std::map<QString,StopWatchItem>::iterator,bool> ret;
	Profiler::initLogger();

	StopWatchItem item = {sourceFile, func, line};

	ret = GlobalProfilerMap.emplace(timerName, item);
	if (!ret.second)
	{
		if (ret.first->second.sourceFile == sourceFile && ret.first->second.func == func && ret.first->second.line == line)
		{
			_logger->Message(Logger::DEBUG, sourceFile, func, line, "restart timer '%s'", QSTRING_CSTR(timerName));
			ret.first->second.startTime = clock();
		}
		else
		{
			_logger->Message(Logger::DEBUG, sourceFile, func, line, "ERROR timer '%s' started in multiple locations. First occurence %s:%d:%s()",
			                 QSTRING_CSTR(timerName), FileUtils::getBaseName(ret.first->second.sourceFile).toLocal8Bit().constData(), ret.first->second.line, ret.first->second.func);
		}
	}
	else
	{
		_logger->Message(Logger::DEBUG, sourceFile, func, line, "start timer '%s'", QSTRING_CSTR(timerName));
	}
}


void Profiler::TimerGetTime(const QString& timerName, const char* sourceFile, const char* func, unsigned int line)
{
	std::map<QString,StopWatchItem>::iterator ret = GlobalProfilerMap.find(timerName);
	Profiler::initLogger();
	if (ret != GlobalProfilerMap.end())
	{
		_logger->Message(Logger::DEBUG, sourceFile, func, line, "timer '%s' started at %s:%d:%s() took %f s execution time until here", QSTRING_CSTR(timerName),
		                 FileUtils::getBaseName(ret->second.sourceFile).toLocal8Bit().constData(), ret->second.line, ret->second.func, getClockDelta(ret->second.startTime));
	}
	else
	{
		_logger->Message(Logger::DEBUG, sourceFile, func, line, "ERROR timer '%s' not started", QSTRING_CSTR(timerName));
	}
}
