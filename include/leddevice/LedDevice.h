#pragma once

// qt includes
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>

// STL includes
#include <vector>
#include <map>
#include <algorithm>

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

	///
	/// @brief Get color order of device
	/// @return The color order
	///
	const QString & getColorOrder() const { return _colorOrder; }

	///
	/// @brief Set the current active ledDevice type
	///
	/// @param deviceType Device's type
	///
    void setActiveDeviceType(const QString& deviceType);

	///
	/// @brief Get the current active ledDevice type
	///
	const QString & getActiveDeviceType() const { return _activeDeviceType; }

	void setLedCount(int ledCount);
	int  getLedCount() const { return _ledCount; }

	bool enabled() const { return _enabled; }
	int getLatchTime() const { return _latchTime_ms; }

	///
	/// Check, if device is ready to be used
	/// i.e. initialisation and configuration were successfull
	///
	/// @return True if device is ready
	///
	bool isReady() const { return _deviceReady; }

	///
	/// Check, if device is in error state
	///
	/// @return True if device is in error
	///
	bool isInError() const { return _deviceInError; }

	inline bool componentState() const { return enabled(); }

	/// Prints the RGB-Color values to stdout.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	static void printLedValues (const std::vector<ColorRgb>& ledValues );


public slots:
	///
	/// Is called on thread start, all construction tasks and init should run here
	///
	virtual void start() { _deviceReady = (open() == 0 ? true : false);}

	///
	/// Update the RGB-Color values to the leds.
	/// Handles refreshing of leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	/// @return Zero on success else negative (i.e. device is not ready)
	///
	virtual int updateLeds(const std::vector<ColorRgb>& ledValues);

	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	virtual void close();

	///
	/// Enables/disables the device for output.
	/// If the device is not ready, it will not be enabled
	///
	/// @param enable The new state of the device
	///
	void setEnable(bool enable);	///

signals:
	///
	/// Emits whenever the led device switches between on/off
	/// @param newState The new state of the device
	///
	void enableStateChanged(bool newState);

protected:

	///
	/// Initialise a device's configuration
	///
	/// @param deviceConfig the json device config
	/// @return True if success
	///
	virtual bool init(const QJsonObject &deviceConfig);

	///
	/// Opens and initiatialises the output device
	///
	/// @return Zero on succes (i.e. device is ready and enabled) else negative
	///
	virtual int open();

	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb>& ledValues) = 0;

	///
	/// Writes "BLACK" to the output stream
	///
	/// @return Zero on success else negative
	///
	virtual int writeBlack();

	// Helper to pipe device config from constructor to start()
	QJsonObject _devConfig;

	/// The common Logger instance for all LedDevices
	Logger * _log;

	/// The buffer containing the packed RGB values
	std::vector<uint8_t> _ledBuffer;

	bool _deviceReady;
	bool _deviceInError;

	QString _activeDeviceType;

	int _ledCount;
	int _ledRGBCount;
	int _ledRGBWCount;

	/// Timer object which makes sure that led data is written at a minimum rate
	/// e.g. Adalight device will switch off when it does not receive data at least every 15 seconds
	QTimer*	_refresh_timer;
	int		_refresh_timer_interval;

	/// timestamp of last write
	qint64	_last_write_time;

	/// Time a device requires mandatorily between two writes
	int		_latchTime_ms;


protected slots:

	/// Write the last data to the leds again
	///
	/// @return Zero on success else negative
	///
	int rewriteLeds();

	/// Switch the leds off
	/// Writes "Black to LED" or may switch-off the LED hardware, if supported
	///
	virtual int switchOff();

	/// Switch the leds on
	/// May switch-on the LED hardware, if supported
	///
	virtual int switchOn();

	/// Set device in error state
	///
	/// @param errorMsg The error message to be logged
	///
    virtual void setInError( const QString& errorMsg);

private:

	/// Start new refresh cycle
	///
	void startRefreshTimer();

	/// Stop refresh cycle
	///
	void stopRefreshTimer();


	bool	_componentRegistered;
	bool	_enabled;
	bool	_refresh_enabled;
	QString	_colorOrder;

	/// Last LED values written
	std::vector<ColorRgb> _last_ledValues;
};
