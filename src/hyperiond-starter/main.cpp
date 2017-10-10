// qt includes
#include <QCoreApplication>
#include <QDebug>

// c++ includes
#include <csignal>

#include "configHandle.h"

void signal_handler(const int signum)
{
	qDebug() << "hyperiond-starter stopped with code:" << signum;
	QCoreApplication::quit();
}

int main(int argc, char** argv)
{
	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);

	// signals
    //signal(SIGINT,  signal_handler);
    //signal(SIGTERM, signal_handler);
    //signal(SIGABRT, signal_handler);
    //signal(SIGCHLD, signal_handler);
    //signal(SIGPIPE, signal_handler);

    // force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

    // initialize configHandle
	QString configPath = QDir::homePath()+"/.hyperion/config/";
	QString daemonPath = app.applicationDirPath()+"/hyperiond";
	configHandle cHandle(configPath, daemonPath);

	return app.exec();
}
