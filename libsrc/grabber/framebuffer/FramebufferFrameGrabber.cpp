#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cstring>

// STL includes
#include <iostream>

//Qt
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QSize>

// Constants
namespace {
const bool verbose = false;

// fb discovery service
const char DISCOVERY_DIRECTORY[] = "/dev/";
const char DISCOVERY_FILEPATTERN[] = "fb?";

} //End of constants

// Local includes
#include <grabber/FramebufferFrameGrabber.h>

FramebufferFrameGrabber::FramebufferFrameGrabber(const QString & device)
	: Grabber("FRAMEBUFFERGRABBER")
	  , _fbDevice(device)
	  , _fbfd (-1)
{
	_useImageResampler = true;
}

FramebufferFrameGrabber::~FramebufferFrameGrabber()
{
	closeDevice();
}

bool FramebufferFrameGrabber::setupScreen()
{
	bool rc (false);

	if ( _fbfd >= 0 )
	{
		closeDevice();
	}

	rc = getScreenInfo();
	setEnabled(rc);

	return rc;
}

bool FramebufferFrameGrabber::setWidthHeight(int width, int height)
{
	bool rc (false);
	if(Grabber::setWidthHeight(width, height))
	{
		rc = setupScreen();
	}
	return rc;
}

int FramebufferFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	int rc = 0;

	if (_isEnabled && !_isDeviceInError)
	{
		if ( getScreenInfo() )
		{
			/* map the device to memory */
			uint8_t * fbp = static_cast<uint8_t*>(mmap(nullptr, _fixInfo.smem_len, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, _fbfd, 0));
			if (fbp == MAP_FAILED) {

				QString errorReason = QString ("Error mapping %1, [%2] %3").arg(_fbDevice).arg(errno).arg(std::strerror(errno));
				this->setInError ( errorReason );
				closeDevice();
				rc = -1;
			}
			else
			{
				_imageResampler.processImage(fbp,
											  static_cast<int>(_varInfo.xres),
											  static_cast<int>(_varInfo.yres),
											  static_cast<int>(_fixInfo.line_length),
											  _pixelFormat,
											  image);
				munmap(fbp, _fixInfo.smem_len);
			}
		}
		closeDevice();
	}
	return rc;
}

bool FramebufferFrameGrabber::openDevice()
{
	bool rc = true;

	/* Open the framebuffer device */
	_fbfd = ::open(QSTRING_CSTR(_fbDevice), O_RDONLY);
	if (_fbfd < 0)
	{
		QString errorReason = QString ("Error opening %1, [%2] %3").arg(_fbDevice).arg(errno).arg(std::strerror(errno));
		this->setInError ( errorReason );
		rc = false;
	}
	return rc;
}

bool FramebufferFrameGrabber::closeDevice()
{
	bool rc = false;
	if (_fbfd >= 0)
	{
		if( ::close(_fbfd) == 0) {
			rc = true;
		}
		_fbfd = -1;
	}
	return rc;
}

QSize FramebufferFrameGrabber::getScreenSize() const
{
	return getScreenSize(_fbDevice);
}

QSize FramebufferFrameGrabber::getScreenSize(const QString& device) const
{
	int width (0);
	int height(0);

	int fbfd = ::open(QSTRING_CSTR(device), O_RDONLY);
	if (fbfd != -1)
	{
		struct fb_var_screeninfo vinfo;
		int result = ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo);
		if (result == 0)
		{
			width = static_cast<int>(vinfo.xres);
			height = static_cast<int>(vinfo.yres);
			DebugIf(verbose, _log, "FB device [%s] found with resolution: %dx%d", QSTRING_CSTR(device), width, height);
		}
		::close(fbfd);
	}
	return QSize(width, height);
}

bool FramebufferFrameGrabber::getScreenInfo()
{
	bool rc (false);

	if ( openDevice() )
	{
		if (ioctl(_fbfd, FBIOGET_FSCREENINFO, &_fixInfo) < 0 || ioctl (_fbfd, FBIOGET_VSCREENINFO, &_varInfo) < 0)
		{
			QString errorReason = QString ("Error getting screen information for %1, [%2] %3").arg(_fbDevice).arg(errno).arg(std::strerror(errno));
			this->setInError ( errorReason );
			closeDevice();
		}
		else
		{
			rc = true;
			switch (_varInfo.bits_per_pixel)
			{
			case 16: _pixelFormat = PixelFormat::BGR16;
				break;
			case 24: _pixelFormat = PixelFormat::BGR24;
				break;
			case 32: _pixelFormat = PixelFormat::BGR32;
				break;
			default:
				rc= false;
				QString errorReason = QString ("Unknown pixel format: %1 bits per pixel").arg(static_cast<int>(_varInfo.bits_per_pixel));
				this->setInError ( errorReason );
				closeDevice();
			}
		}
	}
	return rc;
}

QJsonObject FramebufferFrameGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;

	//Find framebuffer devices 0-9
	QDir deviceDirectory (DISCOVERY_DIRECTORY);
	QStringList deviceFilter(DISCOVERY_FILEPATTERN);
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	int fbIdx (0);
	QJsonArray video_inputs;

	QFileInfoList::const_iterator deviceFileIterator;
	for (deviceFileIterator = deviceFiles.constBegin(); deviceFileIterator != deviceFiles.constEnd(); ++deviceFileIterator)
	{
		fbIdx = (*deviceFileIterator).fileName().right(1).toInt();
		QString device = (*deviceFileIterator).absoluteFilePath();
		DebugIf(verbose, _log, "FB device [%s] found", QSTRING_CSTR(device));

		QSize screenSize = getScreenSize(device);
		if ( !screenSize.isEmpty() )
		{
			QJsonArray fps = { "1", "5", "10", "15", "20", "25", "30", "40", "50", "60" };

			QJsonObject in;

			QString displayName;
			displayName = QString("FB%1").arg(fbIdx);

			in["name"] = displayName;
			in["inputIdx"] = fbIdx;

			QJsonArray formats;
			QJsonObject format;

			QJsonArray resolutionArray;

			QJsonObject resolution;

			resolution["width"] = screenSize.width();
			resolution["height"] = screenSize.height();
			resolution["fps"] = fps;

			resolutionArray.append(resolution);

			format["resolutions"] = resolutionArray;
			formats.append(format);

			in["formats"] = formats;
			video_inputs.append(in);
		}

		if (!video_inputs.isEmpty())
		{
			inputsDiscovered["device"] = "framebuffer";
			inputsDiscovered["device_name"] = "Framebuffer";
			inputsDiscovered["type"] = "screen";
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults, video_inputs_default, resolution_default;
			resolution_default["fps"] = _fps;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}
	}

	if (inputsDiscovered.isEmpty())
	{
		DebugIf(verbose, _log, "No displays found to capture from!");
	}

	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}
