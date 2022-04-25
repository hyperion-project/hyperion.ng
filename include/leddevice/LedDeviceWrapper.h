#ifndef LEDEVICEWRAPPER_H
#define LEDEVICEWRAPPER_H

// util
#include <utils/Logger.h>
#include <utils/ColorRgb.h>
#include <utils/Components.h>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	#include <QRecursiveMutex>
#else
	#include <QMutex>
#endif

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
	explicit LedDeviceWrapper(Hyperion* hyperion);
	~LedDeviceWrapper() override;
	///
	/// @brief Constructs a new LedDevice, moves to thread and starts
	/// @param config  With the given configuration
	///
	void createLedDevice(const QJsonObject& config);

	///
	/// @brief Get all available device schemas
	/// @return device schemas
	///
	static QJsonObject getLedDeviceSchemas();

	///
	/// @brief add all device constructors to the map
	///
	static int addToDeviceMap(QString name, LedDeviceCreateFuncType funcPtr);

	///
	/// @brief Return all available device constructors
	/// @return device constructors
	///
	static const LedDeviceRegistry& getDeviceMap();

	///
	/// @brief Get the current latch time of the ledDevice
	/// @ return latch time in ms
	///
	int getLatchTime() const;

	///
	/// @brief Get the current active ledDevice type
	///
	QString getActiveDeviceType() const;

	///
	/// @brief Return the last enable state
	///
	bool enabled() const;

	///
	/// @brief Get the current colorOrder from device
	///
	QString getColorOrder() const;

	///
	/// @brief Get the number of LEDs from device
	///
	unsigned int getLedCount() const;

public slots:
	///
	/// @brief Handle new component state request
	/// @param component  The comp from enum
	/// @param state      The new state
	///
	void handleComponentState(hyperion::Components component, bool state);

signals:
	///
	/// PIPER signal for Hyperion -> LedDevice
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	int updateLeds(const std::vector<ColorRgb>& ledValues);

	///
	/// @brief Switch the LEDs on.
	///
	void switchOn();

	///
	/// @brief Switch the LEDs off.
	///
	void switchOff();
	
	void stopLedDevice();

private slots:
	///
	/// @brief Is called whenever the led device switches between on/off. The led device can disable it's component state
	/// The signal comes from the LedDevice
	/// @param newState The new state of the device
	///
	void handleInternalEnableState(bool newState);


protected:
	/// contains all available led device constructors
	static LedDeviceRegistry _ledDeviceMap;
	
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	static QRecursiveMutex       _ledDeviceMapLock;
#else
	static QMutex                _ledDeviceMapLock;
#endif
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

#endif // LEDEVICEWRAPPER_H
