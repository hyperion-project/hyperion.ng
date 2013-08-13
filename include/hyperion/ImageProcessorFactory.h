#pragma once

// STL includes
#include <memory>

// Jsoncpp includes
#include <json/json.h>

#include <hyperion/LedString.h>

// Forward class declaration
class ImageProcessor;

class ImageProcessorFactory
{
public:
	static ImageProcessorFactory& getInstance();

public:
	void init(const LedString& ledString);

	ImageProcessor* newImageProcessor() const;

private:
	LedString _ledString;
};
