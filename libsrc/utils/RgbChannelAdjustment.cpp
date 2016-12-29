// STL includes
#include <cmath>
#include <cstdint>
#include <algorithm>

// Utils includes
#include <utils/RgbChannelAdjustment.h>

RgbChannelAdjustment::RgbChannelAdjustment()
{
	setAdjustment(UINT8_MAX, UINT8_MAX, UINT8_MAX);
}

RgbChannelAdjustment::RgbChannelAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB)
{
	setAdjustment(adjustR, adjustG, adjustB);
}

RgbChannelAdjustment::~RgbChannelAdjustment()
{
}

void RgbChannelAdjustment::setAdjustment(uint8_t adjustR, uint8_t adjustG, uint8_t adjustB)
{
	_adjust[RED] = adjustR;
	_adjust[GREEN] = adjustG;
	_adjust[BLUE] = adjustB;
	initializeMapping();
}

uint8_t RgbChannelAdjustment::getAdjustmentR() const
{
	return _adjust[RED];
}

void RgbChannelAdjustment::setAdjustmentR(uint8_t adjustR)
{
	setAdjustment(adjustR, _adjust[GREEN], _adjust[BLUE]);
}

uint8_t RgbChannelAdjustment::getAdjustmentG() const
{
	return _adjust[GREEN];
}

void RgbChannelAdjustment::setAdjustmentG(uint8_t adjustG)
{
	setAdjustment(_adjust[RED], adjustG, _adjust[BLUE]);
}

uint8_t RgbChannelAdjustment::getAdjustmentB() const
{
	return _adjust[BLUE];
}

void RgbChannelAdjustment::setAdjustmentB(uint8_t adjustB)
{
	setAdjustment(_adjust[RED], _adjust[GREEN], adjustB);
}

uint8_t RgbChannelAdjustment::getAdjustmentR(uint8_t inputR) const
{
	return _mapping[RED][inputR];
}

uint8_t RgbChannelAdjustment::getAdjustmentG(uint8_t inputG) const
{
	return _mapping[GREEN][inputG];
}

uint8_t RgbChannelAdjustment::getAdjustmentB(uint8_t inputB) const
{
	return _mapping[BLUE][inputB];
}

void RgbChannelAdjustment::initializeMapping()
{
	// initialize linear mapping
	for (unsigned channel=0; channel<3; channel++)
		for (unsigned idx=0; idx<=UINT8_MAX; idx++)
		{
			_mapping[channel][idx] = std::min( ((idx * _adjust[channel]) / UINT8_MAX), (unsigned)UINT8_MAX);
		}
}
