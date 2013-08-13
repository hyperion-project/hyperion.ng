
// QT includes
#include <QCoreApplication>

// Hyperion includes
#include <hyperion/DispmanxWrapper.h>

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);


	DispmanxWrapper dispmanx;
	dispmanx.start();

	app.exec();
}
