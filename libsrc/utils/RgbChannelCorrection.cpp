// STL includes
#include <cmath>

// Utils includes
#include <utils/RgbChannelCorrection.h>

RgbChannelCorrection::RgbChannelCorrection() :
	_correctionR(255),
	_correctionG(255),
	_correctionB(255)
{
	initializeMapping();
}

RgbChannelCorrection::RgbChannelCorrection(int correctionR, int correctionG, int correctionB) :
	_correctionR(correctionR),
	_correctionG(correctionG),
	_correctionB(correctionB)
{
	initializeMapping();
}

RgbChannelCorrection::~RgbChannelCorrection()
{
}

uint8_t RgbChannelCorrection::getcorrectionR() const
{
	return _correctionR;
}

void RgbChannelCorrection::setcorrectionR(uint8_t correctionR)
{
	_correctionR = correctionR;
	initializeMapping();
}

uint8_t RgbChannelCorrection::getcorrectionG() const
{
	return _correctionG;
}

void RgbChannelCorrection::setcorrectionG(uint8_t correctionG)
{
	_correctionG = correctionG;
	initializeMapping();
}

uint8_t RgbChannelCorrection::getcorrectionB() const
{
	return _correctionB;
}

void RgbChannelCorrection::setcorrectionB(uint8_t correctionB)
{
	_correctionB = correctionB;
	initializeMapping();
}

uint8_t RgbChannelCorrection::correctionR(uint8_t inputR) const
{
	return _mappingR[inputR];
}

uint8_t RgbChannelCorrection::correctionG(uint8_t inputG) const
{
	return _mappingG[inputG];
}

uint8_t RgbChannelCorrection::correctionB(uint8_t inputB) const
{
	return _mappingB[inputB];
}

void RgbChannelCorrection::initializeMapping()
{
	// initialize the mapping
	for (int i = 0; i < 256; ++i)
	{
		int outputR = (i * _correctionR) / 255;
		if (outputR < -255)
			{
				outputR = -255;
			}
		else if (outputR > 255)
			{
				outputR = 255;
			}
		_mappingR[i] = outputR;
	}
	for (int i = 0; i < 256; ++i)
	{
		int outputG = (i * _correctionG) / 255;
		if (outputG < -255)
			{
				outputG = -255;
			}
		else if (outputG > 255)
			{
				outputG = 255;
			}
		_mappingG[i] = outputG;
	}
	for (int i = 0; i < 256; ++i)
	{
		int outputB = (i * _correctionB) / 255;
		if (outputB < -255)
			{
				outputB = -255;
			}
		else if (outputB > 255)
			{
				outputB = 255;
			}
		_mappingB[i] = outputB;
	}
		

}

