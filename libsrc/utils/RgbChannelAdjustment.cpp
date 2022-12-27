#include <utils/RgbChannelAdjustment.h>

RgbChannelAdjustment::RgbChannelAdjustment(QString channelName)
	: _channelName(channelName)
	, _log(Logger::getInstance(channelName))
	, _brightness(0)
{
	resetInitialized();
}

RgbChannelAdjustment::RgbChannelAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB, QString channelName)
	: _channelName(channelName)
	, _log(Logger::getInstance(channelName))
{
	setAdjustment(adjustR, adjustG, adjustB);
}

void RgbChannelAdjustment::resetInitialized()
{
	memset(_initialized, false, sizeof(_initialized));
}

void RgbChannelAdjustment::setAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB)
{
	_adjust[RED]   = adjustR;
	_adjust[GREEN] = adjustG;
	_adjust[BLUE]  = adjustB;
	resetInitialized();
}

uint8_t RgbChannelAdjustment::getAdjustmentR() const
{
	return _adjust[RED];
}

uint8_t RgbChannelAdjustment::getAdjustmentG() const
{
	return _adjust[GREEN];
}

uint8_t RgbChannelAdjustment::getAdjustmentB() const
{
	return _adjust[BLUE];
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
		_mapping[RED  ][input] = qMin( ((_brightness * input * _adjust[RED  ]) / 65025), (int)UINT8_MAX);
		_mapping[GREEN][input] = qMin( ((_brightness * input * _adjust[GREEN]) / 65025), (int)UINT8_MAX);
		_mapping[BLUE ][input] = qMin( ((_brightness * input * _adjust[BLUE ]) / 65025), (int)UINT8_MAX);
		_initialized[input] = true;
	}
	red   = _mapping[RED  ][input];
	green = _mapping[GREEN][input];
	blue  = _mapping[BLUE ][input];
}
