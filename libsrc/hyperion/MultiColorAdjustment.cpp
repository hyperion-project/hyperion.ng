
// STL includes
#include <cassert>

// Hyperion includes
#include "MultiColorAdjustment.h"

MultiColorAdjustment::MultiColorAdjustment(const unsigned ledCnt) :
	_ledAdjustments(ledCnt, nullptr)
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

void MultiColorAdjustment::setAdjustmentForLed(const std::string& id, const unsigned startLed, const unsigned endLed)
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
	bool allLedsSet = true;
	for (unsigned iLed=0; iLed<_ledAdjustments.size(); ++iLed)
	{
		if (_ledAdjustments[iLed] == nullptr)
		{
			std::cerr << "HYPERION (C.adjustment) ERROR: No adjustment set for " << iLed << std::endl;
			allLedsSet = false;
		}
	}
	return allLedsSet;
}

const std::vector<std::string> & MultiColorAdjustment::getAdjustmentIds()
{
	return _adjustmentIds;
}

ColorAdjustment* MultiColorAdjustment::getAdjustment(const std::string& id)
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

std::vector<ColorRgb> MultiColorAdjustment::applyAdjustment(const std::vector<ColorRgb>& rawColors)
{
	// Create a copy, as we will do the rest of the adjustment in place
	std::vector<ColorRgb> ledColors(rawColors);

	const size_t itCnt = std::min(_ledAdjustments.size(), rawColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorAdjustment* adjustment = _ledAdjustments[i];
		if (adjustment == nullptr)
		{
			// No transform set for this led (do nothing)
			continue;
		}
		ColorRgb& color = ledColors[i];
		
		int RR = adjustment->_rgbRedAdjustment.adjustmentR(color.red);
		int RG = color.red > color.green ? adjustment->_rgbRedAdjustment.adjustmentG(color.red-color.green) : 0;
		int RB = color.red > color.blue ? adjustment->_rgbRedAdjustment.adjustmentB(color.red-color.blue) : 0;
		
		int GR = color.green > color.red ? adjustment->_rgbGreenAdjustment.adjustmentR(color.green-color.red) : 0;
		int GG = adjustment->_rgbGreenAdjustment.adjustmentG(color.green);
		int GB = color.green > color.blue ? adjustment->_rgbGreenAdjustment.adjustmentB(color.green-color.blue) : 0;
		
		int BR = color.blue > color.red ? adjustment->_rgbBlueAdjustment.adjustmentR(color.blue-color.red) : 0;
		int BG = color.blue > color.green ? adjustment->_rgbBlueAdjustment.adjustmentG(color.blue-color.green) : 0;
		int BB = adjustment->_rgbBlueAdjustment.adjustmentB(color.blue);
				
		int ledR = RR + GR + BR;
		int maxR = (int)adjustment->_rgbRedAdjustment.getadjustmentR();
		int ledG = RG + GG + BG;
		int maxG = (int)adjustment->_rgbGreenAdjustment.getadjustmentG();
		int ledB = RB + GB + BB;
		int maxB = (int)adjustment->_rgbBlueAdjustment.getadjustmentB();
		
		if (ledR > maxR)
		  color.red = (uint8_t)maxR;
		else
		  color.red = (uint8_t)ledR;
		
		if (ledG > maxG)
		  color.green = (uint8_t)maxG;
		else
		  color.green = (uint8_t)ledG;
		
		if (ledB > maxB)
		  color.blue = (uint8_t)maxB;
		else
		  color.blue = (uint8_t)ledB;
	}
	return ledColors;
}
