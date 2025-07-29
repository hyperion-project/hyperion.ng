#include <hyperion/CaptureCont.h>

#include <chrono>

// hyperion includes
#include <hyperion/Hyperion.h>

// utils includes
#include <utils/GlobalSignals.h>

// qt includes
#include <QTimer>

namespace {
const int DEFAULT_VIDEO_CAPTURE_PRIORITY = 240;
const int DEFAULT_SCREEN_CAPTURE_PRIORITY = 250;
const int DEFAULT_AUDIO_CAPTURE_PRIORITY = 230;

constexpr std::chrono::seconds DEFAULT_VIDEO_CAPTURE_INACTIVE_TIMEOUT{1};
constexpr std::chrono::seconds DEFAULT_SCREEN_CAPTURE_INACTIVE_TIMEOUT{5};
constexpr std::chrono::seconds DEFAULT_AUDIO_CAPTURE_INACTIVE_TIMEOUT{1};
}

CaptureCont::CaptureCont(const QSharedPointer<Hyperion>& hyperionInstance)
	: _hyperionWeak(hyperionInstance)
	, _screenCaptureEnabled(false)
	, _screenCapturePriority(0)
	, _screenCaptureInactiveTimer(nullptr)
	, _videoCaptureEnabled(false)
	, _videoCapturePriority(0)
	, _videoInactiveTimer(nullptr)
	, _audioCaptureEnabled(false)
	, _audioCapturePriority(0)
	, _audioCaptureInactiveTimer(nullptr)
{
}

CaptureCont::~CaptureCont()
{
	qDebug() << "CaptureCont::~CaptureCont()...";
}

void CaptureCont::start()
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion)
	{
		// settings changes
		connect(hyperion.get(), &Hyperion::settingsChanged, this, &CaptureCont::handleSettingsUpdate);

		// comp changes
		connect(hyperion.get(), &Hyperion::compStateChangeRequest, this, &CaptureCont::handleCompStateChangeRequest);


		// inactive timer screen
		_screenCaptureInactiveTimer.reset(new QTimer(this));
		connect(_screenCaptureInactiveTimer.get(), &QTimer::timeout, this, &CaptureCont::onScreenIsInactive);
		_screenCaptureInactiveTimer->setSingleShot(true);
		_screenCaptureInactiveTimer->setInterval(DEFAULT_SCREEN_CAPTURE_INACTIVE_TIMEOUT);

		// inactive timer video
		_videoInactiveTimer.reset(new QTimer(this));
		connect(_videoInactiveTimer.get(), &QTimer::timeout, this, &CaptureCont::onVideoIsInactive);
		_videoInactiveTimer->setSingleShot(true);
		_videoInactiveTimer->setInterval(DEFAULT_VIDEO_CAPTURE_INACTIVE_TIMEOUT);

		// inactive timer audio
		_audioCaptureInactiveTimer.reset(new QTimer(this));
		connect(_audioCaptureInactiveTimer.get(), &QTimer::timeout, this, &CaptureCont::onAudioIsInactive);
		_audioCaptureInactiveTimer->setSingleShot(true);
		_audioCaptureInactiveTimer->setInterval(DEFAULT_AUDIO_CAPTURE_INACTIVE_TIMEOUT);

		// init
		handleSettingsUpdate(settings::INSTCAPTURE, hyperion->getSetting(settings::INSTCAPTURE));
	}
}

void CaptureCont::stop()
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion)
	{
		disconnect(hyperion.get(), &Hyperion::compStateChangeRequest, this, &CaptureCont::handleCompStateChangeRequest);
		disconnect(hyperion.get(), &Hyperion::settingsChanged, this, &CaptureCont::handleSettingsUpdate);
	}

	_videoInactiveTimer->stop();
	_screenCaptureInactiveTimer->stop();
	_audioCaptureInactiveTimer->stop();

	disconnect(_videoInactiveTimer.get(), &QTimer::timeout, this, &CaptureCont::onVideoIsInactive);
	disconnect(_screenCaptureInactiveTimer.get(), &QTimer::timeout, this, &CaptureCont::onScreenIsInactive);
	disconnect(_audioCaptureInactiveTimer.get(), &QTimer::timeout, this, &CaptureCont::onAudioIsInactive);
}

void CaptureCont::handleVideoImage(const QString& name, const Image<ColorRgb> & image)
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if(_videoCaptureName != name)
	{
		hyperion->registerInput(_videoCapturePriority, hyperion::COMP_V4L, "System", name);
		_videoCaptureName = name;
		emit GlobalSignals::getInstance()->requestSource(hyperion::COMP_V4L, int(hyperion->getInstanceIndex()), _videoCaptureEnabled);
	}
	_videoInactiveTimer->start();
	hyperion->setInputImage(_videoCapturePriority, image);
}

void CaptureCont::handleScreenImage(const QString& name, const Image<ColorRgb>& image)
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if(_screenCaptureName != name)
	{
		hyperion->registerInput(_screenCapturePriority, hyperion::COMP_GRABBER, "System", name);
		_screenCaptureName = name;
		emit GlobalSignals::getInstance()->requestSource(hyperion::COMP_GRABBER, int(hyperion->getInstanceIndex()), _screenCaptureEnabled);
	}
	_screenCaptureInactiveTimer->start();
	hyperion->setInputImage(_screenCapturePriority, image);
}

void CaptureCont::handleAudioImage(const QString& name, const Image<ColorRgb>& image)
{
	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (_audioCaptureName != name)
	{
		hyperion->registerInput(_audioCapturePriority, hyperion::COMP_AUDIO, "System", name);
		_audioCaptureName = name;
	}
	_audioCaptureInactiveTimer->start();
	hyperion->setInputImage(_audioCapturePriority, image);
}

void CaptureCont::setScreenCaptureEnable(bool enable)
{
	if(_screenCaptureEnabled != enable)
	{
		QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
		if(enable)
		{
			hyperion->registerInput(_screenCapturePriority, hyperion::COMP_GRABBER);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setSystemImage, this, &CaptureCont::handleScreenImage);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setSystemImage, hyperion.get(), &Hyperion::forwardSystemProtoMessage);
		}
		else
		{
			disconnect(GlobalSignals::getInstance(), &GlobalSignals::setSystemImage, this, nullptr);
			hyperion->clear(_screenCapturePriority);
			_screenCaptureInactiveTimer->stop();
			_screenCaptureName = "";
		}
		_screenCaptureEnabled = enable;
		hyperion->setNewComponentState(hyperion::COMP_GRABBER, enable);
		emit GlobalSignals::getInstance()->requestSource(hyperion::COMP_GRABBER, int(hyperion->getInstanceIndex()), enable);
	}
}

void CaptureCont::setVideoCaptureEnable(bool enable)
{
	if(_videoCaptureEnabled != enable)
	{
		QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
		if(enable)
		{
			hyperion->registerInput(_videoCapturePriority, hyperion::COMP_V4L);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setV4lImage, this, &CaptureCont::handleVideoImage);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setV4lImage, hyperion.get(), &Hyperion::forwardV4lProtoMessage);
		}
		else
		{
			disconnect(GlobalSignals::getInstance(), &GlobalSignals::setV4lImage, this, nullptr);
			hyperion->clear(_videoCapturePriority);
			_videoInactiveTimer->stop();
			_videoCaptureName = "";
		}
		_videoCaptureEnabled = enable;
		hyperion->setNewComponentState(hyperion::COMP_V4L, enable);
		emit GlobalSignals::getInstance()->requestSource(hyperion::COMP_V4L, int(hyperion->getInstanceIndex()), enable);
	}
}

void CaptureCont::setAudioCaptureEnable(bool enable)
{
	if (_audioCaptureEnabled != enable)
	{
		QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
		if (enable)
		{
			hyperion->registerInput(_audioCapturePriority, hyperion::COMP_AUDIO);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setAudioImage, this, &CaptureCont::handleAudioImage);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setAudioImage, hyperion.get(), &Hyperion::forwardAudioProtoMessage);
		}
		else
		{
			disconnect(GlobalSignals::getInstance(), &GlobalSignals::setAudioImage, this, nullptr);
			hyperion->clear(_audioCapturePriority);
			_audioCaptureInactiveTimer->stop();
			_audioCaptureName = "";
		}
		_audioCaptureEnabled = enable;
		hyperion->setNewComponentState(hyperion::COMP_AUDIO, enable);
		emit GlobalSignals::getInstance()->requestSource(hyperion::COMP_AUDIO, int(hyperion->getInstanceIndex()), enable);
	}
}

void CaptureCont::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if(type == settings::INSTCAPTURE)
	{
		const QJsonObject& obj = config.object();

		int videoCapturePriority = obj["v4lPriority"].toInt(DEFAULT_VIDEO_CAPTURE_PRIORITY);
		if(_videoCapturePriority != videoCapturePriority)
		{
			setVideoCaptureEnable(false); // clear prio
			_videoCapturePriority = videoCapturePriority;
		}

		std::chrono::milliseconds videoCaptureInactiveTimeout = static_cast<std::chrono::milliseconds>(obj["videoInactiveTimeout"].toInt(DEFAULT_VIDEO_CAPTURE_INACTIVE_TIMEOUT.count()) * 1000);
		if(_videoInactiveTimer->intervalAsDuration() != videoCaptureInactiveTimeout)
		{
			_videoInactiveTimer->setInterval(videoCaptureInactiveTimeout);
		}

		int screenCapturePriority = obj["systemPriority"].toInt(DEFAULT_SCREEN_CAPTURE_PRIORITY);
		if(_screenCapturePriority != screenCapturePriority)
		{
			setScreenCaptureEnable(false); // clear prio
			_screenCapturePriority = screenCapturePriority;
		}

		std::chrono::milliseconds screenCaptureInactiveTimeout =  static_cast<std::chrono::milliseconds>(obj["screenInactiveTimeout"].toInt(DEFAULT_SCREEN_CAPTURE_INACTIVE_TIMEOUT.count()) * 1000);
		if(_screenCaptureInactiveTimer->intervalAsDuration() != screenCaptureInactiveTimeout)
		{
			_screenCaptureInactiveTimer->setInterval(screenCaptureInactiveTimeout);
		}

		int autoCapturePriority = obj["audioPriority"].toInt(DEFAULT_AUDIO_CAPTURE_PRIORITY);
		if (_audioCapturePriority != autoCapturePriority)
		{
			setAudioCaptureEnable(false); // clear prio
			_audioCapturePriority = autoCapturePriority;
		}

		std::chrono::milliseconds audioCaptureInactiveTimeout =  static_cast<std::chrono::milliseconds>(obj["audioInactiveTimeout"].toInt(DEFAULT_AUDIO_CAPTURE_INACTIVE_TIMEOUT.count()) * 1000);
		if(_audioCaptureInactiveTimer->intervalAsDuration() != audioCaptureInactiveTimeout)
		{
			_audioCaptureInactiveTimer->setInterval(audioCaptureInactiveTimeout);
		}

		setVideoCaptureEnable(obj["v4lEnable"].toBool(false));
		setScreenCaptureEnable(obj["systemEnable"].toBool(false));
		setAudioCaptureEnable(obj["audioEnable"].toBool(true));
	}
}

void CaptureCont::handleCompStateChangeRequest(hyperion::Components component, bool enable)
{
	if(component == hyperion::COMP_GRABBER)
	{
		setScreenCaptureEnable(enable);
	}
	else if(component == hyperion::COMP_V4L)
	{
		setVideoCaptureEnable(enable);
	}
	else if (component == hyperion::COMP_AUDIO)
	{
		setAudioCaptureEnable(enable);
	}
}

void CaptureCont::onVideoIsInactive()
{
	_hyperionWeak.toStrongRef()->setInputInactive(_videoCapturePriority);
}

void CaptureCont::onScreenIsInactive()
{
	_hyperionWeak.toStrongRef()->setInputInactive(_screenCapturePriority);
}

void CaptureCont::onAudioIsInactive()
{
	_hyperionWeak.toStrongRef()->setInputInactive(_audioCapturePriority);
}
