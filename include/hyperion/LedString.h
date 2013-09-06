
#pragma once

// STL includes
#include <ctime>
#include <string>
#include <vector>

// Local includes
#include <utils/RgbColor.h>

// Forward class declarations
namespace Json { class Value; }

///
/// The Led structure contains the definition of the image portion used to determine a single led's
/// color.
/// <pre>
/// |--------------------image--|
/// | minX  maxX                |
/// |  |-----|minY              |
/// |  |     |                  |
/// |  |-----|maxY              |
/// |                           |
/// |                           |
/// |                           |
/// |---------------------------|
/// <endpre>
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
