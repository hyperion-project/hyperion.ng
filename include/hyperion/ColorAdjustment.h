#ifndef COLORADJUSTMENT_H
#define COLORADJUSTMENT_H

// STL includes
#include <QString>

// Utils includes
#include <utils/RgbChannelAdjustment.h>
#include <utils/RgbTransform.h>
#include <utils/OkhsvTransform.h>

class ColorAdjustment
{
public:
	/// Unique identifier for this color transform
	QString _id;

	/// The BLACK (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbBlackAdjustment;
	/// The WHITE (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbWhiteAdjustment;
	/// The RED (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbRedAdjustment;
	/// The GREEN (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbGreenAdjustment;
	/// The BLUE (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbBlueAdjustment;
	/// The CYAN (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbCyanAdjustment;
	/// The MAGENTA (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbMagentaAdjustment;
	/// The YELLOW (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbYellowAdjustment;

	RgbTransform _rgbTransform;
	OkhsvTransform _okhsvTransform;
};

#endif // COLORADJUSTMENT_H
