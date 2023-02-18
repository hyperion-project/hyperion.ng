#include <grabber/AudioGrabber.h>
#include <math.h>
#include <QImage>
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

// Constants
namespace {
	const uint16_t RESOLUTION = 255;
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
namespace QColorConstants
{
	const QColor Black  = QColor(0xFF, 0x00, 0x00);
	const QColor Red    = QColor(0xFF, 0x00, 0x00);
	const QColor Green  = QColor(0x00, 0xFF, 0x00);
	const QColor Blue   = QColor(0x00, 0x00, 0xFF);
	const QColor Yellow = QColor(0xFF, 0xFF, 0x00);
}
#endif
 //End of constants

AudioGrabber::AudioGrabber()
	: Grabber("AudioGrabber")
	, _deviceProperties()
	, _device("none")
	, _hotColor(QColorConstants::Red)
	, _warnValue(80)
	, _warnColor(QColorConstants::Yellow)
	, _safeValue(45)
	, _safeColor(QColorConstants::Green)
	, _multiplier(0)
	, _tolerance(20)
	, _dynamicMultiplier(INT16_MAX)
	, _started(false)
{
}

AudioGrabber::~AudioGrabber()
{
	freeResources();
}

void AudioGrabber::freeResources()
{
}

void AudioGrabber::setDevice(const QString& device)
{
	_device = device;

	if (_started)
	{
		this->stop();
		this->start();
	}
}

void AudioGrabber::setConfiguration(const QJsonObject& config)
{
	QJsonArray hotColorArray = config["hotColor"].toArray(QJsonArray::fromVariantList(QList<QVariant>({ QVariant(255), QVariant(0), QVariant(0) })));
	QJsonArray warnColorArray = config["warnColor"].toArray(QJsonArray::fromVariantList(QList<QVariant>({ QVariant(255), QVariant(255), QVariant(0) })));
	QJsonArray safeColorArray = config["safeColor"].toArray(QJsonArray::fromVariantList(QList<QVariant>({ QVariant(0), QVariant(255), QVariant(0) })));

	_hotColor = QColor(hotColorArray.at(0).toInt(), hotColorArray.at(1).toInt(), hotColorArray.at(2).toInt());
	_warnColor = QColor(warnColorArray.at(0).toInt(), warnColorArray.at(1).toInt(), warnColorArray.at(2).toInt());
	_safeColor = QColor(safeColorArray.at(0).toInt(), safeColorArray.at(1).toInt(), safeColorArray.at(2).toInt());

	_warnValue = config["warnValue"].toInt(80);
	_safeValue = config["safeValue"].toInt(45);
	_multiplier = config["multiplier"].toDouble(0);
	_tolerance = config["tolerance"].toInt(20);
}

void AudioGrabber::resetMultiplier()
{
	_dynamicMultiplier = INT16_MAX;
}

void AudioGrabber::processAudioFrame(int16_t* buffer, int length)
{
	// Apply Visualizer and Construct Image

	// TODO: Pass Audio Frame to python and let the script calculate the image.

	// TODO: Support Stereo capture with different meters per side

	// Default VUMeter - Later Make this pluggable for different audio effects

	double averageAmplitude = 0;
	// Calculate the the average amplitude value in the buffer
	for (int i = 0; i < length; i++)
	{
		averageAmplitude += fabs(buffer[i]) / length;
	}

	double * currentMultiplier;

	if (_multiplier < std::numeric_limits<double>::epsilon())
	{
		// Dynamically calculate multiplier.
		const double pendingMultiplier = INT16_MAX / fmax(1.0, averageAmplitude + ((_tolerance / 100.0) * averageAmplitude));

		if (pendingMultiplier < _dynamicMultiplier)
			_dynamicMultiplier = pendingMultiplier;

		currentMultiplier = &_dynamicMultiplier;
	}
	else
	{
		// User defined multiplier
		currentMultiplier = &_multiplier;
	}

	// Apply multiplier to average amplitude
	const double result = averageAmplitude * (*currentMultiplier);

	// Calculate the average percentage
	const double percentage = fmin(result / INT16_MAX, 1);

	// Calculate the value
	const int value = static_cast<int>(ceil(percentage * RESOLUTION));

	// Draw Image
	QImage image(1, RESOLUTION, QImage::Format_RGB888);
	image.fill(QColorConstants::Black);

	int safePixelValue = static_cast<int>(round(( _safeValue / 100.0) * RESOLUTION));
	int warnPixelValue = static_cast<int>(round(( _warnValue / 100.0) * RESOLUTION));

	for (int i = 0; i < RESOLUTION; i++)
	{
		QColor color = QColorConstants::Black;
		int position = RESOLUTION - i;

		if (position < safePixelValue)
		{
			color = _safeColor;
		}
		else if (position < warnPixelValue)
		{
			color = _warnColor;
		}
		else
		{
			color = _hotColor;
		}

		if (position < value)
		{
			image.setPixelColor(0, i, color);
		}
		else
		{
			image.setPixelColor(0, i, QColorConstants::Black);
		}
	}

	// Convert to Image<ColorRGB>
	Image<ColorRgb> finalImage (static_cast<unsigned>(image.width()), static_cast<unsigned>(image.height()));
	for (int y = 0; y < image.height(); y++)
	{
		memcpy((unsigned char*)finalImage.memptr() + y * image.width() * 3, static_cast<unsigned char*>(image.scanLine(y)), image.width() * 3);
	}

	emit newFrame(finalImage);
}

Logger* AudioGrabber::getLog()
{
	return _log;
}

bool AudioGrabber::start()
{
	resetMultiplier();

	_started = true;

	return true;
}

void AudioGrabber::stop()
{
	_started = false;
}

void AudioGrabber::restart()
{
	stop();
	start();
}

QJsonArray AudioGrabber::discover(const QJsonObject& /*params*/)
{
	QJsonArray result; // Return empty result
	return result;
}
