#include <utils/Process.h>
#include <utils/Logger.h>

#include <QCoreApplication>
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
	std::cout << std::endl
		<< "      *******************************************" << std::endl
		<< "      *      hyperion will restart now          *" << std::endl
		<< "      *******************************************" << std::endl << std::endl;


	QStringList qargs = QCoreApplication::arguments();
	int size = qargs.size();
	char *args[size+1];
	args[size] = nullptr;
	for(int i=0; i<size; i++)
	{
		int str_size = qargs[i].toLocal8Bit().size();
		args[i] = new char[str_size+1];
		strncpy(args[i], qargs[i].toLocal8Bit().constData(),str_size );
		args[i][str_size] = '\0';
	}

	execv(args[0],args);
	Error(log, "error while restarting hyperion");
}

QByteArray command_exec(QString cmd, QByteArray data)
{
	char buffer[128];
	QString result = "";

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
