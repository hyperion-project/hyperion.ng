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
	RgbChannelAdjustment _rgbBlackAdjustment {0, 0, 0, "black"};
	/// The WHITE (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbWhiteAdjustment{255, 255, 255, "white"};
	/// The RED (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbRedAdjustment {255, 0, 0 , "red"};
	/// The GREEN (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbGreenAdjustment {0, 255, 0, "green"};
	/// The BLUE (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbBlueAdjustment {0, 0, 255, "blue"};
	/// The CYAN (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbCyanAdjustment {0, 255, 255, "cyan"};
	/// The MAGENTA (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbMagentaAdjustment {255, 0, 255, "magenta"};
	/// The YELLOW (RGB-Channel) adjustment
	RgbChannelAdjustment _rgbYellowAdjustment {255, 255, 0, "yellow"};

	RgbTransform _rgbTransform;
	OkhsvTransform _okhsvTransform;
};

#endif // COLORADJUSTMENT_H
