// Hyperion includes
#include <utils/Logger.h>
#include <hyperion/MultiColorAdjustment.h>

MultiColorAdjustment::MultiColorAdjustment(int ledCnt)
	: _ledAdjustments(ledCnt, nullptr)
	, _log(Logger::getInstance("ADJUSTMENT"))
{
}

MultiColorAdjustment::~MultiColorAdjustment()
{
	// Clean up all the transforms
	for (ColorAdjustment * adjustment : _adjustment)
	{
		delete adjustment;
		// BUG: Calling pop_back while iterating is invalid
		_adjustment.pop_back();
	}
}

void MultiColorAdjustment::addAdjustment(ColorAdjustment * adjustment)
{
	_adjustmentIds.push_back(adjustment->_id);
	_adjustment.push_back(adjustment);
}

void MultiColorAdjustment::setAdjustmentForLed(const QString& id, int startLed, int endLed)
{
	// abort
	if(startLed > endLed)
	{
		Error(_log,"startLed > endLed -> %d > %d", startLed, endLed);
		return;
	}
	// catch wrong values
	if(endLed > static_cast<int>(_ledAdjustments.size()-1))
	{
		Warning(_log,"The color calibration 'LED index' field has LEDs specified which aren't part of your led layout");
		endLed = static_cast<int>(_ledAdjustments.size()-1);
	}

	// Get the identified adjustment (don't care if is nullptr)
	ColorAdjustment * adjustment = getAdjustment(id);
	for (int iLed=startLed; iLed<=endLed; ++iLed)
	{
		_ledAdjustments[iLed] = adjustment;
	}
}

bool MultiColorAdjustment::verifyAdjustments() const
{
	bool ok = true;
	for (unsigned iLed=0; iLed<_ledAdjustments.size(); ++iLed)
	{
		ColorAdjustment * adjustment = _ledAdjustments[iLed];

		if (adjustment == nullptr)
		{
			Warning(_log, "No calibration set for led %d", iLed);
			ok = false;
		}
	}
	return ok;
}

QStringList MultiColorAdjustment::getAdjustmentIds() const
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
	const size_t itCnt = qMin(_ledAdjustments.size(), ledColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorAdjustment* adjustment = _ledAdjustments[i];
		if (adjustment == nullptr)
		{
			//std::cout << "MultiColorAdjustment::applyAdjustment() - No transform set for this led : " << i << std::endl;
			// No transform set for this led (do nothing)
			continue;
		}
		ColorRgb& color = ledColors[i];

		uint8_t ored   = color.red;
		uint8_t ogreen = color.green;
		uint8_t oblue  = color.blue;
		uint8_t B_RGB = 0;
		uint8_t B_CMY = 0;
		uint8_t B_W = 0;

		if (!adjustment->_okhsvTransform.isIdentity())
		{
			adjustment->_okhsvTransform.transform(ored, ogreen, oblue);
		}
		adjustment->_rgbTransform.transform(ored,ogreen,oblue);
		adjustment->_rgbTransform.getBrightnessComponents(B_RGB, B_CMY, B_W);

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

		uint8_t OR, OG, OB, RR, RG, RB, GR, GG, GB, BR, BG, BB;
		uint8_t CR, CG, CB, MR, MG, MB, YR, YG, YB, WR, WG, WB;

		adjustment->_rgbBlackAdjustment.apply  (black  , 255  , OR, OG, OB);
		adjustment->_rgbRedAdjustment.apply    (red    , B_RGB, RR, RG, RB);
		adjustment->_rgbGreenAdjustment.apply  (green  , B_RGB, GR, GG, GB);
		adjustment->_rgbBlueAdjustment.apply   (blue   , B_RGB, BR, BG, BB);
		adjustment->_rgbCyanAdjustment.apply   (cyan   , B_CMY, CR, CG, CB);
		adjustment->_rgbMagentaAdjustment.apply(magenta, B_CMY, MR, MG, MB);
		adjustment->_rgbYellowAdjustment.apply (yellow , B_CMY, YR, YG, YB);
		adjustment->_rgbWhiteAdjustment.apply  (white  , B_W  , WR, WG, WB);

		color.red   = OR + RR + GR + BR + CR + MR + YR + WR;
		color.green = OG + RG + GG + BG + CG + MG + YG + WG;
		color.blue  = OB + RB + GB + BB + CB + MB + YB + WB;
	}
}
