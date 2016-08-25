
// STL includes
#include <csignal>
#include <iomanip>

// QT includes
#include <QtGui/QImage>

// Dispmanx grabber includes
#include <grabber/DispmanxFrameGrabber.h>

static bool running = true;

void signal_handler(int signum)
{
	running = false;
}

int main(int argc, char** argv)
{
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);

	int grabFlags = 0;
	int grabCount = -1;
	try
	{
		// create the option parser and initialize all parameters
		Parser parser("Simple application to send a command to hyperion using the Json interface");
		ParameterSet & parameters = optionParser.getParameters();

		QString flagDescr = QString("Set the grab flags of the dispmanx frame grabber [default: 0x%1]").arg(grabFlags, 8, 16, QChar('0'));
		Option   & argFlags = parser.add<Option>   ('f', "flags", flagDescr.toLatin1().constData());
		IntOption      & argCount = parser.add<IntOption>      ('n', "count", "Number of images to capture (default infinite)");
		argCount.setDefault(grabCount);
		Option & argList  = parser.add<Option >('l', "list",  "List the possible flags");
		Option & argHelp  = parser.add<Option >('h', "help",  "Show this help message and exit");

		// parse all options
		optionParser.parse(argc, const_cast<const char **>(argv));

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			optionParser.usage();
			return 0;
		}
		if (parser.isSet(argList))
		{
			std::cout.width(15);
			std::cout.width(10);
			std::cout << "Possible DISPMANX flags: " << std::endl;
			std::cout << "Name                            | " << "Value" << std::endl;
			std::cout << "--------------------------------| " << "------" << std::endl;
			std::cout << "DISPMANX_NO_ROTATE              | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_NO_ROTATE << std::endl;
			std::cout << "DISPMANX_ROTATE_90              | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_ROTATE_90 << std::endl;
			std::cout << "DISPMANX_ROTATE_180             | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_ROTATE_180 << std::endl;
			std::cout << "DISPMANX_ROTATE_270             | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_ROTATE_270 << std::endl;

			std::cout << "DISPMANX_FLIP_HRIZ              | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_FLIP_HRIZ << std::endl;
			std::cout << "DISPMANX_FLIP_VERT              | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_FLIP_VERT << std::endl;

			std::cout << "DISPMANX_SNAPSHOT_NO_YUV        | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_SNAPSHOT_NO_YUV << std::endl;
			std::cout << "DISPMANX_SNAPSHOT_NO_RGB        | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_SNAPSHOT_NO_RGB << std::endl;
			std::cout << "DISPMANX_SNAPSHOT_FILL          | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_SNAPSHOT_FILL << std::endl;
			std::cout << "DISPMANX_SNAPSHOT_SWAP_RED_BLUE | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_SNAPSHOT_SWAP_RED_BLUE << std::endl;
			std::cout << "DISPMANX_SNAPSHOT_PACK          | 0x" << std::hex << std::setfill('0') << std::setw(8) << DISPMANX_SNAPSHOT_PACK << std::endl;
			return 0;
		}
		if (parser.isSet(argFlags))
		{
			QString flagStr = QString::fromStdString(argFlags.getValue());

			bool ok = false;
//			grabFlags = flagStr.toInt(&ok);
			if (flagStr.startsWith("0x"))
			{
				grabFlags = flagStr.toInt(&ok, 16);
			}
			else
			{
				grabFlags = flagStr.toInt(&ok, 10);
			}
			std::cout << "Resulting flags: " << grabFlags << " (=0x" << std::hex << std::setfill('0') << std::setw(8) << grabFlags << ")" << std::dec << std::endl;
			if (!ok)
			{
				std::cerr << "Failed to parse flags (" << flagStr.toStdString().c_str() << ")" << std::endl;
				return -1;
			}
		}

		grabCount = argCount.getValue();
	}
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		std::cerr << e.what() << std::endl;
		return 1;
	}


	DispmanxFrameGrabber frameGrabber(64, 64);
	frameGrabber.setFlags(grabFlags);

	unsigned iFrame = 0;
	QImage qImage(64, 64, QImage::Format_ARGB32);
	Image<ColorRgba> imageRgba(64, 64);

	for (int i=0; i<grabCount || grabCount < 0; ++i)
	{
		frameGrabber.grabFrame(imageRgba);

		for (int iScanline=0; iScanline<qImage.height(); ++iScanline)
		{
			unsigned char* scanLinePtr = qImage.scanLine(iScanline);
			memcpy(scanLinePtr, imageRgba.memptr()+imageRgba.width()*iScanline, imageRgba.width()*sizeof(ColorRgba));
		}

		const QImage qImageSwp = qImage.rgbSwapped();
		qImageSwp.save(QString("HYPERION_f0x%1_%2.png").arg(grabFlags, 8, 16, QChar('0')).arg(iFrame));
		++iFrame;

		timespec sleepTime;
		sleepTime.tv_sec  = 1;
		sleepTime.tv_nsec = 0;
		nanosleep(&sleepTime, NULL);
	}

	return 0;
}

