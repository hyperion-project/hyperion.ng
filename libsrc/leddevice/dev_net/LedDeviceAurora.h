#pragma once
// STL includes
#include <string>
#include <cmath>
#include <fstream>

// Leddevice includes
#include <leddevice/LedDevice.h>
// Qt includes
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QTimer>
///
/// Implementation of the LedDevice that write the led-colors to an
/// ASCII-textfile('/home/pi/LedDevice.out')
///
class LedDeviceAurora : public LedDevice
{
public:
	///
	/// Constructs the test-device, which opens an output stream to the file
	///
	LedDeviceAurora(const QJsonObject &deviceConfig);

	///
	/// Destructor of this test-device
	///
	virtual ~LedDeviceAurora();

	/// Switch the leds off
	virtual int switchOff();
	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);
protected:
	///
	/// Writes the RGB-Color values to the leds.
	///
	/// @param[in] ledValues  The RGB-color per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	bool init(const QJsonObject &deviceConfig);

private:
	/// The outputstream
	//	std::ofstream _ofs;
	// QNetworkAccessManager object for sending requests.
	QNetworkAccessManager* manager;

    // the number of leds (needed when switching off)

    size_t panelCount;
    /// Array of the pannel ids.
	std::vector<unsigned int> panelIds;
	QByteArray get(QString host, QString token, QString route);
	QByteArray putJson(QString url, QString json);
	QByteArray changeMode(QString host, QString token, QString route);
	///
	/// @param route
	///
	/// @return the full URL of the request.
	///
	QString getUrl(QString host, QString token, QString route);
};
