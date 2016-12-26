
// STL includes
#include <cassert>

// Hyperion includes
#include <utils/Logger.h>
#include "MultiColorAdjustment.h"

MultiColorAdjustment::MultiColorAdjustment(const unsigned ledCnt)
	: _ledAdjustments(ledCnt, nullptr)
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
	for (unsigned iLed=0; iLed<_ledAdjustments.size(); ++iLed)
	{
		if (_ledAdjustments[iLed] == nullptr)
		{
			Warning(Logger::getInstance("ColorAdjust"), "No adjustment set for %d", iLed);
			return false;
		}
	}
	return true;
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
		
		uint32_t nrng = (uint32_t) (255-color.red)*(255-color.green);
		uint32_t rng  = (uint32_t) (color.red)    *(255-color.green);
		uint32_t nrg  = (uint32_t) (255-color.red)*(color.green);
		uint32_t rg   = (uint32_t) (color.red)    *(color.green);
		
		uint8_t black   = nrng*(255-color.blue)/65025;
		uint8_t red     = rng *(255-color.blue)/65025;
		uint8_t green   = nrg *(255-color.blue)/65025;
		uint8_t blue    = nrng*(color.blue)    /65025;
		uint8_t cyan    = nrg *(color.blue)    /65025;
		uint8_t magenta = rng *(color.blue)    /65025;
		uint8_t yellow  = rg  *(255-color.blue)/65025;
		uint8_t white   = rg  *(color.blue)    /65025;
		
		uint8_t OR = adjustment->_rgbBlackAdjustment.getAdjustmentR(black);
		uint8_t OG = adjustment->_rgbBlackAdjustment.getAdjustmentG(black);
		uint8_t OB = adjustment->_rgbBlackAdjustment.getAdjustmentB(black);
		
		uint8_t RR = adjustment->_rgbRedAdjustment.getAdjustmentR(red);
		uint8_t	RG = adjustment->_rgbRedAdjustment.getAdjustmentG(red);
		uint8_t	RB = adjustment->_rgbRedAdjustment.getAdjustmentB(red);
		
		uint8_t GR = adjustment->_rgbGreenAdjustment.getAdjustmentR(green);
		uint8_t	GG = adjustment->_rgbGreenAdjustment.getAdjustmentG(green);
		uint8_t	GB = adjustment->_rgbGreenAdjustment.getAdjustmentB(green);
		
		uint8_t BR = adjustment->_rgbBlueAdjustment.getAdjustmentR(blue);
		uint8_t	BG = adjustment->_rgbBlueAdjustment.getAdjustmentG(blue);
		uint8_t	BB = adjustment->_rgbBlueAdjustment.getAdjustmentB(blue);
		
		uint8_t CR = adjustment->_rgbCyanAdjustment.getAdjustmentR(cyan);
		uint8_t CG = adjustment->_rgbCyanAdjustment.getAdjustmentG(cyan);
		uint8_t CB = adjustment->_rgbCyanAdjustment.getAdjustmentB(cyan);
		
		uint8_t MR = adjustment->_rgbMagentaAdjustment.getAdjustmentR(magenta);
		uint8_t MG = adjustment->_rgbMagentaAdjustment.getAdjustmentG(magenta);
		uint8_t MB = adjustment->_rgbMagentaAdjustment.getAdjustmentB(magenta);
		
		uint8_t YR = adjustment->_rgbYellowAdjustment.getAdjustmentR(yellow);
		uint8_t YG = adjustment->_rgbYellowAdjustment.getAdjustmentG(yellow);
		uint8_t YB = adjustment->_rgbYellowAdjustment.getAdjustmentB(yellow);
		
		uint8_t WR = adjustment->_rgbWhiteAdjustment.getAdjustmentR(white);
		uint8_t WG = adjustment->_rgbWhiteAdjustment.getAdjustmentG(white);
		uint8_t WB = adjustment->_rgbWhiteAdjustment.getAdjustmentB(white);
		
		color.red   = OR + RR + GR + BR + CR + MR + YR + WR;
		color.green = OG + RG + GG + BG + CG + MG + YG + WG;
		color.blue  = OB + RB + GB + BB + CB + MB + YB + WB;
	}
}
