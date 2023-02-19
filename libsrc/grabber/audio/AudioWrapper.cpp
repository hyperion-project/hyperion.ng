#include <grabber/AudioWrapper.h>
#include <hyperion/GrabberWrapper.h>
#include <QObject>
#include <QMetaType>

AudioWrapper::AudioWrapper()
	: GrabberWrapper("AudioGrabber", &_grabber)
	, _grabber()
{
	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	connect(&_grabber, &AudioGrabber::newFrame, this, &AudioWrapper::newFrame, Qt::DirectConnection);
}

AudioWrapper::~AudioWrapper()
{
	stop();
}

bool AudioWrapper::start()
{
	return (_grabber.start() && GrabberWrapper::start());
}

void AudioWrapper::stop()
{
	_grabber.stop();
	GrabberWrapper::stop();
}

void AudioWrapper::action()
{
	// Dummy we will push the audio images
}

void AudioWrapper::newFrame(const Image<ColorRgb>& image)
{
	emit systemImage(_grabberName, image);
}

void AudioWrapper::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::AUDIO)
	{
		const QJsonObject& obj = config.object();

		// set global grabber state
		setAudioGrabberState(obj["enable"].toBool(false));

		if (getAudioGrabberState())
		{
			_grabber.setDevice(obj["device"].toString());
			_grabber.setConfiguration(obj);

			_grabber.restart();
		}
		else
		{
			stop();
		}
	}
}
