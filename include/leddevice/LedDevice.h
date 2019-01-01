#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// STL incldues
#include <vector>
#include <map>
#include <algorithm>

#include <QTimer>

// Utility includes
#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/RgbToRgbw.h>
#include <utils/Logger.h>
#include <functional>
#include <utils/Components.h>

class LedDevice;

typedef LedDevice* ( *LedDeviceCreateFuncType ) ( const QJsonObject& );
typedef std::map<QString,LedDeviceCreateFuncType> LedDeviceRegistry;

///
/// Interface (pure virtual base class) for LedDevices.
///
class LedDevice : public QObject
{
	Q_OBJECT

public:
	LedDevice(const QJsonObject& config = QJsonObject(), QObject* parent = nullptr);
	virtual ~LedDevice();

	/// Switch the leds off (led hardware disable)
	virtual int switchOff();

	/// Switch the leds on (led hardware enable), used if reinitialization is required for the device implementation
	virtual int switchOn();

	virtual int setLedValues(const std::vector<ColorRgb>& ledValues);

	///
	/// @brief Get color order of device
	/// @return The color order
	///
	const QString & getColorOrder() { return _colorOrder; };

	void setActiveDevice(QString dev);
	const QString & getActiveDevice() { return _activeDevice; };
	void setLedCount(int ledCount);
	int  getLedCount() { return _ledCount; }

	void setEnable(bool enable);
	bool enabled() { return _enabled; };
	const int getLatchTime() { return _latchTime_ms; };

	inline bool componentState() { return enabled(); };

public slots:
	///
	/// Is called on thread start, all construction tasks and init should run here
	///
	virtual void start() { _deviceReady = open(); };

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues) = 0;

signals:
	///
	/// Emits whenever the led device switches between on/off
	/// @param newState The new state of the device
	///
	void enableStateChanged(bool newState);

protected:
	virtual bool init(const QJsonObject &deviceConfig);

	///
	/// Opens and configures the output device
	///
	/// @return Zero on succes else negative
	///
	virtual int open();

	// Helper to pipe device config from constructor to start()
	QJsonObject _devConfig;

	/// The common Logger instance for all LedDevices
	Logger * _log;

	/// The buffer containing the packed RGB values
	std::vector<uint8_t> _ledBuffer;

	bool _deviceReady;

	QString _activeDevice;

	int _ledCount;
	int _ledRGBCount;
	int _ledRGBWCount;

	/// Timer object which makes sure that led data is written at a minimum rate
	/// e.g. Adalight device will switch off when it does not receive data at least every 15 seconds
	QTimer       _refresh_timer;
	unsigned int _refresh_timer_interval;
	qint64       _last_write_time;
	unsigned int _latchTime_ms;
protected slots:
	/// Write the last data to the leds again
	int rewriteLeds();

private:
	std::vector<ColorRgb> _ledValues;
	bool   _componentRegistered;
	bool   _enabled;
	QString _colorOrder;
};
