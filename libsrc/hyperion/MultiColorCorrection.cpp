
// STL includes
#include <cassert>

// Hyperion includes
#include <utils/Logger.h>
#include "MultiColorCorrection.h"

MultiColorCorrection::MultiColorCorrection(const unsigned ledCnt) :
	_ledCorrections(ledCnt, nullptr)
{
}

MultiColorCorrection::~MultiColorCorrection()
{
	// Clean up all the correctinos
	for (ColorCorrection * correction : _correction)
	{
		delete correction;
	}
}

void MultiColorCorrection::addCorrection(ColorCorrection * correction)
{
	_correctionIds.push_back(correction->_id);
	_correction.push_back(correction);
}

void MultiColorCorrection::setCorrectionForLed(const std::string& id, const unsigned startLed, const unsigned endLed)
{
	assert(startLed <= endLed);
	assert(endLed < _ledCorrections.size());

	// Get the identified correction (don't care if is nullptr)
	ColorCorrection * correction = getCorrection(id);
	for (unsigned iLed=startLed; iLed<=endLed; ++iLed)
	{
		_ledCorrections[iLed] = correction;
	}
}

bool MultiColorCorrection::verifyCorrections() const
{
	for (unsigned iLed=0; iLed<_ledCorrections.size(); ++iLed)
	{
		if (_ledCorrections[iLed] == nullptr)
		{
			Warning(Logger::getInstance("ColorCorrect"), "No adjustment set for %d", iLed);
			return false;
		}
	}
	return true;
}

const std::vector<std::string> & MultiColorCorrection::getCorrectionIds()
{
	return _correctionIds;
}

ColorCorrection* MultiColorCorrection::getCorrection(const std::string& id)
{
	// Iterate through the unique corrections until we find the one with the given id
	for (ColorCorrection * correction : _correction)
	{
		if (correction->_id == id)
		{
			return correction;
		}
	}

	// The ColorCorrection was not found
	return nullptr;
}

void MultiColorCorrection::applyCorrection(std::vector<ColorRgb>& ledColors)
{
	const size_t itCnt = std::min(_ledCorrections.size(), ledColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorCorrection * correction = _ledCorrections[i];
		if (correction == nullptr)
		{
			// No correction set for this led (do nothing)
			continue;
		}
		ColorRgb& color = ledColors[i];

		color.red   = correction->_rgbCorrection.adjustmentR(color.red);
		color.green = correction->_rgbCorrection.adjustmentG(color.green);
		color.blue  = correction->_rgbCorrection.adjustmentB(color.blue);
	}
}
