#include <utils/RgbChannelAdjustment.h>


RgbChannelAdjustment::RgbChannelAdjustment(const QString& channelName)
	: RgbChannelAdjustment(0, 0, 0, channelName)
{
}

RgbChannelAdjustment::RgbChannelAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB, const QString& channelName )
	: RgbChannelAdjustment({adjustR, adjustG, adjustB}, channelName)
{
}

RgbChannelAdjustment::RgbChannelAdjustment(const ColorRgb& adjust, const QString& channelName )
	: _channelName(channelName)
	, _log(Logger::getInstance("CHANNEL_" + channelName.toUpper()))
	, _mapping{ {0}, {0}, {0} }
	, _brightness(0)
{
	setAdjustment(adjust);
}

void RgbChannelAdjustment::resetInitialized()
{
	memset(_initialized, 0, sizeof(_initialized));
}

void RgbChannelAdjustment::setAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB)
{
	setAdjustment( {adjustR, adjustG, adjustB} );
}

void RgbChannelAdjustment::setAdjustment(const ColorRgb& adjust)
{
	_adjust = adjust;
	resetInitialized();
}

uint8_t RgbChannelAdjustment::getAdjustmentR() const
{
	return _adjust.red;
}

uint8_t RgbChannelAdjustment::getAdjustmentG() const
{
	return _adjust.green;
}

uint8_t RgbChannelAdjustment::getAdjustmentB() const
{
	return _adjust.blue;
}

void RgbChannelAdjustment::apply(uint8_t input, uint8_t brightness, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	if (_brightness != brightness)
	{
		_brightness = brightness;
		resetInitialized();
	}

	if (!_initialized[input])
	{
		const double adjustedInput = _brightness * input / DOUBLE_UINT8_MAX_SQUARED;
		_mapping.red[input] = static_cast<quint8>(qBound(0, static_cast<int>(_adjust.red * adjustedInput), static_cast<int>(UINT8_MAX)));
		_mapping.green[input] = static_cast<quint8>(qBound(0 ,static_cast<int>(_adjust.green * adjustedInput), static_cast<int>(UINT8_MAX)));
		_mapping.blue[input] = static_cast<quint8>(qBound(0, static_cast<int>(_adjust.blue * adjustedInput), static_cast<int>(UINT8_MAX)));
		_initialized[input] = true;
	}
	red   = _mapping.red[input];
	green = _mapping.green[input];
	blue  = _mapping.blue[input];
}
