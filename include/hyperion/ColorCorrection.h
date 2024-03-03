#pragma once

// Qt includes
#include <QString>

// Utils includes
#include <utils/RgbChannelCorrection.h>

class ColorCorrection
{
public:
	
	/// Unique identifier for this color correction
	QString _id;

	/// The RGB correction
	RgbChannelCorrection _rgbCorrection;
};
