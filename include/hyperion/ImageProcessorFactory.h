#pragma once

// STL includes
#include <memory>

// Jsoncpp includes
#include <json/json.h>

#include <hyperion/LedString.h>

// Forward class declaration
class ImageProcessor;

///
/// The ImageProcessor is a singleton factor for creating ImageProcessors that translate images to
/// led color values.
///
class ImageProcessorFactory
{
public:
	///
	/// Returns the 'singleton' instance (creates the singleton if it does not exist)
	///
	/// @return The singleton instance of the ImageProcessorFactory
	///
	static ImageProcessorFactory& getInstance();

public:
	///
	/// Initialises this factory with the given led-configuration
	///
	/// @param[in] ledString  The led configuration
	/// @param[in] enableBlackBorderDetector Flag indicating if the blacborder detector should be enabled
	/// @param[in] blackborderThreshold The threshold which the blackborder detector should use
	///
	void init(const LedString& ledString, bool enableBlackBorderDetector, double blackborderThreshold);

	///
	/// Creates a new ImageProcessor. The onwership of the processor is transferred to the caller.
	///
	/// @return The newly created ImageProcessor
	///
	ImageProcessor* newImageProcessor() const;

private:
	/// The Led-string specification
	LedString _ledString;

	/// Flag indicating if the black border detector should be used
	bool _enableBlackBorderDetector;

	/// Threshold for the blackborder detector [0 .. 255]
	uint8_t _blackborderThreshold;
};
