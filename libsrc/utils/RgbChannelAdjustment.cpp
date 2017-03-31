// STL includes
#include <cmath>
#include <cstdint>
#include <algorithm>

// Utils includes
#include <utils/RgbChannelAdjustment.h>

RgbChannelAdjustment::RgbChannelAdjustment(QString channelName)
	: _channelName(channelName)
	, _log(Logger::getInstance(channelName))
{
	//setAdjustment(UINT8_MAX, UINT8_MAX, UINT8_MAX);
}

RgbChannelAdjustment::RgbChannelAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB, QString channelName)
	: _channelName(channelName)
	, _log(Logger::getInstance(channelName))
{
	setAdjustment(adjustR, adjustG, adjustB);
}

RgbChannelAdjustment::~RgbChannelAdjustment()
{
}

void RgbChannelAdjustment::setAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB)
{
	_adjust[RED]   = adjustR;
	_adjust[GREEN] = adjustG;
	_adjust[BLUE]  = adjustB;
	initializeMapping();
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

void RgbChannelAdjustment::apply(uint8_t input, uint8_t & red, uint8_t & green, uint8_t & blue)
{
	red   = _mapping[RED][input];
	green = _mapping[GREEN][input];
	blue  = _mapping[BLUE][input];
}

void RgbChannelAdjustment::initializeMapping()
{
	Debug(_log, "initialize mapping with %d,%d,%d", _adjust[RED], _adjust[GREEN], _adjust[BLUE]);
	// initialize linear mapping
	for (unsigned channel=0; channel<3; channel++)
		for (unsigned idx=0; idx<=UINT8_MAX; idx++)
		{
			_mapping[channel][idx] = std::min( ((idx * _adjust[channel]) / UINT8_MAX), (unsigned)UINT8_MAX);
		}
}
