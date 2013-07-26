
#pragma once

// hyperion-utils includes
#include <utils/RgbImage.h>

#include <hyperion/LedString.h>
#include <hyperion/ImageToLedsMap.h>
#include <hyperion/LedDevice.h>

class Hyperion
{
public:
	Hyperion(const Json::Value& jsonConfig);

	~Hyperion();

	void setInputSize(const unsigned width, const unsigned height);

	RgbImage& image()
	{
		return *mImage;
	}

	void commit();

	void operator() (const RgbImage& inputImage);

	void setColor(const RgbColor& color);

private:
	void applyTransform(std::vector<RgbColor>& colors) const;

	LedString mLedString;

	RgbImage* mImage;

	ImageToLedsMap mLedsMap;

	LedDevice* mDevice;
};
