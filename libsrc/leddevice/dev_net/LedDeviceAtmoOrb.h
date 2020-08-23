#ifndef LEDEVICEATMOORB_H
#define LEDEVICEATMOORB_H

// Qt includes
#include <QUdpSocket>
#include <QHostAddress>
#include <QVector>

// LedDevice includes
#include <leddevice/LedDevice.h>

class QUdpSocket;

/**
 * Implementation for the AtmoOrb
 *
 * To use set the device to "atmoorb".
 *
 * @author RickDB (github)
 */
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

	/// Transition time between colors (not implemented)
	int _transitiontime;

	// Maximum allowed color difference, will skip Orb (external) smoothing once reached
	int _skipSmoothingDiff;

	/// Number of leds in Orb, used to determine buffer size
	int _numLeds;

	/// Array of the orb ids.
	QVector<int> _orbIds;

	// Last send color map
	QMap<int, int> lastColorRedMap;
	QMap<int, int> lastColorGreenMap;
	QMap<int, int> lastColorBlueMap;
};

#endif // LEDEVICEATMOORB_H
