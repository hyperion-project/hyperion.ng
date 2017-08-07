#pragma once

// Qt includes
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QHostAddress>
#include <QMap>
#include <QVector>

// Leddevice includes
#include <leddevice/LedDevice.h>

class QUdpSocket;

class AtmoOrbLight {
public:
	unsigned int id;

	///
	/// Constructs the light.
	///
	/// @param id the orb id
	AtmoOrbLight(unsigned int id);
};

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
	// Last send color map
	QMap<int, int> lastColorRedMap;
	QMap<int, int> lastColorGreenMap;
	QMap<int, int> lastColorBlueMap;

	// Multicast status
	bool joinedMulticastgroup;

	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceAtmoOrb(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);
	///
	/// Destructor of this device
	///
	virtual ~LedDeviceAtmoOrb();

	virtual int switchOff();

private:
	///
	/// Sends the given led-color values to the Orbs
	///
	/// @param ledValues The color-value per led
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector <ColorRgb> &ledValues);

	/// QNetworkAccessManager object for sending requests.
	QNetworkAccessManager *_manager;

	/// String containing multicast group IP address
	QString _multicastGroup;

	/// use Orbs own (external) smoothing algorithm
	bool _useOrbSmoothing;

	/// Transition time between colors (not implemented)
	int _transitiontime;

	// Maximum allowed color difference, will skip Orb (external) smoothing once reached
	int _skipSmoothingDiff;

	/// Multicast port to send data to
	int _multiCastGroupPort;

	/// Number of leds in Orb, used to determine buffer size
	int _numLeds;

	/// QHostAddress object of multicast group IP address
	QHostAddress _groupAddress;

	/// QUdpSocket object used to send data over
	QUdpSocket * _udpSocket;

	/// Array of the orb ids.
	QVector<unsigned int> _orbIds;

	///
	/// Set Orbcolor
	///
	/// @param orbId the orb id
	/// @param color which color to set
	/// @param commandType which type of command to send (off / smoothing / etc..)
	///
	void setColor(unsigned int orbId, const ColorRgb &color, int commandType);

	///
	/// Send Orb command
	///
	/// @param bytes the byte array containing command to send over multicast
	///
	void sendCommand(const QByteArray &bytes);
};
