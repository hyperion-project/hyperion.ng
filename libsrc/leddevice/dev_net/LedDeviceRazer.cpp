#include "LedDeviceRazer.h"
#include <utils/QStringUtils.h>

#if _WIN32
#include <windows.h>
#endif

#include <chrono>

using namespace Chroma;
using namespace Chroma::Keyboard;
using namespace Chroma::Keypad;
using namespace Chroma::Mouse;
using namespace Chroma::Mousepad;
using namespace Chroma::Headset;
using namespace Chroma::Chromalink;

// Constants
namespace {
	bool verbose = false;

	// Configuration settings
	const char CONFIG_RAZER_DEVICE_TYPE[] = "subType";
	const char CONFIG_SINGLE_COLOR[] = "singleColor";

	// WLED JSON-API elements
	const char API_DEFAULT_HOST[] = "localhost";
	const int API_DEFAULT_PORT = 54235;

	const char API_BASE_PATH[] = "/razer/chromasdk";
	const char API_RESULT[] = "result";

	constexpr std::chrono::milliseconds HEARTBEAT_INTERVALL{ 1000 };

} //End of constants

LedDeviceRazer::LedDeviceRazer(const QJsonObject& deviceConfig)
	: LedDevice(deviceConfig)
	, _restApi(nullptr)
	, _apiPort(API_DEFAULT_PORT)
	, _maxRow(Chroma::MAX_ROW)
	, _maxColumn(Chroma::MAX_COLUMN)
	, _maxLeds(Chroma::MAX_LEDS)
{
}

LedDevice* LedDeviceRazer::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceRazer(deviceConfig);
}

LedDeviceRazer::~LedDeviceRazer()
{
	delete _restApi;
	_restApi = nullptr;
}


bool LedDeviceRazer::init(const QJsonObject& deviceConfig)
{
	bool isInitOK = false;
	setRewriteTime(HEARTBEAT_INTERVALL.count());
	connect(_refreshTimer, &QTimer::timeout, this, &LedDeviceRazer::rewriteLEDs);

	// Initialise sub-class
	if (LedDevice::init(deviceConfig))
	{

		//Razer Chroma SDK allows localhost connection only
		_hostname = API_DEFAULT_HOST;
		_apiPort = API_DEFAULT_PORT;

		Debug(_log, "Hostname     : %s", QSTRING_CSTR(_hostname));
		Debug(_log, "Port         : %d", _apiPort);

		_razerDeviceType = deviceConfig[CONFIG_RAZER_DEVICE_TYPE].toString("invalid").toLower();
		_isSingleColor = deviceConfig[CONFIG_SINGLE_COLOR].toBool();

		Debug(_log, "Razer Device : %s", QSTRING_CSTR(_razerDeviceType));
		Debug(_log, "Single Color : %d", _isSingleColor);

		int configuredLedCount = this->getLedCount();
		if (resolveDeviceProperties(_razerDeviceType))
		{
			if (_isSingleColor && configuredLedCount > 1)
			{
				Info(_log, "In single color mode only the first LED of the configured [%d] will be used.", configuredLedCount);
			}
			else
			{
				if (_maxLeds != configuredLedCount)
				{
					Warning(_log, "Razer device might not work as expected. The type \"%s\" requires an LED-matrix [%d,%d] configured, i.e. %d LEDs. Currently only %d are defined via the layout.",
						QSTRING_CSTR(_razerDeviceType),
						_maxRow, _maxColumn, _maxLeds,
						configuredLedCount
					);
				}
			}

			if (initRestAPI(_hostname, _apiPort))
			{
				isInitOK = true;
			}
		}
		else
		{
			Error(_log, "Razer devicetype \"%s\" not supported", QSTRING_CSTR(_razerDeviceType));
		}
	}

	return isInitOK;
}

bool LedDeviceRazer::initRestAPI(const QString& hostname, int port)
{
	bool isInitOK = false;

	if (_restApi == nullptr)
	{
		_restApi = new ProviderRestApi(hostname, port);
		_restApi->setLogger(_log);

		_restApi->setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

		isInitOK = true;
	}

	return isInitOK;
}

bool LedDeviceRazer::checkApiError(const httpResponse& response)
{
	bool apiError = false;

	if (response.error())
	{
		this->setInError(response.getErrorReason());
		apiError = true;
	}
	else
	{
		QString errorReason;

		QString strJson(response.getBody().toJson(QJsonDocument::Compact));
		//DebugIf(verbose, _log, "Reply: [%s]", strJson.toUtf8().constData());

		QJsonObject jsonObj = response.getBody().object();

		if (!jsonObj[API_RESULT].isNull())
		{
			int resultCode = jsonObj[API_RESULT].toInt();

			if (resultCode != 0)
			{
				errorReason = QString("Chroma SDK error (%1)").arg(resultCode);
				this->setInError(errorReason);
				apiError = true;
			}
		}
	}
	return apiError;
}

int LedDeviceRazer::open()
{
	int retval = -1;
	_isDeviceReady = false;

	// Try to open the LedDevice

	QJsonObject obj;

	obj.insert("title", "Hyperion - Razer Chroma");
	obj.insert("description", "Hyperion to Razer Chroma interface");

	QJsonObject authorDetails;
	authorDetails.insert("name", "Hyperion Team");
	authorDetails.insert("contact", "https://github.com/hyperion-project/hyperion.ng");

	obj.insert("author", authorDetails);
	obj.insert("device_supported", QJsonArray::fromStringList(Chroma::SupportedDevices));

	obj.insert("category", "application");

	_restApi->setPort(API_DEFAULT_PORT);
	_restApi->setBasePath(API_BASE_PATH);

	httpResponse response = _restApi->post(obj);
	if (!checkApiError(response))
	{
		QJsonObject jsonObj = response.getBody().object();
		if (jsonObj["uri"].isNull())
		{
			this->setInError("Chroma SDK error. No 'uri' received");
		}
		else
		{
			_uri = jsonObj.value("uri").toString();
			_restApi->setUrl(_uri);

			DebugIf(verbose, _log, "Session-ID: %d, uri [%s]", jsonObj.value("sessionid").toInt(), QSTRING_CSTR(_uri.toString()));

			QJsonObject effectObj;
			effectObj.insert("effect", "CHROMA_STATIC");
			QJsonObject param;
			param.insert("color", 255);
			effectObj.insert("param", param);

			_restApi->setPath(_razerDeviceType);
			response = _restApi->put(effectObj);

			if (!checkApiError(response))
			{
				_restApi->setPath(_razerDeviceType);
				response = _restApi->put(effectObj);

				if (!checkApiError(response))
				{
					// Everything is OK, device is ready
					_isDeviceReady = true;
					retval = 0;
				}
			}
		}
	}
	return retval;
}

int LedDeviceRazer::close()
{
	int retval = -1;
	_isDeviceReady = false;

	if (!_uri.isEmpty())
	{
		httpResponse response = _restApi->deleteResource(_uri);
		if (!checkApiError(response))
		{
			// Everything is OK -> device is closed
			retval = 0;
		}
	}
	return retval;
}

int LedDeviceRazer::write(const std::vector<ColorRgb>& ledValues)
{
	int retval = -1;

	QJsonObject effectObj;

	if (_isSingleColor)
	{
		//Static effect
		effectObj.insert("effect", "CHROMA_STATIC");

		ColorRgb color = ledValues[0];
		int colorParam = (color.red * 65536) + (color.green * 256) + color.blue;

		QJsonObject param;
		param.insert("color", colorParam);
		effectObj.insert("param", param);
	}
	else
	{
		//Custom effect
		if (_customEffectType == Chroma::CHROMA_CUSTOM2)
		{
			effectObj.insert("effect", "CHROMA_CUSTOM2");
		}
		else {
			effectObj.insert("effect", "CHROMA_CUSTOM");
		}

		QJsonArray rowParams;
		for (int row = 0; row < _maxRow; row++) {
			QJsonArray columnParams;
			for (int col = 0; col < _maxColumn; col++) {
				int pos = row * _maxColumn + col;
				int bgrColor;
				if (pos < static_cast<int>(ledValues.size()))
				{
					bgrColor = (ledValues[pos].red * 65536) + (ledValues[pos].green * 256) + ledValues[pos].blue;
				}
				else
				{
					bgrColor = 0;
				}
				columnParams.append(bgrColor);
			}

			if (_maxRow == 1)
			{
				rowParams = columnParams;
			}
			else
			{
				rowParams.append(columnParams);
			}
		}
		effectObj.insert("param", rowParams);
	}

	_restApi->setPath(_razerDeviceType);
	httpResponse response = _restApi->put(effectObj);
	if (!checkApiError(response))
	{
		retval = 0;
	}
	return retval;
}

int LedDeviceRazer::rewriteLEDs()
{
	int retval = -1;

	_restApi->setPath("heartbeat");
	httpResponse response = _restApi->put();
	if (!checkApiError(response))
	{
		retval = 0;
	}
	return retval;
}

bool LedDeviceRazer::resolveDeviceProperties(const QString& deviceType)
{
	bool rc = true;

	int typeID = Chroma::SupportedDevices.indexOf(deviceType);

	switch (typeID)
	{
	case Chroma::DEVICE_KEYBOARD:
		_maxRow = Chroma::Keyboard::MAX_ROW;
		_maxColumn = Chroma::Keyboard::MAX_COLUMN;
		_customEffectType = Chroma::Keyboard::CUSTOM_EFFECT_TYPE;
		break;
	case Chroma::DEVICE_MOUSE:
		_maxRow = Chroma::Mouse::MAX_ROW;
		_maxColumn = Chroma::Mouse::MAX_COLUMN;
		_customEffectType = Chroma::Mouse::CUSTOM_EFFECT_TYPE;
		break;
	case Chroma::DEVICE_HEADSET:
		_maxRow = Chroma::Headset::MAX_ROW;
		_maxColumn = Chroma::Headset::MAX_COLUMN;
		_customEffectType = Chroma::Headset::CUSTOM_EFFECT_TYPE;
		break;
	case Chroma::DEVICE_MOUSEPAD:
		_maxRow = Chroma::Mousepad::MAX_ROW;
		_maxColumn = Chroma::Mousepad::MAX_COLUMN;
		_customEffectType = Chroma::Mousepad::CUSTOM_EFFECT_TYPE;
		break;
	case Chroma::DEVICE_KEYPAD:
		_maxRow = Chroma::Keypad::MAX_ROW;
		_maxColumn = Chroma::Keypad::MAX_COLUMN;
		_customEffectType = Chroma::Keypad::CUSTOM_EFFECT_TYPE;
		break;
	case Chroma::DEVICE_CHROMALINK:
		_maxRow = Chroma::Chromalink::MAX_ROW;
		_maxColumn = Chroma::Chromalink::MAX_COLUMN;
		_customEffectType = Chroma::Chromalink::CUSTOM_EFFECT_TYPE;
		break;
	default:
		rc = false;
		break;
	}

	if (rc)
	{
		_maxLeds = _maxRow * _maxColumn;
	}
	return rc;
}

QJsonObject LedDeviceRazer::getProperties(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());
	QJsonObject properties;

	_razerDeviceType = params[CONFIG_RAZER_DEVICE_TYPE].toString("invalid").toLower();
	if (resolveDeviceProperties(_razerDeviceType))
	{
		QJsonObject propertiesDetails;
		propertiesDetails.insert("maxRow", _maxRow);
		propertiesDetails.insert("maxColumn", _maxColumn);
		propertiesDetails.insert("maxLedCount", _maxLeds);

		properties.insert("properties", propertiesDetails);

		DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData());
	}
	return properties;
}
