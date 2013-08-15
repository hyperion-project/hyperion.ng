
// STL includes
#include <csignal>

// QT includes
#include <QImage>
#include <QTest>

// Hyperion includes
#include <hyperion/DispmanxFrameGrabber.h>

static bool running = true;

void signal_handler(int signum)
{
	running = false;
}

int main()
{
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);

	DispmanxFrameGrabber frameGrabber(64, 64);

	unsigned iFrame = 0;
	QImage qImage(64, 64, QImage::Format_RGB888);
	RgbImage rgbImage(64, 64);

	while(running)
	{
		frameGrabber.grabFrame(rgbImage);

		for (int iScanline=0; iScanline<qImage.height(); ++iScanline)
		{
			unsigned char* scanLinePtr = qImage.scanLine(iScanline);
			memcpy(scanLinePtr, rgbImage.memptr()+rgbImage.width()*iScanline, rgbImage.width()*sizeof(RgbColor));
		}

		qImage.save(QString("HYPERION_%3.png").arg(iFrame));
		++iFrame;

		QTest::qSleep(1000);
	}

	return 0;
}
