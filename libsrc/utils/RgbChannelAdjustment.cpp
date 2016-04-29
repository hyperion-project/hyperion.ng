// STL includes
#include <cmath>

// Utils includes
#include <utils/RgbChannelAdjustment.h>

RgbChannelAdjustment::RgbChannelAdjustment() :
	_adjustR(255),
	_adjustG(255),
	_adjustB(255)
{
	initializeMapping();
}

RgbChannelAdjustment::RgbChannelAdjustment(int adjustR, int adjustG, int adjustB) :
	_adjustR(adjustR),
	_adjustG(adjustG),
	_adjustB(adjustB)
{
	initializeMapping();
}

RgbChannelAdjustment::~RgbChannelAdjustment()
{
}

uint8_t RgbChannelAdjustment::getadjustmentR() const
{
	return _adjustR;
}

void RgbChannelAdjustment::setadjustmentR(uint8_t adjustR)
{
	_adjustR = adjustR;
	initializeMapping();
}

uint8_t RgbChannelAdjustment::getadjustmentG() const
{
	return _adjustG;
}

void RgbChannelAdjustment::setadjustmentG(uint8_t adjustG)
{
	_adjustG = adjustG;
	initializeMapping();
}

uint8_t RgbChannelAdjustment::getadjustmentB() const
{
	return _adjustB;
}

void RgbChannelAdjustment::setadjustmentB(uint8_t adjustB)
{
	_adjustB = adjustB;
	initializeMapping();
}

uint8_t RgbChannelAdjustment::adjustmentR(uint8_t inputR) const
{
	return _mappingR[inputR];
}

uint8_t RgbChannelAdjustment::adjustmentG(uint8_t inputG) const
{
	return _mappingG[inputG];
}

uint8_t RgbChannelAdjustment::adjustmentB(uint8_t inputB) const
{
	return _mappingB[inputB];
}

void RgbChannelAdjustment::initializeMapping()
{
	// initialize the mapping
	for (int i = 0; i < 256; ++i)
	{
		int outputR = (i * _adjustR) / 255;
    		if (outputR > 255)
		{
			outputR = 255;
		}
		_mappingR[i] = outputR;
	}
	for (int i = 0; i < 256; ++i)
	{
		int outputG = (i * _adjustG) / 255;
		if (outputG > 255)
		{
			outputG = 255;
		}
		_mappingG[i] = outputG;
	}
	for (int i = 0; i < 256; ++i)
	{
		int outputB = (i * _adjustB) / 255;
		if (outputB > 255)
		{
			outputB = 255;
		}
		_mappingB[i] = outputB;
	}
		

}
