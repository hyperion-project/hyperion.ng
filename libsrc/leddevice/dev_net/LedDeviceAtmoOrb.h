#pragma once

// Qt includes
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
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
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	explicit LedDeviceAtmoOrb(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig) override;

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);
	///
	/// Destructor of this device
	///
	virtual ~LedDeviceAtmoOrb() override;

protected:

	///
	/// Initialise device's network details
	///
	/// @return True if success
	bool initNetwork();

	///
	/// Opens and initiatialises the output device
	///
	/// @return Zero on succes (i.e. device is ready and enabled) else negative
	///
	virtual int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	virtual void close() override;

private:

	///
	/// Sends the given led-color values to the Orbs
	///
	/// @param ledValues The color-value per led
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector <ColorRgb> &ledValues) override;

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

	/// QNetworkAccessManager object for sending requests.
	QNetworkAccessManager *_networkmanager;

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
