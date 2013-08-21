#pragma once

// stl includes
#include <list>

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
namespace hyperion {
	class HsvTransform;
	class ColorTransform;
}

class Hyperion : public QObject
{
	Q_OBJECT
public:
	typedef PriorityMuxer::InputInfo InputInfo;

	enum Color
	{
		RED, GREEN, BLUE, INVALID
	};

	enum Transform
	{
		SATURATION_GAIN, VALUE_GAIN, THRESHOLD, GAMMA, BLACKLEVEL, WHITELEVEL
	};

	static LedString createLedString(const Json::Value& ledsConfig);

	Hyperion(const Json::Value& jsonConfig);

	~Hyperion();

	unsigned getLedCount() const;

	void setColor(int priority, RgbColor &ledColor, const int timeout_ms);

	void setColors(int priority, std::vector<RgbColor> &ledColors, const int timeout_ms);

	void setTransform(Transform transform, Color color, double value);

	void clear(int priority);

	void clearall();

	double getTransform(Transform transform, Color color) const;

	QList<int> getActivePriorities() const;

	const InputInfo& getPriorityInfo(const int priority) const;

private slots:
	void update();

private:
	void applyTransform(std::vector<RgbColor>& colors) const;

	LedString _ledString;

	PriorityMuxer _muxer;

	hyperion::HsvTransform * _hsvTransform;
	hyperion::ColorTransform * _redTransform;
	hyperion::ColorTransform * _greenTransform;
	hyperion::ColorTransform * _blueTransform;

	LedDevice* _device;

	QTimer _timer;
};
