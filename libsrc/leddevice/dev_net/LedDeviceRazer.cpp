#include "LedDeviceRazer.h"
#include <utils/QStringUtils.h>

#if _WIN32
#include <windows.h>
#endif
// Constants
namespace {
bool verbose = false;

// Configuration settings
const char CONFIG_ADDRESS[] = "host";
const char RAZOR_DEVICE_TYPE[] = "razorDevice";

// WLED JSON-API elements
const char API_DEFAULT_HOST[] = "localhost";
const int API_DEFAULT_PORT = 54235;

const char API_BASE_PATH[] = "/razer/chromasdk";
const char API_PATH_INFO[] = "info";
const char API_PATH_STATE[] = "state";

const char API_RESULT[] = "result";

// List of State Information
const char STATE_ON[] = "on";
const char STATE_VALUE_TRUE[] = "true";
const char STATE_VALUE_FALSE[] = "false";
} //End of constants

LedDeviceRazer::LedDeviceRazer(const QJsonObject& deviceConfig)
	: LedDevice()
	  , _restApi(nullptr)
	  , _apiPort(API_DEFAULT_PORT)
{
	_devConfig = deviceConfig;
	_isDeviceReady = false;

	_activeDeviceType = deviceConfig["type"].toString("UNSPECIFIED").toLower();
}

LedDevice* LedDeviceRazer::construct(const QJsonObject& deviceConfig)
{
	return new LedDeviceRazer(deviceConfig);
}

bool LedDeviceRazer::init(const QJsonObject& deviceConfig)
{
	bool isInitOK = false;
	setRewriteTime(1000);
	connect(_refreshTimer, &QTimer::timeout, this, &LedDeviceRazer::rewriteLEDs);

	// Initialise sub-class
	if (LedDevice::init(deviceConfig))
	{
		// Initialise LedDevice configuration and execution environment
		uint configuredLedCount = this->getLedCount();
		Debug(_log, "DeviceType   : %s", QSTRING_CSTR(this->getActiveDeviceType()));
		Debug(_log, "LedCount     : %u", configuredLedCount);
		Debug(_log, "ColorOrder   : %s", QSTRING_CSTR(this->getColorOrder()));
		Debug(_log, "LatchTime    : %d", this->getLatchTime());
		Debug(_log, "RefreshTime  : %d", _refreshTimerInterval_ms);

		//Razor Chroma SDK allows localhost connection only
		_hostname = API_DEFAULT_HOST;
		_apiPort = API_DEFAULT_PORT;

		Debug(_log, "Hostname     : %s", QSTRING_CSTR(_hostname));
		Debug(_log, "Port         : %d", _apiPort);

		_razerDeviceType = deviceConfig[RAZOR_DEVICE_TYPE].toString("keyboard");

		Debug(_log, "Razer device : %s", QSTRING_CSTR(_razerDeviceType));

		if (initRestAPI(_hostname, _apiPort))
		{
			isInitOK = true;
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
		DebugIf(verbose, _log, "Reply: [%s]", strJson.toUtf8().constData());

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
	QString errortext;
	_isDeviceReady = false;

	// Try to open the LedDevice

	QJsonObject obj;

	obj.insert("title", "Hyperion - Razer Chroma");
	obj.insert("description", "Hyperion to Razer Chroma interface");

	QJsonObject authorDetails;
	authorDetails.insert("name", "Hyperion Team");
	authorDetails.insert("contact", "https://github.com/hyperion-project/hyperion.ng");

	obj.insert("author", authorDetails);

	QJsonArray deviceList = { "keyboard","mouse","headset","mousepad","keypad","chromalink" };
	obj.insert("device_supported", deviceList);

	obj.insert("category", "application");

	QJsonDocument data = QJsonDocument(obj);

	_restApi->setPort(API_DEFAULT_PORT);
	_restApi->setBasePath(API_BASE_PATH);

	httpResponse response = _restApi->post(data.toJson(QJsonDocument::Compact));
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
			data = QJsonDocument(effectObj);

			_restApi->setPath(_razerDeviceType);
			response = _restApi->put(data.toJson(QJsonDocument::Compact));

			if (!checkApiError(response))
			{
				_restApi->setPath(_razerDeviceType);
				response = _restApi->put(data.toJson(QJsonDocument::Compact));

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

	ColorRgb color = ledValues[0];
	int colorParam = (color.red * 65536) + (color.green * 256) + color.blue;

	QJsonObject effectObj;
	effectObj.insert("effect", "CHROMA_STATIC");
	QJsonObject param;
	param.insert("color", colorParam);
	effectObj.insert("param", param);
	QJsonDocument data = QJsonDocument(effectObj);

	_restApi->setPath(_razerDeviceType);
	httpResponse response = _restApi->put(data.toJson(QJsonDocument::Compact));
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
