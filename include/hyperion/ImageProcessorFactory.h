#pragma once

// STL includes
#include <memory>

// QT includes
#include <QJsonObject>

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
	/// @param[in] blackborderConfig Contains the blackborder configuration
	///
	void init(const LedString& ledString, const QJsonObject &blackborderConfig, int mappingType);

	///
	/// Creates a new ImageProcessor. The onwership of the processor is transferred to the caller.
	///
	/// @return The newly created ImageProcessor
	///
	ImageProcessor* newImageProcessor() const;

private:
	/// The Led-string specification
	LedString _ledString;

	/// Reference to the blackborder json configuration values
	QJsonObject _blackborderConfig;

	// image 2 led mapping type
	int _mappingType;
};
