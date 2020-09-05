#ifdef _WIN32
#include <utils/Logger.h>
#include <QString>
#include <QByteArray>
namespace Process {

void restartHyperion(bool asNewProcess) {}

QByteArray command_exec(const QString& /*cmd*/, const QByteArray& /*data*/)
{
	return QSTRING_CSTR(QString());
}
};

#else

#include <utils/Process.h>
#include <utils/Logger.h>

#include <QCoreApplication>
#include <QProcess>
#include <QStringList>

#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace Process {

void restartHyperion(bool asNewProcess)
{
	Logger* log = Logger::getInstance("Process");
	Info(log, "Restarting hyperion ...");

	std::cout << std::endl
		<< "      *******************************************" << std::endl
		<< "      *      hyperion will restart now          *" << std::endl
		<< "      *******************************************" << std::endl << std::endl;

	auto arguments = QCoreApplication::arguments();
	if (!arguments.contains("--wait-hyperion"))
		arguments << "--wait-hyperion";

	QProcess::startDetached(QCoreApplication::applicationFilePath(), arguments);

	QCoreApplication::quit();
}

QByteArray command_exec(const QString& cmd, const QByteArray& data)
{
	char buffer[128];
	QString result;

	std::shared_ptr<FILE> pipe(popen(cmd.toLocal8Bit().constData(), "r"), pclose);
	if (pipe)
	{
		while (!feof(pipe.get()))
		{
			if (fgets(buffer, 128, pipe.get()) != NULL)
				result += buffer;
		}
	}
	return QSTRING_CSTR(result);
}

};

#endif
