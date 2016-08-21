#pragma once

// STL incldues
#include <vector>
#include <QObject>
#include <map>

// Utility includes
#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/RgbToRgbw.h>
#include <utils/Logger.h>
#include <functional>
#include <json/json.h>

class LedDevice;

typedef LedDevice* ( *LedDeviceCreateFuncType ) ( const Json::Value& );
typedef std::map<std::string,LedDeviceCreateFuncType> LedDeviceRegistry;

///
/// Interface (pure virtual base class) for LedDevices.
///
class LedDevice : public QObject
{
	Q_OBJECT

public:
	LedDevice();
	///
	/// Empty virtual destructor for pure virtual base class
	///
	virtual ~LedDevice() {}

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues) = 0;

	/// Switch the leds off
	virtual int switchOff() = 0;
	
	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	virtual int open();

	static int addToDeviceMap(std::string name, LedDeviceCreateFuncType funcPtr);
	static const LedDeviceRegistry& getDeviceMap();
protected:
	/// The common Logger instance for all LedDevices
	Logger * _log;
	
	int _ledCount;

	/// The buffer containing the packed RGB values
	std::vector<uint8_t> _ledBuffer;
};
