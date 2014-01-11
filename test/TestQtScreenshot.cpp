
// STL includes
#include <iostream>

// QT includes
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>

int main(int argc, char** argv)
{

	QApplication app(argc, argv);

	QPixmap originalPixmap = QPixmap::grabWindow(QApplication::desktop()->winId());

	std::cout << "Grabbed image: [" << originalPixmap.width() << "; " << originalPixmap.height() << "]" << std::endl;

	return 0;
}
