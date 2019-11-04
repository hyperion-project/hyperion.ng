#pragma once

// util
#include <utils/Logger.h>
#include <utils/ColorRgb.h>
#include <utils/Components.h>

class LedDevice;
class Hyperion;

typedef LedDevice* ( *LedDeviceCreateFuncType ) ( const QJsonObject& );
typedef std::map<QString,LedDeviceCreateFuncType> LedDeviceRegistry;

///
/// @brief Creates and destroys LedDevice instances with LedDeviceFactory and moves the device to a thread. Pipes all signal/slots and methods to LedDevice instance
///
class LedDeviceWrapper : public QObject
{
	Q_OBJECT
public:
	LedDeviceWrapper(Hyperion* hyperion);
	~LedDeviceWrapper();
	///
	/// @brief Contructs a new LedDevice, moves to thread and starts
	/// @param config  With the given config
	///
	void createLedDevice(const QJsonObject& config);

	///
	/// @brief Get all available device schemas
	/// @return device schemas
	///
	static const QJsonObject getLedDeviceSchemas();

	///
	/// @brief add all device constrcutors to the map
	///
	static int addToDeviceMap(QString name, LedDeviceCreateFuncType funcPtr);

	///
	/// @brief Return all available device contructors
	/// @return device constrcutors
	///
	static const LedDeviceRegistry& getDeviceMap();

	///
	/// @brief Get the current latchtime of the ledDevice
	/// @ return latchtime in ms
	///
	int getLatchTime();

	///
	/// @brief Get the current active ledDevice type
	///
	const QString & getActiveDeviceType();

	///
	/// @brief Return the last enable state
	///
	const bool & enabled() { return _enabled; };

	///
	/// @brief Get the current colorOrder from device
	///
	const QString & getColorOrder();

public slots:
	///
	/// @brief Handle new component state request
	/// @apram component  The comp from enum
	/// @param state      The new state
	///
	void handleComponentState(const hyperion::Components component, const bool state);

signals:
	///
	/// PIPER signal for Hyperion -> LedDevice
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	int write(const std::vector<ColorRgb>& ledValues);

private slots:
	///
	/// @brief Is called whenever the led device switches between on/off. The led device can disable it's component state
	/// The signal comes from the LedDevice
	/// @param newState The new state of the device
	///
	void handleInternalEnableState(bool newState);


protected:
	/// contains all available led device constrcutors
	static LedDeviceRegistry _ledDeviceMap;

private:
	///
	/// @brief switchOff() the device and Stops the device thread
	/// 
	void stopDeviceThread();

private:
	// parent Hyperion
	Hyperion* _hyperion;
	// Pointer of current led device
	LedDevice* _ledDevice;
	// the enable state
	bool _enabled;
};
