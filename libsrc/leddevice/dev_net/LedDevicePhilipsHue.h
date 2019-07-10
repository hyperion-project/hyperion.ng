#pragma once

// STL includes
#include <set>

// Qt includes
#include <QNetworkAccessManager>
#include <QTimer>

// Leddevice includes
#include <leddevice/LedDevice.h>

// Forward declaration
struct CiColorTriangle;

/**
 * A color point in the color space of the hue system.
 */
struct CiColor
{
	/// X component.
	float x;
	/// Y component.
	float y;
	/// The brightness.
	float bri;

	///
	/// Converts an RGB color to the Hue xy color space and brightness.
	/// https://github.com/PhilipsHue/PhilipsHueSDK-iOS-OSX/blob/master/ApplicationDesignNotes/RGB%20to%20xy%20Color%20conversion.md
	///
	/// @param red the red component in [0, 1]
	///
	/// @param green the green component in [0, 1]
	///
	/// @param blue the blue component in [0, 1]
	///
	/// @return color point
	///
	static CiColor rgbToCiColor(float red, float green, float blue, CiColorTriangle colorSpace);

	///
	/// @param p the color point to check
	///
	/// @return true if the color point is covered by the lamp color space
	///
	static bool isPointInLampsReach(CiColor p, CiColorTriangle colorSpace);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the cross product between p1 and p2
	///
	static float crossProduct(CiColor p1, CiColor p2);

	///
	/// @param a reference point one
	///
	/// @param b reference point two
	///
	/// @param p the point to which the closest point is to be found
	///
	/// @return the closest color point of p to a and b
	///
	static CiColor getClosestPointToPoint(CiColor a, CiColor b, CiColor p);

	///
	/// @param p1 point one
	///
	/// @param p2 point tow
	///
	/// @return the distance between the two points
	///
	static float getDistanceBetweenTwoPoints(CiColor p1, CiColor p2);
};

bool operator==(CiColor p1, CiColor p2);
bool operator!=(CiColor p1, CiColor p2);

/**
 * Color triangle to define an available color space for the hue lamps.
 */
struct CiColorTriangle
{
	CiColor red, green, blue;
};

class PhilipsHueBridge : public QObject
{
	Q_OBJECT

private:
	Logger* _log;
	/// QNetworkAccessManager  for sending requests.
	QNetworkAccessManager manager;
	/// Ip address of the bridge
	QString host;
	/// User name for the API ("newdeveloper")
	QString username;
	/// Timer for bridge reconnect interval
	QTimer bTimer;

private slots:
	///
	/// Receive all replies and check for error, schedule reconnect on issues
	/// Emits newLights() on success when triggered from connect()
	///
	void resolveReply(QNetworkReply* reply);

public slots:
	///
	/// Connect to bridge to check availbility and user
	///
	void bConnect(void);

signals:
	///
	///	Emits with a QMap of current bridge light/value pairs
	///
	void newLights(QMap<quint16,QJsonObject> map);

public:
	PhilipsHueBridge(Logger* log, QString host, QString username);

	///
	/// @param route the route of the POST request.
	///
	/// @param content the content of the POST request.
	///
	void post(QString route, QString content);
};

/**
 * Simple class to hold the id, the latest color, the color space and the original state.
 */
class PhilipsHueLight
{
private:
	Logger* _log;
	PhilipsHueBridge* bridge;
	/// light id
	unsigned int id;
	bool on;
	unsigned int transitionTime;
	CiColor color;
	/// The model id of the hue lamp which is used to determine the color space.
	QString modelId;
	CiColorTriangle colorSpace;
	/// The json string of the original state.
	QString originalState;

	///
	/// @param state the state as json object to set
	///
	void set(QString state);

public:
	// Hue system model ids (http://www.developers.meethue.com/documentation/supported-lights).
	// Light strips, color iris, ...
	static const std::set<QString> GAMUT_A_MODEL_IDS;
	// Hue bulbs, spots, ...
	static const std::set<QString> GAMUT_B_MODEL_IDS;
	// Hue Lightstrip plus, go ...
	static const std::set<QString> GAMUT_C_MODEL_IDS;

	///
	/// Constructs the light.
	///
	/// @param log the logger
	/// @param bridge the bridge
	/// @param id the light id
	///
	PhilipsHueLight(Logger* log, PhilipsHueBridge* bridge, unsigned int id, QJsonObject values);
	~PhilipsHueLight();

	///
	/// @param on
	///
	void setOn(bool on);

	///
	/// @param transitionTime the transition time between colors in multiples of 100 ms
	///
	void setTransitionTime(unsigned int transitionTime);

	///
	/// @param color the color to set
	/// @param brightnessFactor the factor to apply to the CiColor#bri value
	///
	void setColor(CiColor color, float brightnessFactor = 1.0f);
	CiColor getColor() const;

	///
	/// @return the color space of the light determined by the model id reported by the bridge.
	CiColorTriangle getColorSpace() const;

};

/**
 * Implementation for the Philips Hue system.
 *
 * To use set the device to "philipshue".
 * Uses the official Philips Hue API (http://developers.meethue.com).
 *
 * @author ntim (github), bimsarck (github)
 */
class LedDevicePhilipsHue: public LedDevice
{

	Q_OBJECT

public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDevicePhilipsHue(const QJsonObject &deviceConfig);

	///
	/// Destructor of this device
	///
	virtual ~LedDevicePhilipsHue();

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

public slots:
	/// thread start
	virtual void start();

private slots:
	/// creates new PhilipsHueLight(s) based on user lightid with bridge feedback
	///
	/// @param map Map of lightid/value pairs of bridge
	///
	void newLights(QMap<quint16, QJsonObject> map);

	void stateChanged(bool newState);

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
	/// bridge class
	PhilipsHueBridge* _bridge;

	///
	bool switchOffOnBlack;
	/// The brightness factor to multiply on color change.
	float brightnessFactor;
	/// Transition time in multiples of 100 ms.
	/// The default of the Hue lights is 400 ms, but we may want it snapier.
	int transitionTime;
	/// Array of the light ids.
	std::vector<unsigned int> lightIds;
	/// Array to save the lamps.
	std::vector<PhilipsHueLight> lights;
};
