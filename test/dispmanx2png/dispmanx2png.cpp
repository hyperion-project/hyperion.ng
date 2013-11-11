
// STL includes
#include <csignal>

// QT includes
#include <QImage>
#include <QTest>

// Dispmanx grabber includes
#include <dispmanx-grabber/DispmanxFrameGrabber.h>

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
	frameGrabber.setFlags(DISPMANX_SNAPSHOT_NO_RGB|DISPMANX_SNAPSHOT_FILL);

	unsigned iFrame = 0;
	QImage qImage(64, 64, QImage::Format_ARGB32);
	Image<ColorRgba> imageRgba(64, 64);

	while(running)
	{
		frameGrabber.grabFrame(imageRgba);

		for (int iScanline=0; iScanline<qImage.height(); ++iScanline)
		{
			unsigned char* scanLinePtr = qImage.scanLine(iScanline);
			memcpy(scanLinePtr, imageRgba.memptr()+imageRgba.width()*iScanline, imageRgba.width()*sizeof(ColorRgba));
		}

		qImage.save(QString("HYPERION_%3.png").arg(iFrame));
		++iFrame;

		QTest::qSleep(1000);
	}

	return 0;
}
