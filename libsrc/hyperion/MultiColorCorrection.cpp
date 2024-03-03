// Hyperion includes
#include <utils/Logger.h>
#include <hyperion/MultiColorCorrection.h>

MultiColorCorrection::MultiColorCorrection(int ledCnt) :
	_ledCorrections(ledCnt, nullptr)
, _log(Logger::getInstance("CORRECTION"))
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

void MultiColorCorrection::setCorrectionForLed(const QString& id, int startLed, int endLed)
{
	// abort
	if(startLed > endLed)
	{
		Error(_log,"startLed > endLed -> %d > %d", startLed, endLed);
		return;
	}
	// catch wrong values
	if(endLed > static_cast<int>(_ledCorrections.size()-1))
	{
		Warning(_log,"The color correction 'LED index' field has LEDs specified which aren't part of your led layout");
		endLed = static_cast<int>(_ledCorrections.size()-1);
	}

	// Get the identified correction (don't care if is nullptr)
	ColorCorrection * correction = getCorrection(id);
	for (int iLed=startLed; iLed<=endLed; ++iLed)
	{
		_ledCorrections[iLed] = correction;
	}
}

bool MultiColorCorrection::verifyCorrections() const
{
	bool ok = true;
	for (unsigned iLed=0; iLed<_ledCorrections.size(); ++iLed)
	{
		ColorCorrection* adjustment = _ledCorrections[iLed];

		if (adjustment == nullptr)
		{
			Warning(_log, "No correction set for led %d", iLed);
			ok = false;
		}
	}
	return ok;
}

QStringList & MultiColorCorrection::getCorrectionIds()
{
	return _correctionIds;
}

ColorCorrection* MultiColorCorrection::getCorrection(const QString& id)
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
	const size_t itCnt = qMin(_ledCorrections.size(), ledColors.size());
	for (size_t i=0; i<itCnt; ++i)
	{
		ColorCorrection * correction = _ledCorrections[i];
		if (correction == nullptr)
		{
			std::cout << "MultiColorCorrection::applyCorrection() - No correction set for this led : " << i << std::endl;
			// No correction set for this led (do nothing)
			continue;
		}
		ColorRgb& color = ledColors[i];

		color.red   = correction->_rgbCorrection.correctionR(color.red);
		color.green = correction->_rgbCorrection.correctionG(color.green);
		color.blue  = correction->_rgbCorrection.correctionB(color.blue);
	}
}
