#include "HyperionConfig.h"
#include "utils/Profiler.h"

#include <QFileInfo>
#include <QString>

struct StopWatchItem {
	const char* sourceFile;
	const char* func;
	unsigned int line;
	clock_t startTime;
};

static unsigned int blockCounter = 0;
static std::map<std::string,StopWatchItem> GlobalProfilerMap;
Logger* Profiler::_logger = nullptr;



std::string profiler_getBaseName( std::string sourceFile)
{
	QFileInfo fi( sourceFile.c_str() );
	return fi.fileName().toStdString();
}

double getClockDelta(clock_t start)
{
	return ((double)(clock() - start) / CLOCKS_PER_SEC)	;
}



Profiler::Profiler(const char* sourceFile, const char* func, unsigned int line) :
	_file(sourceFile),
	_func(func),
	_line(line),
	_blockId(blockCounter++),
	_startTime(clock())
{
	Profiler::initLogger();
	_logger->Message(Logger::DEBUG,_file,_func,_line,">>> enter block %d", _blockId);
}


Profiler::~Profiler()
{
  	_logger->Message( Logger::DEBUG, _file,_func, _line, "<<< exit block %d, executed for %f s", _blockId, getClockDelta(_startTime) );
}

void Profiler::initLogger()
{
	if (_logger == nullptr ) 
		_logger = Logger::getInstance("PROFILER", Logger::DEBUG);
}

void Profiler::TimerStart(const std::string timerName, const char* sourceFile, const char* func, unsigned int line)
{
	std::pair<std::map<std::string,StopWatchItem>::iterator,bool> ret;
	Profiler::initLogger();

	StopWatchItem item = {sourceFile, func, line};

	ret = GlobalProfilerMap.emplace(timerName, item);
	if ( ! ret.second )
	{
		if ( ret.first->second.sourceFile == sourceFile && ret.first->second.func == func && ret.first->second.line == line )
		{
			_logger->Message(Logger::DEBUG, sourceFile, func, line, "restart timer '%s'", timerName.c_str() );
			ret.first->second.startTime = clock();
		}
		else
		{
			_logger->Message(Logger::DEBUG, sourceFile, func, line, "ERROR timer '%s' started in multiple locations. First occurence %s:%d:%s()",
			                 timerName.c_str(), profiler_getBaseName(ret.first->second.sourceFile).c_str(), ret.first->second.line, ret.first->second.func );
		}
	}
	else
	{
		_logger->Message(Logger::DEBUG, sourceFile, func, line, "start timer '%s'", timerName.c_str() );
	}
}


void Profiler::TimerGetTime(const std::string timerName, const char* sourceFile, const char* func, unsigned int line)
{
	std::map<std::string,StopWatchItem>::iterator ret = GlobalProfilerMap.find(timerName);
	Profiler::initLogger();
	if (ret != GlobalProfilerMap.end())
	{
		_logger->Message(Logger::DEBUG, sourceFile, func, line, "timer '%s' started at %s:%d:%s() took %f s execution time until here", timerName.c_str(),
		                 profiler_getBaseName(ret->second.sourceFile).c_str(), ret->second.line, ret->second.func, getClockDelta(ret->second.startTime) );
	}
	else
	{
		_logger->Message(Logger::DEBUG, sourceFile, func, line, "ERROR timer '%s' not started", timerName.c_str() );
	}
}

