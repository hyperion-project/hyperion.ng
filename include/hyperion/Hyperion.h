
#pragma once

// QT includes
#include <QObject>
#include <QTimer>

// hyperion-utils includes
#include <utils/RgbImage.h>

// Hyperion includes
#include <hyperion/LedString.h>
#include <hyperion/LedDevice.h>
#include <hyperion/PriorityMuxer.h>

// Forward class declaration
namespace hyperion { class ColorTransform; }


class Hyperion : public QObject
{
	Q_OBJECT
public:
	static LedString createLedString(const Json::Value& ledsConfig);

	static Json::Value loadConfig(const std::string& configFile);

	Hyperion(const std::string& configFile);
	Hyperion(const Json::Value& jsonConfig);

	~Hyperion();

	unsigned getLedCount() const;

	void setValue(int priority, std::vector<RgbColor> &ledColors, const int timeout_ms);

private slots:
	void update();

private:
	void applyTransform(std::vector<RgbColor>& colors) const;

	LedString mLedString;

	PriorityMuxer mMuxer;

	hyperion::ColorTransform* mRedTransform;
	hyperion::ColorTransform* mGreenTransform;
	hyperion::ColorTransform* mBlueTransform;

	LedDevice* mDevice;

	QTimer _timer;
};
