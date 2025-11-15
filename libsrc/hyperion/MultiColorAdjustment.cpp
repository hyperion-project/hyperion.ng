#include <algorithm>

// Hyperion includes

#include <utils/Logger.h>
#include <hyperion/MultiColorAdjustment.h>

MultiColorAdjustment::MultiColorAdjustment(int ledCnt)
	: _ledAdjustments(static_cast<size_t>(ledCnt), nullptr)
	, _log(Logger::getInstance("ADJUSTMENT"))
{
	TRACK_SCOPE();
}

MultiColorAdjustment::~MultiColorAdjustment()
{
	TRACK_SCOPE();
	for (ColorAdjustment* adjustment : _adjustment)
	{
		delete adjustment;
	}
	_adjustment.clear();
}

void MultiColorAdjustment::addAdjustment(ColorAdjustment * adjustment)
{
	_adjustmentIds.push_back(adjustment->_id);
	_adjustment.push_back(adjustment);
}

void MultiColorAdjustment::setAdjustmentForLed(const QString& adjutmentId, int startLed, int endLed)
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
	ColorAdjustment * adjustment = getAdjustment(adjutmentId);
	for (size_t iLed=static_cast<size_t>(startLed); iLed<=static_cast<size_t>(endLed); ++iLed)
	{
		_ledAdjustments[iLed] = adjustment;
	}
}

bool MultiColorAdjustment::verifyAdjustments() const
{
	bool isAdjustmentDefined = true;
	for (unsigned iLed=0; iLed<_ledAdjustments.size(); ++iLed)
	{
		const ColorAdjustment * adjustment = _ledAdjustments[iLed];

		if (adjustment == nullptr)
		{
			Warning(_log, "No calibration set for LED %d", iLed);
			isAdjustmentDefined = false;
		}
	}
	return isAdjustmentDefined;
}

QStringList MultiColorAdjustment::getAdjustmentIds() const
{
	return _adjustmentIds;
}

ColorAdjustment* MultiColorAdjustment::getAdjustment(const QString& adjustmentId)
{
	auto adjustmentIter = std::find_if(_adjustment.begin(), _adjustment.end(), [&adjustmentId](const ColorAdjustment* adjustment) {
		return adjustment->_id == adjustmentId;
	});

	if (adjustmentIter != _adjustment.end()) {
		return *adjustmentIter;
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

void MultiColorAdjustment::applyAdjustment(QVector<ColorRgb>& ledColors)
{
	const size_t itCnt = qMin(_ledAdjustments.size(), ledColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorAdjustment* adjustment = _ledAdjustments[i];
		if (adjustment == nullptr)
		{
			// No transform set for this LED (do nothing)
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

		adjustment->_rgbTransform.applyGamma(ored,ogreen,oblue);
		adjustment->_rgbTransform.getBrightnessComponents(B_RGB, B_CMY, B_W);

		uint32_t nr_ng = static_cast<uint32_t>((UINT8_MAX - ored) * (UINT8_MAX - ogreen));
		uint32_t r_ng  = static_cast<uint32_t>(ored * (UINT8_MAX - ogreen));
		uint32_t nr_g  = static_cast<uint32_t>((UINT8_MAX - ored) * ogreen);
		uint32_t r_g   = static_cast<uint32_t>(ored * ogreen);

		uint8_t black   = static_cast<uint8_t>(nr_ng * (UINT8_MAX - oblue) / DOUBLE_UINT8_MAX_SQUARED);
		uint8_t red     = static_cast<uint8_t>(r_ng * (UINT8_MAX - oblue) / DOUBLE_UINT8_MAX_SQUARED);
		uint8_t green   = static_cast<uint8_t>(nr_g * (UINT8_MAX - oblue) / DOUBLE_UINT8_MAX_SQUARED);
		uint8_t blue    = static_cast<uint8_t>(nr_ng * (oblue) / DOUBLE_UINT8_MAX_SQUARED);
		uint8_t cyan    = static_cast<uint8_t>(nr_g * (oblue) / DOUBLE_UINT8_MAX_SQUARED);
		uint8_t magenta = static_cast<uint8_t>(r_ng * (oblue) / DOUBLE_UINT8_MAX_SQUARED);
		uint8_t yellow  = static_cast<uint8_t>(r_g * (UINT8_MAX - oblue) / DOUBLE_UINT8_MAX_SQUARED);
		uint8_t white   = static_cast<uint8_t>(r_g * (oblue) / DOUBLE_UINT8_MAX_SQUARED);

		uint8_t OR, OG, OB;  // Original Colors
		uint8_t RR, RG, RB;  // Red Adjustments
		uint8_t GR, GG, GB;  // Green Adjustments
		uint8_t BR, BG, BB;  // Blue Adjustments
		uint8_t CR, CG, CB;  // Cyan Adjustments
		uint8_t MR, MG, MB;  // Magenta Adjustments
		uint8_t YR, YG, YB;  // Yellow Adjustments
		uint8_t WR, WG, WB;  // White Adjustments

		adjustment->_rgbBlackAdjustment.apply  (black  , UINT8_MAX, OR, OG, OB);
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

		adjustment->_rgbTransform.applyTemperature(color);
		adjustment->_rgbTransform.applyBacklight(color.red, color.green, color.green);
	}
}
