
#pragma once

// hyperion-utils includes
#include <utils/RgbImage.h>

// Hyperion includes
#include <hyperion/LedString.h>
#include <hyperion/LedDevice.h>
#include <hyperion/PriorityMuxer.h>

// Forward class declaration
namespace hyperion { class ColorTransform; }


class Hyperion
{
public:
	Hyperion(const Json::Value& jsonConfig);

	~Hyperion();

	unsigned getLedCount() const;

	void setValue(int priority, std::vector<RgbColor> &ledColors, const int timeout_ms);

private:
	void applyTransform(std::vector<RgbColor>& colors) const;

	LedString mLedString;

	PriorityMuxer mMuxer;

	hyperion::ColorTransform* mRedTransform;
	hyperion::ColorTransform* mGreenTransform;
	hyperion::ColorTransform* mBlueTransform;

	LedDevice* mDevice;
};
