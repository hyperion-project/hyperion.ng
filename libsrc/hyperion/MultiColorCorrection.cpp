
// STL includes
#include <cassert>

// Hyperion includes
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
	bool allLedsSet = true;
	for (unsigned iLed=0; iLed<_ledCorrections.size(); ++iLed)
	{
		if (_ledCorrections[iLed] == nullptr)
		{
			std::cerr << "HYPERION (C.correction) ERROR: No correction set for " << iLed << std::endl;
			allLedsSet = false;
		}
	}
	return allLedsSet;
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

std::vector<ColorRgb> MultiColorCorrection::applyCorrection(const std::vector<ColorRgb>& rawColors)
{
	// Create a copy, as we will do the rest of the correction in place
	std::vector<ColorRgb> ledColors(rawColors);

	const size_t itCnt = std::min(_ledCorrections.size(), rawColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorCorrection * correction = _ledCorrections[i];
		if (correction == nullptr)
		{
			// No correction set for this led (do nothing)
			continue;
		}
		ColorRgb& color = ledColors[i];

		color.red   = correction->_rgbCorrection.correctionR(color.red);
		color.green = correction->_rgbCorrection.correctionG(color.green);
		color.blue  = correction->_rgbCorrection.correctionB(color.blue);
	}
	return ledColors;
}
