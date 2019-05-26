#include <hyperion/CaptureCont.h>

// hyperion includes
#include <hyperion/Hyperion.h>

// utils includes
#include <utils/GlobalSignals.h>

// qt includes
#include <QTimer>

CaptureCont::CaptureCont(Hyperion* hyperion)
	: QObject()
	, _hyperion(hyperion)
	, _systemCaptEnabled(false)
	, _systemCaptName()
	, _systemInactiveTimer(new QTimer(this))
	, _v4lCaptEnabled(false)
	, _v4lCaptName()
	, _v4lInactiveTimer(new QTimer(this))
{
	// settings changes
	connect(_hyperion, &Hyperion::settingsChanged, this, &CaptureCont::handleSettingsUpdate);

	// comp changes
	connect(_hyperion, &Hyperion::componentStateChanged, this, &CaptureCont::componentStateChanged);

	// inactive timer system
	connect(_systemInactiveTimer, &QTimer::timeout, this, &CaptureCont::setSystemInactive);
	_systemInactiveTimer->setSingleShot(true);
	_systemInactiveTimer->setInterval(5000);

	// inactive timer v4l
	connect(_v4lInactiveTimer, &QTimer::timeout, this, &CaptureCont::setV4lInactive);
	_v4lInactiveTimer->setSingleShot(true);
	_v4lInactiveTimer->setInterval(1000);

	// init
	handleSettingsUpdate(settings::INSTCAPTURE, _hyperion->getSetting(settings::INSTCAPTURE));
}

CaptureCont::~CaptureCont()
{
}

void CaptureCont::handleV4lImage(const QString& name, const Image<ColorRgb> & image)
{
	if(_v4lCaptName != name)
	{
		_hyperion->registerInput(_v4lCaptPrio, hyperion::COMP_V4L, "System", name);
		_v4lCaptName = name;
	}
	_v4lInactiveTimer->start();
	_hyperion->setInputImage(_v4lCaptPrio, image);
}

void CaptureCont::handleSystemImage(const QString& name, const Image<ColorRgb>& image)
{
	if(_systemCaptName != name)
	{
		_hyperion->registerInput(_systemCaptPrio, hyperion::COMP_GRABBER, "System", name);
		_systemCaptName = name;
	}
	_systemInactiveTimer->start();
	_hyperion->setInputImage(_systemCaptPrio, image);
}

void CaptureCont::setSystemCaptureEnable(const bool& enable)
{
	if(_systemCaptEnabled != enable)
	{
		if(enable)
		{
			_hyperion->registerInput(_systemCaptPrio, hyperion::COMP_GRABBER);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setSystemImage, this, &CaptureCont::handleSystemImage);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setSystemImage, _hyperion, &Hyperion::forwardProtoMessage);
		}
		else
		{
			disconnect(GlobalSignals::getInstance(), &GlobalSignals::setSystemImage, 0, 0);
			_hyperion->clear(_systemCaptPrio);
		}
		_systemCaptEnabled = enable;
		_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_GRABBER, enable);
		_hyperion->setComponentState(hyperion::COMP_GRABBER, enable);
	}
}

void CaptureCont::setV4LCaptureEnable(const bool& enable)
{
	if(_v4lCaptEnabled != enable)
	{
		if(enable)
		{
			_hyperion->registerInput(_v4lCaptPrio, hyperion::COMP_V4L);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setV4lImage, this, &CaptureCont::handleV4lImage);
			connect(GlobalSignals::getInstance(), &GlobalSignals::setV4lImage, _hyperion, &Hyperion::forwardProtoMessage);
		}
		else
		{
			disconnect(GlobalSignals::getInstance(), &GlobalSignals::setV4lImage, 0, 0);
			_hyperion->clear(_v4lCaptPrio);
			_v4lInactiveTimer->stop();
		}
		_v4lCaptEnabled = enable;
		_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_V4L, enable);
		_hyperion->setComponentState(hyperion::COMP_V4L, enable);
	}
}

void CaptureCont::handleSettingsUpdate(const settings::type& type, const QJsonDocument& config)
{
	if(type == settings::INSTCAPTURE)
	{
		const QJsonObject& obj = config.object();
		if(_v4lCaptPrio != obj["v4lPriority"].toInt(240))
		{
			setV4LCaptureEnable(false); // clear prio
			_v4lCaptPrio = obj["v4lPriority"].toInt(240);
		}
		if(_systemCaptPrio != obj["systemPriority"].toInt(250))
		{
			setSystemCaptureEnable(false); // clear prio
			_systemCaptPrio = obj["systemPriority"].toInt(250);
		}

		setV4LCaptureEnable(obj["v4lEnable"].toBool(true));
		setSystemCaptureEnable(obj["systemEnable"].toBool(true));
	}
}

void CaptureCont::componentStateChanged(const hyperion::Components component, bool enable)
{
	if(component == hyperion::COMP_GRABBER)
	{
		setSystemCaptureEnable(enable);
	}
	else if(component == hyperion::COMP_V4L)
	{
		setV4LCaptureEnable(enable);
	}
}

void CaptureCont::setV4lInactive()
{
	_hyperion->setInputInactive(_v4lCaptPrio);
}

void CaptureCont::setSystemInactive()
{
	_hyperion->setInputInactive(_systemCaptPrio);
}
