#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// STL incldues
#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include <QTimer>

// Utility includes
#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/RgbToRgbw.h>
#include <utils/Logger.h>
#include <functional>

class LedDevice;

typedef LedDevice* ( *LedDeviceCreateFuncType ) ( const QJsonObject& );
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

	/// Switch the leds off
	virtual int switchOff();
	
	virtual int setLedValues(const std::vector<ColorRgb>& ledValues);
	
	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	virtual int open();

	static int addToDeviceMap(std::string name, LedDeviceCreateFuncType funcPtr);
	static const LedDeviceRegistry& getDeviceMap();
	static void setActiveDevice(std::string dev);
	static std::string activeDevice() { return _activeDevice; }
	static QJsonObject getLedDeviceSchemas();
	static void setLedCount(int ledCount);
	static int  getLedCount() { return _ledCount; }
protected:
	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues) = 0;
	virtual bool init(const QJsonObject &deviceConfig);

	/// The common Logger instance for all LedDevices
	Logger * _log;

	/// The buffer containing the packed RGB values
	std::vector<uint8_t> _ledBuffer;

	bool _deviceReady;

	static std::string _activeDevice;
	static LedDeviceRegistry _ledDeviceMap;

	static int _ledCount;
	static int _ledRGBCount;
	static int _ledRGBWCount;

	/// Timer object which makes sure that led data is written at a minimum rate
	/// e.g. Adalight device will switch off when it does not receive data at least every 15 seconds
	QTimer        _refresh_timer;
	unsigned int _refresh_timer_interval;
	
protected slots:
	/// Write the last data to the leds again
	int rewriteLeds();

private:
	std::vector<ColorRgb> _ledValues;
};
