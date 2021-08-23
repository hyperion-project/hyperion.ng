#ifndef LEDEVICEATMOORB_H
#define LEDEVICEATMOORB_H

// Qt includes
#include <QUdpSocket>
#include <QHostAddress>
#include <QVector>

// LedDevice includes
#include <leddevice/LedDevice.h>

class QUdpSocket;

///
/// Implementation of the LedDevice interface for sending to
/// AtmoOrb devices via network
///
class LedDeviceAtmoOrb : public LedDevice
{
	Q_OBJECT
public:

	///
	/// @brief Constructs an AtmoOrb LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceAtmoOrb(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~LedDeviceAtmoOrb() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// @brief Discover AtmoOrb devices available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params) override;

	///
	/// @brief Send an update to the AtmoOrb device to identify it.
	///
	/// Following parameters are required
	/// @code
	/// {
	///     "orbId"  : "orb identifier in the range of (1-255)",
	/// }
	///@endcode
	///
	/// @param[in] params Parameters to address device
	///
	virtual void identify(const QJsonObject& params) override;

protected:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

private:

	///
	/// Set Orbcolor
	///
	/// @param orbId the orb id
	/// @param color which color to set
	/// @param commandType which type of command to send (off / smoothing / etc..)
	///
	void setColor(int orbId, const ColorRgb &color, int commandType);

	///
	/// Send Orb command
	///
	/// @param bytes the byte array containing command to send over multicast
	///
	void sendCommand(const QByteArray &bytes);

	/// QUdpSocket object used to send data over
	QUdpSocket * _udpSocket;

	/// QHostAddress object of multicast group IP address
	QHostAddress _groupAddress;

	/// String containing multicast group IP address
	QString _multicastGroup;

	/// Multicast port to send data to
	quint16 _multiCastGroupPort;

	// Multicast status
	bool _joinedMulticastgroup;

	/// use Orbs own (external) smoothing algorithm
	bool _useOrbSmoothing;

	// Maximum allowed color difference, will skip Orb (external) smoothing once reached
	int _skipSmoothingDiff;

	/// Array of the orb ids.
	QVector<int> _orbIds;

	// Last send color map
	QMap<int, int> lastColorRedMap;
	QMap<int, int> lastColorGreenMap;
	QMap<int, int> lastColorBlueMap;

	QMultiMap<int, QHostAddress> _services;
};

#endif // LEDEVICEATMOORB_H
