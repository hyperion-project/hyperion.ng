
// STL includes
#include <cassert>

// Hyperion includes
#include <utils/Logger.h>
#include "MultiColorAdjustment.h"

MultiColorAdjustment::MultiColorAdjustment(const unsigned ledCnt)
	: _ledAdjustments(ledCnt, nullptr)
	, _log(Logger::getInstance("ColorAdjust"))
{
}

MultiColorAdjustment::~MultiColorAdjustment()
{
	// Clean up all the transforms
	for (ColorAdjustment * adjustment : _adjustment)
	{
		delete adjustment;
	}
}

void MultiColorAdjustment::addAdjustment(ColorAdjustment * adjustment)
{
	_adjustmentIds.push_back(adjustment->_id);
	_adjustment.push_back(adjustment);
}

void MultiColorAdjustment::setAdjustmentForLed(const QString& id, const unsigned startLed, const unsigned endLed)
{
	assert(startLed <= endLed);
	assert(endLed < _ledAdjustments.size());

	// Get the identified adjustment (don't care if is nullptr)
	ColorAdjustment * adjustment = getAdjustment(id);
	for (unsigned iLed=startLed; iLed<=endLed; ++iLed)
	{
		_ledAdjustments[iLed] = adjustment;
	}
}

bool MultiColorAdjustment::verifyAdjustments() const
{
	for (unsigned iLed=0; iLed<_ledAdjustments.size(); ++iLed)
	{
		ColorAdjustment * adjustment = _ledAdjustments[iLed];

		if (adjustment == nullptr)
		{
			Error(_log, "No adjustment set for %d", iLed);
			return false;
		}
		if (adjustment->_rgbTransform.getBrightness() <= adjustment->_rgbTransform.getBacklightThreshold() )
		{
			adjustment->_rgbTransform.setBacklightThreshold(0.0);
			adjustment->_rgbTransform.setBrightness(0.5);
			Warning(_log, "Adjustment for %d has invalid Brightness values, values set to default. (setBacklightThreshold is bigger then brightness)", iLed);
		}
	}
	return true;
}

const QStringList & MultiColorAdjustment::getAdjustmentIds()
{
	return _adjustmentIds;
}

ColorAdjustment* MultiColorAdjustment::getAdjustment(const QString& id)
{
	// Iterate through the unique adjustments until we find the one with the given id
	for (ColorAdjustment* adjustment : _adjustment)
	{
		if (adjustment->_id == id)
		{
			return adjustment;
		}
	}

	// The ColorAdjustment was not found
	return nullptr;
}

void MultiColorAdjustment::setBacklightEnabled(bool enable)
{
	for (ColorAdjustment* adjustment : _adjustment)
	{
		adjustment->_rgbTransform.setBackLightEnabled(enable);
	}
}


void MultiColorAdjustment::applyAdjustment(std::vector<ColorRgb>& ledColors)
{
	const size_t itCnt = std::min(_ledAdjustments.size(), ledColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorAdjustment* adjustment = _ledAdjustments[i];
		if (adjustment == nullptr)
		{
			// No transform set for this led (do nothing)
			continue;
		}
		ColorRgb& color = ledColors[i];

		uint8_t ored   = color.red;
		uint8_t ogreen = color.green;
		uint8_t oblue  = color.blue;
		
		adjustment->_rgbTransform.transform(ored,ogreen,oblue);

		uint32_t nrng = (uint32_t) (255-ored)*(255-ogreen);
		uint32_t rng  = (uint32_t) (ored)    *(255-ogreen);
		uint32_t nrg  = (uint32_t) (255-ored)*(ogreen);
		uint32_t rg   = (uint32_t) (ored)    *(ogreen);
		
		uint8_t black   = nrng*(255-oblue)/65025;
		uint8_t red     = rng *(255-oblue)/65025;
		uint8_t green   = nrg *(255-oblue)/65025;
		uint8_t blue    = nrng*(oblue)    /65025;
		uint8_t cyan    = nrg *(oblue)    /65025;
		uint8_t magenta = rng *(oblue)    /65025;
		uint8_t yellow  = rg  *(255-oblue)/65025;
		uint8_t white   = rg  *(oblue)    /65025;
		
		uint8_t OR, OG, OB, RR, RG, RB, GR, GG, GB, BR, BG, BB, CR, CG, CB, MR, MG, MB, YR, YG, YB, WR, WG, WB;
		adjustment->_rgbBlackAdjustment.apply(black, OR, OG, OB);
		adjustment->_rgbRedAdjustment.apply(red, RR, RG, RB);
		adjustment->_rgbGreenAdjustment.apply(green, GR, GG, GB);
		adjustment->_rgbBlueAdjustment.apply(blue, BR, BG, BB);
		adjustment->_rgbCyanAdjustment.apply(cyan, CR, CG, CB);
		adjustment->_rgbMagentaAdjustment.apply(magenta, MR, MG, MB);
		adjustment->_rgbYellowAdjustment.apply(yellow, YR, YG, YB);
		adjustment->_rgbWhiteAdjustment.apply(white, WR, WG, WB);

		color.red   = OR + RR + GR + BR + CR + MR + YR + WR;
		color.green = OG + RG + GG + BG + CG + MG + YG + WG;
		color.blue  = OB + RB + GB + BB + CB + MB + YB + WB;
	}
}
