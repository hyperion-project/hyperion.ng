#include <grabber/framebuffer/FramebufferFrameGrabber.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cstring>
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

}; //End of constants

FramebufferFrameGrabber::FramebufferFrameGrabber(int deviceIdx)
	: Grabber("GRABBER-FB")
	, _deviceFd(-1)
{
	_input = deviceIdx;
	_useImageResampler = true;
}

FramebufferFrameGrabber::~FramebufferFrameGrabber()
{
	closeDevice();
}

bool FramebufferFrameGrabber::setupScreen()
{
	if ( _deviceFd >= 0 )
	{
		closeDevice();
	}

	bool success = getScreenInfo();
	setEnabled(success);

	return success;
}

bool FramebufferFrameGrabber::setWidthHeight(int width, int height)
{
	if (Grabber::setWidthHeight(width, height))
	{
		return setupScreen();
	}

	return false;
}

int FramebufferFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_isEnabled || _isDeviceInError)
	{
		return -1;
	}

	if ( !getScreenInfo() )
	{
		return -1;
	}

	/* map the device to memory */
	auto * fbp = static_cast<uint8_t*>(mmap(nullptr, _fixInfo.smem_len, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, _deviceFd, 0));
	if (fbp == MAP_FAILED)
	{

		QString errorReason = QString ("Error mapping %1, [%2] %3").arg(getDeviceName()).arg(errno).arg(std::strerror(errno));
		this->setInError ( errorReason );
		closeDevice();
		return -1;
	}

	_imageResampler.processImage(fbp,
									static_cast<int>(_varInfo.xres),
									static_cast<int>(_varInfo.yres),
									static_cast<int>(_fixInfo.line_length),
									_pixelFormat,
									image);
	munmap(fbp, _fixInfo.smem_len);

	closeDevice();

	return 0;
}

bool FramebufferFrameGrabber::openDevice()
{
	/* Open the framebuffer device */
	_deviceFd = ::open(QSTRING_CSTR(getDeviceName()), O_RDONLY);
	if (_deviceFd < 0)
	{
		QString errorReason = QString ("Error opening %1, [%2] %3").arg(getDeviceName()).arg(errno).arg(std::strerror(errno));
		this->setInError ( errorReason );
		return false;
	}
	return true;
}

bool FramebufferFrameGrabber::closeDevice()
{
	if (_deviceFd < 0)
	{
		return true;
	}

	bool success = (::close(_deviceFd) == 0);
	_deviceFd = -1;

	return success;
}

QSize FramebufferFrameGrabber::getScreenSize() const
{
	return getScreenSize(getDeviceName());
}

QSize FramebufferFrameGrabber::getScreenSize(const QString& device) const
{
	int fbfd = ::open(QSTRING_CSTR(device), O_RDONLY);
	if (fbfd == -1)
	{
		return {};
	}

	QSize size;
	struct fb_var_screeninfo vinfo;
	int result = ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo);
	if (result == 0)
	{
		size.setWidth(static_cast<int>(vinfo.xres));
		size.setHeight(static_cast<int>(vinfo.yres));
		DebugIf(verbose, _log, "FB device [%s] found with resolution: %dx%d", QSTRING_CSTR(device), size.width(), size.height());
	}
	::close(fbfd);

	return size;
}

bool FramebufferFrameGrabber::getScreenInfo()
{
	if ( !openDevice() )
	{
		return false;
	}

	if (ioctl(_deviceFd, FBIOGET_FSCREENINFO, &_fixInfo) < 0 || ioctl (_deviceFd, FBIOGET_VSCREENINFO, &_varInfo) < 0)
	{
		QString errorReason = QString ("Error getting screen information for %1, [%2] %3").arg(getDeviceName()).arg(errno).arg(std::strerror(errno));
		this->setInError ( errorReason );
		closeDevice();
		return false;
	}

	switch (_varInfo.bits_per_pixel)
	{
	case 16: _pixelFormat = PixelFormat::BGR16;
		break;
	case 24: _pixelFormat = PixelFormat::BGR24;
		break;
	case 32: _pixelFormat = PixelFormat::BGR32;
		break;
	default:
		QString errorReason = QString ("Unknown pixel format: %1 bits per pixel").arg(static_cast<int>(_varInfo.bits_per_pixel));
		this->setInError ( errorReason );
		closeDevice();
		return false;
	}

	return true;
}

QJsonArray FramebufferFrameGrabber::getInputDeviceDetails() const
{
	//Find framebuffer devices 0-9
	QDir deviceDirectory (DISCOVERY_DIRECTORY);
	QStringList deviceFilter(DISCOVERY_FILEPATTERN);
	deviceDirectory.setNameFilters(deviceFilter);
	deviceDirectory.setSorting(QDir::Name);
	QFileInfoList deviceFiles = deviceDirectory.entryInfoList(QDir::System);

	QJsonArray video_inputs;
	for (const auto &deviceFile : deviceFiles)
	{
		QString const fileName = deviceFile.fileName();
		int deviceIdx = fileName.right(1).toInt();
		QString device = deviceFile.absoluteFilePath();

		DebugIf(verbose, _log, "FB device [%s] found", QSTRING_CSTR(device));

		QSize screenSize = getScreenSize(device);
		if ( !screenSize.isEmpty() )
		{
			QJsonArray fps = { "1", "5", "10", "15", "20", "25", "30", "40", "50", "60" };

			QJsonObject in;

			QString displayName;
			displayName = QString("FB%1").arg(deviceIdx);

			in["name"] = displayName;
			in["inputIdx"] = deviceIdx;

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
	}

	return video_inputs;
}

QJsonObject FramebufferFrameGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;

	QJsonArray const video_inputs = getInputDeviceDetails();
	if (video_inputs.isEmpty())
	{
		DebugIf(verbose, _log, "No displays found to capture from!");
		return {};
	}

	if (!video_inputs.isEmpty())
	{
		inputsDiscovered["device"] = "framebuffer";
		inputsDiscovered["device_name"] = "Framebuffer";
		inputsDiscovered["type"] = "screen";
		inputsDiscovered["video_inputs"] = video_inputs;

		QJsonObject defaults;
		QJsonObject video_inputs_default;
		QJsonObject resolution_default;
		resolution_default["fps"] = _fps;
		video_inputs_default["resolution"] = resolution_default;
		video_inputs_default["inputIdx"] = 0;
		defaults["video_input"] = video_inputs_default;
		inputsDiscovered["default"] = defaults;
	}

	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}
