#include <hyperion/CaptureCont.h>

#include <hyperion/Hyperion.h>

CaptureCont::CaptureCont(Hyperion* hyperion)
	: QObject()
	, _hyperion(hyperion)
	, _systemCaptEnabled(false)
	, _v4lCaptEnabled(false)
{
	// settings changes
	connect(_hyperion, &Hyperion::settingsChanged, this, &CaptureCont::handleSettingsUpdate);

	// comp changes
	connect(_hyperion, &Hyperion::componentStateChanged, this, &CaptureCont::componentStateChanged);

	// init
	handleSettingsUpdate(settings::INSTCAPTURE, _hyperion->getSetting(settings::INSTCAPTURE));
}

CaptureCont::~CaptureCont()
{

}

void CaptureCont::handleV4lImage(const Image<ColorRgb> & image)
{
	_hyperion->setInputImage(_v4lCaptPrio, image);
}

void CaptureCont::handleSystemImage(const Image<ColorRgb>& image)
{
	_hyperion->setInputImage(_systemCaptPrio, image);
}


void CaptureCont::setSystemCaptureEnable(const bool& enable)
{
	if(_systemCaptEnabled != enable)
	{
		if(enable)
		{
			_hyperion->registerInput(_systemCaptPrio, hyperion::COMP_GRABBER, "System", "DoNotKnow");
			connect(_hyperion, &Hyperion::systemImage, this, &CaptureCont::handleSystemImage);
		}
		else
		{
			disconnect(_hyperion, &Hyperion::systemImage, this, &CaptureCont::handleSystemImage);
			_hyperion->clear(_systemCaptPrio);
		}
		_systemCaptEnabled = enable;
		_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_GRABBER, enable);
	}
}

void CaptureCont::setV4LCaptureEnable(const bool& enable)
{
	if(_v4lCaptEnabled != enable)
	{
		if(enable)
		{
			_hyperion->registerInput(_v4lCaptPrio, hyperion::COMP_V4L, "System", "DoNotKnow");
			connect(_hyperion, &Hyperion::v4lImage, this, &CaptureCont::handleV4lImage);
		}
		else
		{
			disconnect(_hyperion, &Hyperion::v4lImage, this, &CaptureCont::handleV4lImage);
			_hyperion->clear(_v4lCaptPrio);
		}
		_v4lCaptEnabled = enable;
		_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_V4L, enable);
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
