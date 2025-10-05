#ifndef LEDEVICEWRAPPER_H
#define LEDEVICEWRAPPER_H

// util
#include <utils/Logger.h>
#include <utils/ColorRgb.h>
#include <utils/Components.h>

#include <QScopedPointer>
#include <QWeakPointer>

class LedDevice;
class Hyperion;

using LedDeviceCreateFuncType = LedDevice* (*)( const QJsonObject& );
using LedDeviceRegistry = QMap<QString,LedDeviceCreateFuncType>;

///
/// @brief Creates and destroys LedDevice instances with LedDeviceFactory and moves the device to an own thread. Pipes all signal/slots and methods to an Led-device instance
///
class LedDeviceWrapper : public QObject
{
	Q_OBJECT
public:
	explicit LedDeviceWrapper(const QSharedPointer<Hyperion>& hyperionInstance);
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

    void NewFunction(const QString &schemaFile, QString &data, QJsonObject &schema, QString &item);

    ///
	/// @brief add all device constructors to the map
	///
	static int addToDeviceMap(QString name, LedDeviceCreateFuncType funcPtr);

	///
	/// @brief Initialize the device map
	///
	static void initializeDeviceMap();

	///
	/// @brief Return all available device constructors
	/// @return device constructors
	///
	static const LedDeviceRegistry& getDeviceMap();

	///
	/// @brief Get the current latch time of the LED-device
	/// @ return latch time in ms
	///
	int getLatchTime() const;

	///
	/// @brief Get the current active LED-device type
	///
	QString getActiveDeviceType() const;

	///
	/// @brief Return the last enable state
	///
	bool isEnabled() const;

	///
	/// @brief Return the last ON state
	///
	bool isOn() const;

	///
	/// @brief Get the current colorOrder from device
	///
	QString getColorOrder() const;

	///
	/// @brief Get the number of LEDs from device
	///
	int getLedCount() const;

public slots:
	///
	/// @brief Handle new component state request
	/// @param component  The comp from enum
	/// @param state      The new state
	///
	void handleComponentState(hyperion::Components component, bool state);

	///
	/// @brief Disables the LED-device and stops the device's thread
	///
	void stopDevice();

signals:
	///
	/// PIPER signal for Hyperion -> LedDevice
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	int updateLeds(const QVector<ColorRgb>& ledValues);

	///
	/// @brief Switch the LEDs on.
	///
	void switchOn();

	///
	/// @brief Switch the LEDs off.
	///
	void switchOff();

	///
	/// @brief Enable the LED-device.
	///
	void enable();

	///
	/// @brief Disable the LED-device.
	///
	void disable();

	///
	/// @brief Stop the LED-device.
	///
	void stop();

	///
	/// @brief Emits when the LED-device stopped.
	///
	void  isStopped();

private slots:
	///
	/// @brief Is called whenever the LED-device is enabled/disabled.
	/// The signal comes from the LedDevice
	/// @param newState The new state of the device
	///
	void onIsEnabledChanged(bool isEnabled);

	///
	/// @brief Is called whenever the LED-device switches between on/off.
	/// The signal comes from the LedDevice
	/// @param newState The new state of the device
	///
	void onIsOnChanged(bool isOn);

protected:
	/// contains all available led device constructors
	static LedDeviceRegistry _ledDeviceMap;

private:
	/// The common Logger instance for all LED-devices
	Logger * _log;

	/// Hyperion instance pointer
	QWeakPointer<Hyperion> _hyperionWeak;

	// Pointer to the current LED-device & its thread
	QScopedPointer<LedDevice, QScopedPointerDeleteLater> _ledDevice;
	QScopedPointer<QThread> _ledDeviceThread;

	// 	LED-Device's states
	bool _isEnabled;
	bool _isOn;
};

#endif // LEDEVICEWRAPPER_H
