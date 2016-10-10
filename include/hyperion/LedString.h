
#pragma once

// STL includes
#include <ctime>
#include <string>
#include <vector>

// Local includes
#include <utils/ColorRgb.h>

// QT includes
#include <QString>

// Forward class declarations
namespace Json { class Value; }

/// Enumeration containing the possible orders of device color byte data
enum ColorOrder
{
	ORDER_RGB, ORDER_RBG, ORDER_GRB, ORDER_BRG, ORDER_GBR, ORDER_BGR
};

inline QString colorOrderToString(const ColorOrder colorOrder)
{
	switch (colorOrder)
	{
	case ORDER_RGB:
		return "rgb";
	case ORDER_RBG:
		return "rbg";
	case ORDER_GRB:
		return "grb";
	case ORDER_BRG:
		return "brg";
	case ORDER_GBR:
		return "gbr";
	case ORDER_BGR:
		return "bgr";
	default:
		return "not-a-colororder";
	}
}
inline ColorOrder stringToColorOrder(const QString & order)
{
	if (order == "rgb")
	{
		return ORDER_RGB;
	}
	else if (order == "bgr")
	{
		return ORDER_BGR;
	}
	else if (order == "rbg")
	{
		return ORDER_RBG;
	}
	else if (order == "brg")
	{
		return ORDER_BRG;
	}
	else if (order == "gbr")
	{
		return ORDER_GBR;
	}
	else if (order == "grb")
	{
		return ORDER_GRB;
	}

	std::cout << "Unknown color order defined (" << order.toStdString() << "). Using RGB." << std::endl;
	return ORDER_RGB;
}

///
/// The Led structure contains the definition of the image portion used to determine a single led's
/// color.
/// @verbatim
/// |--------------------image--|
/// | minX  maxX                |
/// |  |-----|minY              |
/// |  |     |                  |
/// |  |-----|maxY              |
/// |                           |
/// |                           |
/// |                           |
/// |---------------------------|
/// @endverbatim
///
struct Led
{
	///  The index of the led
	unsigned index;

	///  The minimum vertical scan line included for this leds color
	double minX_frac;
	///  The maximum vertical scan line included for this leds color
	double maxX_frac;
	///  The minimum horizontal scan line included for this leds color
	double minY_frac;
	///  The maximum horizontal scan line included for this leds color
	double maxY_frac;
	/// id to clone
	int clone;
	/// the color order
	ColorOrder colorOrder;
};

///
/// The LedString contains the image integration information of the leds
///
class LedString
{
public:
	///
	/// Constructs the LedString with no leds
	///
	LedString();

	///
	/// Destructor of this LedString
	///
	~LedString();

	///
	/// Returns the led specifications
	///
	/// @return The list with led specifications
	///
	std::vector<Led>& leds();

	///
	/// Returns the led specifications
	///
	/// @return The list with led specifications
	///
	const std::vector<Led>& leds() const;

private:
	/// The list with led specifications
	std::vector<Led> mLeds;
};
