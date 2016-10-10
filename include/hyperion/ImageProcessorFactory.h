#pragma once

// STL includes
#include <memory>

// QT includes
#include <QJsonObject>

//	if (jsoncpp_converted_to_QtJSON)
//	{
//		remove("#include <json/json.h>");
//	}

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
	void init(const LedString& ledString, const QJsonObject &blackborderConfig);

	///
	/// Creates a new ImageProcessor. The onwership of the processor is transferred to the caller.
	///
	/// @return The newly created ImageProcessor
	///
	ImageProcessor* newImageProcessor() const;

private:
	/// The Led-string specification
	LedString _ledString;

	// Reference to the blackborder json configuration values
	QJsonObject _blackborderConfig;
};
