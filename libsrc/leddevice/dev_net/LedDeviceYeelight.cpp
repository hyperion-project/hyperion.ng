#include "LedDeviceYeelight.h"

#include <chrono>
#include <thread>

// Qt includes
#include <QEventLoop>

#include <QtNetwork>
#include <QTcpServer>
#include <QColor>
#include <QDateTime>

#include <ssdp/SSDPDiscover.h>
#include <utils/QStringUtils.h>

// mDNS discover
#ifdef ENABLE_MDNS
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#endif
#include <utils/NetUtils.h>

// Constants
namespace {

const bool verbose  = false;
const bool verbose3  = false;

constexpr std::chrono::milliseconds WRITE_TIMEOUT{1000};		 // device write timeout in ms
constexpr std::chrono::milliseconds READ_TIMEOUT{1000};			 // device read timeout in ms
constexpr std::chrono::milliseconds CONNECT_TIMEOUT{1000};		 // device connect timeout in ms
constexpr std::chrono::milliseconds CONNECT_STREAM_TIMEOUT{1000}; // device streaming connect timeout in ms

const bool TEST_CORRELATION_IDS  = false; //Ignore, if yeelight sends responses in different order as request commands

// Configuration settings
const char CONFIG_HOST[] = "host";
const char CONFIG_PORT[] = "port";

const char CONFIG_LIGHTS [] = "lights";

const char CONFIG_COLOR_MODEL [] = "colorModel";
const char CONFIG_TRANS_EFFECT [] = "transEffect";
const char CONFIG_TRANS_TIME [] = "transTime";
const char CONFIG_EXTRA_TIME_DARKNESS[] = "extraTimeDarkness";
const char CONFIG_DEBUGLEVEL [] = "debugLevel";

const char CONFIG_BRIGHTNESS_MIN[] = "brightnessMin";
const char CONFIG_BRIGHTNESS_SWITCHOFF[] = "brightnessSwitchOffOnMinimum";
const char CONFIG_BRIGHTNESS_MAX[] = "brightnessMax";
const char CONFIG_BRIGHTNESSFACTOR[] = "brightnessFactor";

const char CONFIG_RESTORE_STATE[] = "restoreOriginalState";

const char CONFIG_QUOTA_WAIT_TIME[] = "quotaWait";

// Yeelights API
const int API_DEFAULT_PORT = 55443;
const quint16 API_DEFAULT_QUOTA_WAIT_TIME = 1000;

// Yeelight API Command
const char API_COMMAND_ID[] = "id";
const char API_COMMAND_METHOD[] = "method";
const char API_COMMAND_PARAMS[] = "params";
const char API_COMMAND_PROPS[] = "props";

const char API_PARAM_CLASS_COLOR[] = "color";
const char API_PARAM_CLASS_HSV[] = "hsv";

const char API_PROP_NAME[] = "name";
const char API_PROP_MODEL[] = "model";
const char API_PROP_FWVER[] = "fw_ver";

const char API_PROP_POWER[] = "power";
const char API_PROP_MUSIC[] = "music_on";
const char API_PROP_RGB[] = "rgb";
const char API_PROP_CT[] = "ct";
const char API_PROP_COLORFLOW[] = "cf";
const char API_PROP_BRIGHT[] = "bright";

// List of Result Information
const char API_RESULT_ID[] = "id";
const char API_RESULT[] = "result";

// List of Error Information
const char API_ERROR[] = "error";
const char API_ERROR_CODE[] = "code";
const char API_ERROR_MESSAGE[] = "message";

// Yeelight ssdp services
const char SSDP_ID[] = "wifi_bulb";
const char SSDP_FILTER[] = "yeelight(.*)";
const char SSDP_FILTER_HEADER[] = "Location";
const quint16 SSDP_PORT = 1982;

} //End of constants

YeelightLight::YeelightLight( Logger *log, const QString &hostname, quint16 port = API_DEFAULT_PORT)
	:_log(log)
	  ,_debugLevel(0)
	  ,_isInError(false)
	  ,_host (hostname)
	  ,_port(port)
	  ,_tcpSocket(nullptr)
	  ,_tcpStreamSocket(nullptr)
	  ,_correlationID(0)
	  ,_lastWriteTime(QDateTime::currentMSecsSinceEpoch())
	  ,_lastColorRgbValue(0)
	  ,_transitionEffect(YeelightLight::API_EFFECT_SMOOTH)
	  ,_transitionDuration(API_PARAM_DURATION.count())
	  ,_extraTimeDarkness(API_PARAM_EXTRA_TIME_DARKNESS.count())
	  ,_brightnessMin(0)
	  ,_isBrightnessSwitchOffMinimum(false)
	  ,_brightnessMax(100)
	  ,_brightnessFactor(1.0)
	  ,_transitionEffectParam(API_PARAM_EFFECT_SMOOTH)
	  ,_waitTimeQuota(API_DEFAULT_QUOTA_WAIT_TIME)
	  ,_isOn(false)
	  ,_isInMusicMode(false)
{
	_name = hostname;
}

YeelightLight::~YeelightLight()
{
	log (3,"~YeelightLight()","" );
	delete _tcpSocket;
	log (2,"~YeelightLight()","void" );
}

void YeelightLight::setHostname( const QString &hostname, quint16 port = API_DEFAULT_PORT )
{
	log (3,"setHostname()","" );
	_host = hostname;
	_port =port;
}

void YeelightLight::setStreamSocket( QTcpSocket* socket )
{
	log (3,"setStreamSocket()","" );
	_tcpStreamSocket = socket;
}

bool YeelightLight::open()
{
	_isInError = false;
	bool rc = false;

	if ( _tcpSocket == nullptr )
	{
		_tcpSocket = new QTcpSocket();
	}

	if (  _tcpSocket->state() == QAbstractSocket::ConnectedState )
	{
		log (2,"open()","Device is already connected, skip opening: [%d]", _tcpSocket->state());
		rc = true;
	}
	else
	{
		QHostAddress address;
		if (NetUtils::resolveHostToAddress(_log, _host, address))
		{
			_tcpSocket->connectToHost( address.toString(), _port);

			if ( _tcpSocket->waitForConnected( CONNECT_TIMEOUT.count() ) )
			{
				if ( _tcpSocket->state() != QAbstractSocket::ConnectedState )
				{
					this->setInError( _tcpSocket->errorString() );
					rc = false;
				}
				else
				{
					log (2,"open()","Successfully opened Yeelight: %s", QSTRING_CSTR(_host));
					rc = true;
				}
			}
			else
			{
				this->setInError( _tcpSocket->errorString() );
				rc = false;
			}
		}
	}
	return rc;
}

bool YeelightLight::close()
{
	bool rc = true;

	if ( _tcpSocket != nullptr )
	{
		// Test, if device requires closing
		if ( _tcpSocket->isOpen() )
		{
			log (2,"close()","Close Yeelight: %s", QSTRING_CSTR(_host));
			_tcpSocket->close();
			// Everything is OK -> device is closed
		}
	}

	if ( _tcpStreamSocket != nullptr )
	{
		// Test, if stream socket requires closing
		if ( _tcpStreamSocket->isOpen() )
		{
			log (2,"close()","Close stream Yeelight: %s", QSTRING_CSTR(_host));
			_tcpStreamSocket->close();
		}
	}
	return rc;
}

int YeelightLight::writeCommand( const QJsonDocument &command, bool ignoreErrors )
{
	QJsonArray result;
	return writeCommand(command, result, ignoreErrors );
}

int YeelightLight::writeCommand( const QJsonDocument &command, QJsonArray &result, bool ignoreErrors )
{
	log( 3,
		 "writeCommand()",
		 "isON[%d], isInMusicMode[%d]",
		 static_cast<int>( _isOn ), static_cast<int>( _isInMusicMode ) );
	if (_debugLevel >= 2)
	{
		QString help = command.toJson(QJsonDocument::Compact);
		log (2,"writeCommand()","%s", QSTRING_CSTR(help));
	}

	int rc = -1;

	if ( ! _isInError && _tcpSocket->isOpen() )
	{
		qint64 bytesWritten = _tcpSocket->write( command.toJson(QJsonDocument::Compact) + "\r\n");
		if (bytesWritten == -1 )
		{
			this->setInError( QString ("Write Error: %1").arg(_tcpSocket->errorString()) );
		}
		else
		{
			if ( ! _tcpSocket->waitForBytesWritten(WRITE_TIMEOUT.count()) )
			{
				QString errorReason = QString ("(%1) %2").arg(_tcpSocket->error()).arg( _tcpSocket->errorString());
				log ( 2, "Error:", "bytesWritten: [%lld], %s", bytesWritten, QSTRING_CSTR(errorReason));
				this->setInError ( errorReason );
			}
			else
			{
				log ( 3, "Success:", "Bytes written   [%lld]", bytesWritten );

				// Avoid to overrun the Yeelight Command Quota
				qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - _lastWriteTime;

				if ( elapsedTime < _waitTimeQuota )
				{
					int waitTime = _waitTimeQuota;
					log ( 1, "writeCommand():", "Wait %dms, elapsedTime: %dms < quotaTime: %dms", waitTime, static_cast<int>(elapsedTime), _waitTimeQuota);

					// Wait time (in ms) before doing next write to not overrun Yeelight command quota
					std::this_thread::sleep_for(std::chrono::milliseconds(_waitTimeQuota));
				}
			}

			if ( _tcpSocket->waitForReadyRead(READ_TIMEOUT.count()) )
			{
				do
				{
					log ( 3, "Reading:", "Bytes available [%lld]", _tcpSocket->bytesAvailable() );
					while ( _tcpSocket->canReadLine() )
					{
						QByteArray response = _tcpSocket->readLine();

						YeelightResponse yeeResponse = handleResponse( _correlationID, response );
						switch ( yeeResponse.error() ) {

						case YeelightResponse::API_NOTIFICATION:
							rc=0;
							break;
						case YeelightResponse::API_OK:
							result = yeeResponse.getResult();
							rc=0;
							break;
						case YeelightResponse::API_ERROR:
							result = yeeResponse.getResult();
							QString errorReason = QString ("(%1) %2").arg(yeeResponse.getErrorCode()).arg( yeeResponse.getErrorReason() );
							if ( yeeResponse.getErrorCode() != -1)
							{
								if (!ignoreErrors)
								{
									this->setInError ( errorReason );
									rc =-1;
								}
								else
								{
									log ( 1, "writeCommand():", "Ignore Error: %s", QSTRING_CSTR(errorReason) );
									rc = 0;
								}
							}
							else
							{
								//(-1) client quota exceeded
								log ( 1, "writeCommand():", "%s", QSTRING_CSTR(errorReason) );
								rc = -2;
							}
							break;
						}
					}
					log ( 3, "Info:", "Trying to read more responses");
				}
				while ( _tcpSocket->waitForReadyRead(500) );
			}

			log ( 3, "Info:", "No more responses available");
		}

		//In case of no error or quota exceeded, update late write time avoiding immediate next write
		if ( rc == 0 || rc == -2 )
		{
			_lastWriteTime = QDateTime::currentMSecsSinceEpoch();
		}
	}
	else
	{
		log ( 2, "Info:", "Skip write. Device is in error");
	}

	log (3,"writeCommand() rc","%d", rc );
	return rc;
}

bool YeelightLight::streamCommand( const QJsonDocument &command )
{
	// ToDo: Wofür gibt es isON, wenn es beim StreamCommand nicht verwendet wird?
	//log (3,"streamCommand()","isON[%d], isInMusicMode[%d]", _isOn, _isInMusicMode );
	if (_debugLevel >= 2)
	{
		QString help = command.toJson(QJsonDocument::Compact);
		log (3,"streamCommand()","%s", QSTRING_CSTR(help));
	}

	bool rc = false;

	if ( ! _isInError && _tcpStreamSocket->isOpen() )
	{
		qint64 bytesWritten = _tcpStreamSocket->write( command.toJson(QJsonDocument::Compact) + "\r\n");
		if (bytesWritten == -1 )
		{
			this->setInError( QString ("Streaming Error %1").arg(_tcpStreamSocket->errorString()) );
		}
		else
		{
			if ( ! _tcpStreamSocket->waitForBytesWritten(WRITE_TIMEOUT.count()) )
			{
				int error = _tcpStreamSocket->error();
				QString errorReason = QString ("(%1) %2").arg(error).arg( _tcpStreamSocket->errorString());
				log ( 1, "Error:", "bytesWritten: [%lld], %s", bytesWritten, QSTRING_CSTR(errorReason));

				if ( error == QAbstractSocket::RemoteHostClosedError )
				{
					log (1,"streamCommand()","RemoteHostClosedError -  Give it a retry");
					_isInMusicMode = false;
					rc = true;
				}
				else
				{
						this->setInError ( errorReason );
				}
			}
			else
			{
				log ( 3, "Success:", "Bytes written   [%lld]", bytesWritten );
				rc = true;
			}
		}
	}
	else
	{
		log ( 2, "Info:", "Skip write. Device is in error");
	}
	return rc;
}

YeelightResponse YeelightLight::handleResponse(int correlationID, QByteArray const &response )
{
	log (3,"handleResponse()","" );

	YeelightResponse yeeResponse;
	QString errorReason;

	QJsonParseError error;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &error);

	if (error.error != QJsonParseError::NoError)
	{
		yeeResponse.setErrorCode (-10000);
		yeeResponse.setErrorReason( "Got invalid response" );
	}
	else
	{
		QString strJson(jsonDoc.toJson(QJsonDocument::Compact));

		QJsonObject jsonObj = jsonDoc.object();

		if ( !jsonObj[API_COMMAND_METHOD].isNull() )
		{
			yeeResponse.setError(YeelightResponse::API_NOTIFICATION);
			yeeResponse.setResult( QJsonArray() );
			// Do process notifications only for debugging
			if ( verbose3 )
			{
				log ( 3, "Info:", "Notification found : [%s]", QSTRING_CSTR( jsonObj[API_COMMAND_METHOD].toString()));

				QString method = jsonObj[API_COMMAND_METHOD].toString();

				if ( method == API_COMMAND_PROPS )
				{

					if ( jsonObj.contains(API_COMMAND_PARAMS) && jsonObj[API_COMMAND_PARAMS].isObject() )
					{
						QVariantMap paramsMap = jsonObj[API_COMMAND_PARAMS].toVariant().toMap();

						// Loop over all children.
						for (const QString & property : paramsMap.keys())
						{
							QString value = paramsMap[property].toString();
							log ( 3, "Notification ID:", "[%s]:[%s]", QSTRING_CSTR( property ), QSTRING_CSTR( value ));
						}
					}
				}
				else
				{
					log ( 1, "Error:", "Invalid notification message: [%s]", strJson.toUtf8().constData() );
				}
			}
		}
		else
		{
			int id = jsonObj[API_RESULT_ID].toInt();
			if ( id != correlationID && TEST_CORRELATION_IDS)
			{
				errorReason = QString ("%1| API is out of sync, received ID [%2], expected [%3]").
							  arg( _name ).arg( id ).arg( correlationID );

				yeeResponse.setErrorCode (-11000);
				yeeResponse.setErrorReason( errorReason );

				this->setInError ( errorReason );
			}
			else
			{

				if ( jsonObj.contains(API_RESULT) && jsonObj[API_RESULT].isArray() )
				{

					// API call returned an result
					yeeResponse.setResult( jsonObj[API_RESULT].toArray() );

					// Break down result only for debugging
					if ( verbose3 )
					{
						// Debug output
						if(!yeeResponse.getResult().empty())
						{
							for(const QJsonValueRef item : yeeResponse.getResult())
							{
								log ( 3, "Result:", "%s", QSTRING_CSTR( item.toString() ));
							}
						}
					}
				}
				else
				{
					yeeResponse.setError(YeelightResponse::API_ERROR);
					if ( jsonObj.contains(API_ERROR) && jsonObj[API_ERROR].isObject() )
					{
						QVariantMap errorMap = jsonObj[API_ERROR].toVariant().toMap();

						yeeResponse.setErrorCode (errorMap.value(API_ERROR_CODE).toInt());
						yeeResponse.setErrorReason( errorMap.value(API_ERROR_MESSAGE).toString() );
					}
					else
					{
						yeeResponse.setErrorCode (-10010);
						yeeResponse.setErrorReason( "No valid result message" );
						log ( 1, "Reply:", "[%s]", strJson.toUtf8().constData());
					}
				}
			}
		}
	}
	log (3,"handleResponse()", "yeeResponse.error [%d]", yeeResponse.error() );
	return yeeResponse;
}

void YeelightLight::setInError(const QString& errorMsg)
{
	_isInError = true;
	Error(_log, "Yeelight device '%s' signals error: '%s'", QSTRING_CSTR( _name ), QSTRING_CSTR(errorMsg));
}

QJsonDocument YeelightLight::getCommand(const QString &method, const QJsonArray &params)
{
	//Increment Correlation-ID
	++_correlationID;

	QJsonObject obj;
	obj.insert(API_COMMAND_ID,_correlationID);
	obj.insert(API_COMMAND_METHOD,method);
	obj.insert(API_COMMAND_PARAMS,params);

	return QJsonDocument(obj);
}

QJsonObject YeelightLight::getProperties()
{
	log (3,"getProperties()","" );
	QJsonObject properties;

	//All properties
	QJsonArray propertyList = {"power","bright","ct","rgb","hue","sat","color_mode","flowing","delayoff","music_on","name","bg_power","bg_flowing","bg_ct","bg_bright","bg_hue","bg_sat","bg_rgb","nl_br","active_mode" };

	QJsonDocument command = getCommand( API_METHOD_GETPROP, propertyList );

	QJsonArray result;

	if ( writeCommand( command, result ) > -1 )
	{

		// Debug output
		if( !result.empty())
		{
			int i = 0;
			for(const QJsonValueRef item : result)
			{
				log (1,"Property:", "%s = %s", QSTRING_CSTR( propertyList.at(i).toString() ), QSTRING_CSTR( item.toString() ));
				properties.insert( propertyList.at(i).toString(), item );
				++i;
			}
		}
	}

	log (2,"getProperties()","QJsonObject");
	return properties;
}

bool YeelightLight::identify()
{
	log (3,"identify()","" );
	bool rc = true;

	/*
	count		6, total number of visible state changing before color flow is stopped
	action		0, 0 means smart LED recover to the state before the color flow started

	Duration:	500, Gradual change timer sleep-time, in milliseconds
	Mode:		1, color
	Value:		100, RGB value when mode is 1 (blue)
	Brightness:	100, Brightness value

	Duration:	500
	Mode:		1
	Value:		16711696 (red)
	Brightness:	10
	*/
	QJsonArray colorflowParams = { API_PROP_COLORFLOW, 6, 0, "500,1,100,100,500,1,16711696,10"};

	QJsonDocument command = getCommand( API_METHOD_SETSCENE, colorflowParams );

	if ( writeCommand( command ) < 0 )
	{
		rc= false;
	}

	log( 2, "identify() rc","%d", static_cast<int>(rc) );
	return rc;
}

bool YeelightLight::isInMusicMode(bool deviceCheck)
{
	bool inMusicMode = false;

	if ( deviceCheck )
	{
		// Get status from device directly
		QJsonArray propertylist = { API_PROP_MUSIC };

		QJsonDocument command = getCommand( API_METHOD_GETPROP, propertylist );

		QJsonArray result;

		if ( writeCommand( command, result ) >= 0 )
		{
			if( !result.empty())
			{
				inMusicMode = result.at(0).toString() == "1";
			}
		}
	}
	else
	{
		// Test indirectly avoiding command quota
		if ( _tcpStreamSocket != nullptr)
		{
			if ( _tcpStreamSocket->state() == QAbstractSocket::ConnectedState )
			{
				log (3,"isInMusicMode", "Yes, as socket is in ConnectedState");
				inMusicMode = true;
			}
			else
			{
				log (1,"isInMusicMode", "No, StreamSocket state: %d", _tcpStreamSocket->state());
			}
		}
	}
	_isInMusicMode = inMusicMode;

	log( 3, "isInMusicMode()", "%d", static_cast<int>( _isInMusicMode ) );

	return _isInMusicMode;
}

void YeelightLight::mapProperties(const QJsonObject &properties)
{
	log (3,"mapProperties()","" );

	if ( _name.isEmpty() )
	{
		_name = properties.value(API_PROP_NAME).toString();
		if ( _name.isEmpty() )
		{
			_name = _host;
		}
	}
	_model	= properties.value(API_PROP_MODEL).toString();
	_fw_ver	= properties.value(API_PROP_FWVER).toString();

	_power	= properties.value(API_PROP_POWER).toString();
	_colorRgbValue = properties.value(API_PROP_RGB).toString().toInt();
	_bright	= properties.value(API_PROP_BRIGHT).toString().toInt();
	_ct		= properties.value(API_PROP_CT).toString().toInt();

	log (2,"mapProperties() rc","void" );
}

void YeelightLight::storeState()
{
	log (3,"storeState()","" );

	_originalStateProperties = this->getProperties();
	mapProperties( _originalStateProperties );

	log (2,"storeState() rc","void" );
}

bool YeelightLight::restoreState()
{
	log (3,"restoreState()","" );
	bool rc = false;

	QJsonArray paramlist = { API_PARAM_CLASS_COLOR, _colorRgbValue, _bright };

	if ( _isInMusicMode )
	{
		rc = streamCommand( getCommand( API_METHOD_SETSCENE, paramlist ) );
	}
	else
	{
		if ( writeCommand( getCommand( API_METHOD_SETSCENE, paramlist ) ) >= 0 )
		{
			rc =true;
		}
	}

	log( 2, "restoreState() rc","%d", static_cast<int>(rc) );
	return rc;
}

bool YeelightLight::setPower(bool on)
{
	return setPower( on, _transitionEffect, _transitionDuration);
}

bool YeelightLight::setPower(bool on, YeelightLight::API_EFFECT effect, int duration, API_MODE mode)
{
	bool rc = false;
	log( 3,
		 "setPower()",
		 "isON[%d], isInMusicMode[%d]",
		 static_cast<int>( _isOn), static_cast<int>(_isInMusicMode ) );

	// Disable music mode to get power-off command executed
	if ( !on && _isInMusicMode )
	{
		if ( _tcpStreamSocket != nullptr )
		{
			_tcpStreamSocket->close();
		}
	}
	else
	{
		if ( !_isInMusicMode && isInMusicMode(true) )
		{
			setMusicMode(false);
		}
	}

	QString powerParam = on ? API_METHOD_POWER_ON : API_METHOD_POWER_OFF;
	QString effectParam = effect == YeelightLight::API_EFFECT_SMOOTH ? API_PARAM_EFFECT_SMOOTH : API_PARAM_EFFECT_SUDDEN;

	QJsonArray paramlist = { powerParam, effectParam, duration, mode };

	// If power off was successful, automatically music-mode is off too
	if (  writeCommand( getCommand( API_METHOD_POWER, paramlist ) ) > -1 )
	{
		_isOn = on;
		if ( !on )
		{
			_isInMusicMode = false;
		}
		rc =true;
	}
	log( 2,
		 "setPower() rc",
		 "%d, isON[%d], isInMusicMode[%d]",
		 static_cast<int>(rc), static_cast<int>( _isOn ), static_cast<int>( _isInMusicMode ) );

	return rc;
}

bool YeelightLight::setColorRGB(const ColorRgb &color)
{
	bool rc = true;

	int colorParam = (color.red * 65536) + (color.green * 256) + color.blue;

	if ( colorParam == 0 )
	{
		colorParam = 1;
	}

	if ( colorParam != _lastColorRgbValue )
	{
		int bri = std::max( { color.red, color.green, color.blue } ) * 100 / 255;
		int duration = _transitionDuration;

		if ( bri < _brightnessMin )
		{
			if ( _isBrightnessSwitchOffMinimum )
			{
				log( 2,
					 "Set Color RGB:",
					 "Turn off, brightness [%d] < _brightnessMin [%d], "
					 "_isBrightnessSwitchOffMinimum [%d]",
					 bri,_brightnessMin, static_cast<int>(_isBrightnessSwitchOffMinimum ) );
				// Set brightness to 0
				bri = 0;
				duration = _transitionDuration + _extraTimeDarkness;
			}
			else
			{
				//If not switchOff on MinimumBrightness, avoid switch-off
				log( 2,
					 "Set Color RGB:",
					 "Set brightness[%d] to minimum brightness [%d], if not _isBrightnessSwitchOffMinimum [%d]",
					 bri, _brightnessMin, static_cast<int>( _isBrightnessSwitchOffMinimum ) );
				bri = _brightnessMin;
			}
		}
		else
		{
			bri = ( qMin( _brightnessMax, static_cast<int> (_brightnessFactor * qMax( _brightnessMin, bri ) ) ) );
		}

		log ( 3, "Set Color RGB:", "{%u,%u,%u} -> [%d], [%d], [%d], [%d]", color.red, color.green, color.blue, colorParam, bri, _transitionEffect, _transitionDuration );
		QJsonArray paramlist = { API_PARAM_CLASS_COLOR, colorParam, bri };

		// Only add transition effect and duration, if device smoothing is configured (older FW do not support this parameters in set_scene
		if ( _transitionEffect == YeelightLight::API_EFFECT_SMOOTH )
		{
			  paramlist << _transitionEffectParam << duration;
		}

		bool writeOK = false;
		if ( _isInMusicMode )
		{
			writeOK = streamCommand( getCommand( API_METHOD_SETSCENE, paramlist ) );
		}
		else
		{
			if ( writeCommand( getCommand( API_METHOD_SETSCENE, paramlist ) ) >= 0 )
			{
				writeOK = true;
			}
		}
		if ( writeOK )
		{
			_lastColorRgbValue = colorParam;
		}
		else
		{
			rc = false;
		}
	}
	return rc;
}

bool YeelightLight::setColorHSV(const ColorRgb &colorRGB)
{
	bool rc = true;

	QColor color(colorRGB.red, colorRGB.green, colorRGB.blue);

	if ( color != _color )
	{
		int hue;
		int sat;
		int bri;
		int duration = _transitionDuration;

		color.getHsv( &hue, &sat, &bri);

		//Align to Yeelight number ranges (hue: 0-359, sat: 0-100, bri: 0-100)
		if ( hue == -1)
		{
			hue = 0;
		}
		sat = sat * 100 / 255;
		bri = bri * 100 / 255;

		if ( bri < _brightnessMin )
		{
			if ( _isBrightnessSwitchOffMinimum )
			{
				log( 2,
					 "Set Color HSV:",
					 "Turn off, brightness [%d] < _brightnessMin [%d], "
					 "_isBrightnessSwitchOffMinimum [%d]",
					 bri,
					 _brightnessMin,
					 static_cast<int>( _isBrightnessSwitchOffMinimum ) );
				// Set brightness to 0
				bri = 0;
				duration = _transitionDuration + _extraTimeDarkness;
			}
			else
			{
				//If not switchOff on MinimumBrightness, avoid switch-off
				log( 2,
					 "Set Color HSV:",
					 "Set brightness[%d] to minimum brightness [%d], if not _isBrightnessSwitchOffMinimum [%d]",
					 bri, _brightnessMin, static_cast<int>( _isBrightnessSwitchOffMinimum ));
				bri = _brightnessMin;
			}
		}
		else
		{
			bri = ( qMin( _brightnessMax, static_cast<int> (_brightnessFactor * qMax( _brightnessMin, bri ) ) ) );
		}
		log ( 2, "Set Color HSV:", "{%u,%u,%u}, [%d], [%d]", hue, sat, bri, _transitionEffect, duration );
		QJsonArray paramlist = { API_PARAM_CLASS_HSV, hue, sat, bri };

		// Only add transition effect and duration, if device smoothing is configured (older FW do not support this parameters in set_scene
		if ( _transitionEffect == YeelightLight::API_EFFECT_SMOOTH )
		{
			paramlist << _transitionEffectParam << duration;
		}

		bool writeOK=false;
		if ( _isInMusicMode )
		{
			writeOK = streamCommand( getCommand( API_METHOD_SETSCENE, paramlist ) );
		}
		else
		{
			if ( writeCommand( getCommand( API_METHOD_SETSCENE, paramlist ) ) >= 0 )
			{
				writeOK = true;
			}
		}

		if ( writeOK )
		{
			_isOn = true;
			if ( bri == 0 )
			{
				_isOn = false;
				_isInMusicMode = false;
			}
			_color = color;
		}
		else
		{
			rc = false;
		}
	}
	else
	{
		// Skip update. Same Color as before
	}
	log( 3,
		 "setColorHSV() rc",
		 "%d, isON[%d], isInMusicMode[%d]",
		 static_cast<int>( rc ), static_cast<int>( _isOn ), static_cast<int>( _isInMusicMode ) );
	return rc;
}


void YeelightLight::setTransitionEffect(YeelightLight::API_EFFECT effect, int duration)
{
	if( effect != _transitionEffect )
	{
		_transitionEffect = effect;
		_transitionEffectParam = effect == YeelightLight::API_EFFECT_SMOOTH ? API_PARAM_EFFECT_SMOOTH : API_PARAM_EFFECT_SUDDEN;
	}

	if( duration != _transitionDuration )
	{
		_transitionDuration = duration;
	}

}

void YeelightLight::setBrightnessConfig(int min, int max, bool switchoff, int extraTime, double factor)
{
	_brightnessMin = min;
	_isBrightnessSwitchOffMinimum = switchoff;
	_brightnessMax = max;
	_brightnessFactor = factor;
	_extraTimeDarkness = extraTime;
}

bool YeelightLight::setMusicMode(bool on, const QHostAddress &hostAddress, int port)
{
	bool rc = false;
	int musicModeParam = on ? API_METHOD_MUSIC_MODE_ON : API_METHOD_MUSIC_MODE_OFF;

	QJsonArray paramlist = { musicModeParam };

	if ( on )
	{
		paramlist << hostAddress.toString() << port;

		// Music Mode is only on, if write did not fail nor quota was exceeded
		if ( writeCommand( getCommand( API_METHOD_MUSIC_MODE, paramlist ) ) > -1 )
		{
			_isInMusicMode = on;
			rc = true;
		}
	}
	else
	{
		QJsonArray offParams = { API_METHOD_MUSIC_MODE_OFF };
		if ( writeCommand( getCommand( API_METHOD_MUSIC_MODE, offParams ) ) > -1 )
		{
			_isInMusicMode = false;
			rc = true;
		}
	}

	log( 2,
		 "setMusicMode() rc", "%d, isInMusicMode[%d]", static_cast<int>( rc ), static_cast<int>( _isInMusicMode ) );
	return rc;
}

void YeelightLight::log(int logLevel, const char* msg, const char* type, ...)
{
	if ( logLevel <= _debugLevel)
	{
		const size_t max_val_length = 1024;
		char val[max_val_length];
		va_list args;
		va_start(args, type);
		vsnprintf(val, max_val_length, type, args);
		va_end(args);
		std::string s = msg;
		uint max = 20;
		if (max > s.length())
		{
			s.append(max - s.length(), ' ');
		}

		Debug( _log, "%d|%15.15s| %s: %s", logLevel, QSTRING_CSTR(_name), s.c_str(), val);
	}
}

//---------------------------------------------------------------------------------

LedDeviceYeelight::LedDeviceYeelight(const QJsonObject &deviceConfig)
	: LedDevice(deviceConfig)
	  ,_lightsCount (0)
	  ,_outputColorModel(0)
	  ,_transitionEffect(YeelightLight::API_EFFECT_SMOOTH)
	  ,_transitionDuration(API_PARAM_DURATION.count())
	  ,_extraTimeDarkness(0)
	  ,_brightnessMin(0)
	  ,_isBrightnessSwitchOffMinimum(false)
	  ,_brightnessMax(100)
	  ,_brightnessFactor(1.0)
	  ,_waitTimeQuota(API_DEFAULT_QUOTA_WAIT_TIME)
	  ,_debuglevel(0)
	  ,_musicModeServerPort(-1)
{
#ifdef ENABLE_MDNS
	QMetaObject::invokeMethod(&MdnsBrowser::getInstance(), "browseForServiceType",
							   Qt::QueuedConnection, Q_ARG(QByteArray, MdnsServiceRegister::getServiceType(_activeDeviceType)));
#endif
}

LedDeviceYeelight::~LedDeviceYeelight()
{
		delete _tcpMusicModeServer;
}

LedDevice* LedDeviceYeelight::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceYeelight(deviceConfig);
}

bool LedDeviceYeelight::init(const QJsonObject &deviceConfig)
{
	// Overwrite non supported/required features
	setRewriteTime(0);

	if (deviceConfig["rewriteTime"].toInt(0) > 0)
	{
		Info (_log, "Yeelights do not require rewrites. Refresh time is ignored.");
	}

	DebugIf(verbose, _log, "deviceConfig: [%s]", QString(QJsonDocument(_devConfig).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	bool isInitOK = false;

	if ( LedDevice::init(deviceConfig) )
	{
		//Get device specific configuration

		if ( deviceConfig[ CONFIG_COLOR_MODEL ].isString() )
		{
			_outputColorModel = deviceConfig[ CONFIG_COLOR_MODEL ].toString(QString(QChar(MODEL_RGB))).toInt();
		}
		else
		{
			_outputColorModel = deviceConfig[ CONFIG_COLOR_MODEL ].toInt(MODEL_RGB);
		}

		if ( deviceConfig[ CONFIG_TRANS_EFFECT ].isString() )
		{
			_transitionEffect = static_cast<YeelightLight::API_EFFECT>( deviceConfig[ CONFIG_TRANS_EFFECT ].toString(QString(QChar(YeelightLight::API_EFFECT_SMOOTH))).toInt() );
		}
		else
		{
			_transitionEffect = static_cast<YeelightLight::API_EFFECT>( deviceConfig[ CONFIG_TRANS_EFFECT ].toInt(YeelightLight::API_EFFECT_SMOOTH) );
		}

		_transitionDuration = deviceConfig[ CONFIG_TRANS_TIME ].toInt(API_PARAM_DURATION.count());
		_extraTimeDarkness	= _devConfig[CONFIG_EXTRA_TIME_DARKNESS].toInt(0);

		_brightnessMin		= _devConfig[CONFIG_BRIGHTNESS_MIN].toInt(0);
		_isBrightnessSwitchOffMinimum = _devConfig[CONFIG_BRIGHTNESS_SWITCHOFF].toBool(true);
		_brightnessMax		= _devConfig[CONFIG_BRIGHTNESS_MAX].toInt(100);
		_brightnessFactor	= _devConfig[CONFIG_BRIGHTNESSFACTOR].toDouble(1.0);

		if (  deviceConfig[ CONFIG_DEBUGLEVEL ].isString() )
		{
			_debuglevel = deviceConfig[ CONFIG_DEBUGLEVEL ].toString(QString("0")).toInt();
		}
		else
		{
			_debuglevel = deviceConfig[ CONFIG_DEBUGLEVEL ].toInt(0);
		}

		QString outputColorModel = _outputColorModel == MODEL_RGB ? "RGB": "HSV";
		QString transitionEffect = _transitionEffect == YeelightLight::API_EFFECT_SMOOTH ? API_PARAM_EFFECT_SMOOTH : API_PARAM_EFFECT_SUDDEN;

		Debug(_log, "colorModel        : %s", QSTRING_CSTR(outputColorModel));
		Debug(_log, "Transitioneffect  : %s", QSTRING_CSTR(transitionEffect));
		Debug(_log, "Transitionduration: %d", _transitionDuration);
		Debug(_log, "Extra time darkn. : %d", _extraTimeDarkness );

		Debug(_log, "Brightn. Min      : %d", _brightnessMin );
		Debug(_log, "Brightn. Min Off  : %d", _isBrightnessSwitchOffMinimum );
		Debug(_log, "Brightn. Max      : %d", _brightnessMax );
		Debug(_log, "Brightn. Factor   : %.2f", _brightnessFactor );

		_isRestoreOrigState     = _devConfig[CONFIG_RESTORE_STATE].toBool(false);
		Debug(_log, "RestoreOrigState  : %d", _isRestoreOrigState);

		_waitTimeQuota	= _devConfig[CONFIG_QUOTA_WAIT_TIME].toInt(0);
		Debug(_log, "Wait time (quota) : %d", _waitTimeQuota );

		Debug(_log, "Debuglevel        : %d", _debuglevel);

		QJsonArray configuredYeelightLights   = _devConfig[CONFIG_LIGHTS].toArray();
		int configuredYeelightsCount = 0;
		for (const QJsonValueRef light : configuredYeelightLights)
		{
			QString hostName = light.toObject().value(CONFIG_HOST).toString();
			int port = light.toObject().value(CONFIG_PORT).toInt(API_DEFAULT_PORT);

			if ( !hostName.isEmpty() )
			{
				QString name = light.toObject().value("name").toString();
				Debug(_log, "Light [%u] - %s (%s:%d)", configuredYeelightsCount, QSTRING_CSTR(name), QSTRING_CSTR(hostName), port );
				++configuredYeelightsCount;
			}
		}
		Debug(_log, "Light configured  : %d", configuredYeelightsCount );

		int configuredLedCount = this->getLedCount();
		if (configuredYeelightsCount < configuredLedCount )
		{
			QString errorReason = QString("Not enough Yeelights [%1] for configured LEDs [%2] found!")
									  .arg(configuredYeelightsCount)
									  .arg(configuredLedCount);
			this->setInError(errorReason);
			isInitOK = false;
		}
		else
		{

			if (  configuredYeelightsCount > configuredLedCount )
			{
				Warning(_log, "More Yeelights defined [%d] than configured LEDs [%d].", configuredYeelightsCount, configuredLedCount );
			}

			_lightsAddressList.clear();
			for (int j = 0; j < static_cast<int>( configuredLedCount ); ++j)
			{
				QString hostName = configuredYeelightLights[j].toObject().value(CONFIG_HOST).toString();
				int port = configuredYeelightLights[j].toObject().value(CONFIG_PORT).toInt(API_DEFAULT_PORT);
				_lightsAddressList.append( { hostName, port} );
			}

			if ( updateLights(_lightsAddressList) )
			{
				isInitOK = true;
			}
		}
	}
	return isInitOK;
}

bool LedDeviceYeelight::startMusicModeServer()
{
	DebugIf(verbose, _log, "enabled [%d], _isDeviceReady [%d]", _isEnabled, _isDeviceReady);

	bool rc = false;
	if ( _tcpMusicModeServer == nullptr )
	{
		_tcpMusicModeServer = new QTcpServer(this);
	}

	if ( ! _tcpMusicModeServer->isListening() )
	{
		if (! _tcpMusicModeServer->listen(QHostAddress::AnyIPv4))
		{
			QString errorReason = QString ("Failed to start music mode server: (%1) %2").arg(_tcpMusicModeServer->serverError()).arg( _tcpMusicModeServer->errorString());
			this->setInError ( errorReason );
		}
		else
		{
			// use the first non-localhost IPv4 address, IPv6 are not supported by Yeelight currently
			for (const auto& address : QNetworkInterface::allAddresses())
			{
				// is valid when, no loopback, IPv4
				if (!address.isLoopback() && (address.protocol() == QAbstractSocket::IPv4Protocol))
				{
					_musicModeServerAddress = address;
					break;
				}
			}
			if (_musicModeServerAddress.isNull())
			{
				_tcpMusicModeServer->close();
				QString errorReason = QString ("Network error - failed to resolve IP for music mode server");
				this->setInError ( errorReason );
			}
		}
	}

	if ( !_isDeviceInError && _tcpMusicModeServer->isListening() )
	{
		_musicModeServerPort = _tcpMusicModeServer->serverPort();
		Debug (_log, "The music mode server is running at %s:%d", QSTRING_CSTR(_musicModeServerAddress.toString()), _musicModeServerPort);
		rc = true;
	}
	DebugIf(verbose, _log, "rc [%d], enabled [%d], _isDeviceReady [%d]", rc, _isEnabled, _isDeviceReady);
	return rc;
}

bool LedDeviceYeelight::stopMusicModeServer()
{
	DebugIf(verbose, _log, "enabled [%d], _isDeviceReady [%d]", _isEnabled, _isDeviceReady);

	bool rc = false;
	if ( _tcpMusicModeServer != nullptr )
	{
		Debug(_log, "Stop MusicModeServer");
		_tcpMusicModeServer->close();
		rc = true;
	}
	DebugIf(verbose, _log, "rc [%d], enabled [%d], _isDeviceReady [%d]", rc, _isEnabled, _isDeviceReady);
	return rc;
}

int LedDeviceYeelight::open()
{
	DebugIf(verbose, _log, "enabled [%d], _isDeviceReady [%d]", _isEnabled, _isDeviceReady);
	int retval = -1;
	_isDeviceReady = false;

	// Open/Start LedDevice based on configuration
	if ( !_lights.empty() )
	{
		if ( startMusicModeServer() )
		{
			int lightsInError = 0;
			for (YeelightLight& light : _lights)
			{
				light.setTransitionEffect( _transitionEffect, _transitionDuration );
				light.setBrightnessConfig( _brightnessMin, _brightnessMax, _isBrightnessSwitchOffMinimum, _extraTimeDarkness, _brightnessFactor );
				light.setQuotaWaitTime(_waitTimeQuota);
				light.setDebuglevel(_debuglevel);

				if ( ! light.open() )
				{
					Error( _log, "Failed to open [%s]", QSTRING_CSTR(light.getName()) );
					++lightsInError;
				}
			}
			if ( lightsInError < static_cast<int>(_lights.size()) )
			{
				// Everything is OK -> enable device
				_isDeviceReady = true;
				retval = 0;
			}
			else
			{
				this->setInError( "All Yeelights failed to be opened!" );
			}
		}
	}
	else
	{
		// On error/exceptions, set LedDevice in error
	}

	DebugIf(verbose, _log, "retval [%d], enabled [%d], _isDeviceReady [%d]", retval, _isEnabled, _isDeviceReady);
	return retval;
}

int LedDeviceYeelight::close()
{
	DebugIf(verbose, _log, "enabled [%d], _isDeviceReady [%d]", _isEnabled, _isDeviceReady);
	int retval = 0;
	_isDeviceReady = false;

	// LedDevice specific closing activities

	//Close all Yeelight lights
	for (YeelightLight& light : _lights)
	{
		light.close();
	}

	//Close music mode server
	stopMusicModeServer();

	DebugIf(verbose, _log, "retval [%d], enabled [%d], _isDeviceReady [%d]", retval, _isEnabled, _isDeviceReady);
	return retval;
}

bool LedDeviceYeelight::updateLights(const QVector<yeelightAddress> &list)
{
	bool rc = false;
	DebugIf(verbose, _log, "enabled [%d], _isDeviceReady [%d]", _isEnabled, _isDeviceReady);
	if(!_lightsAddressList.empty())
	{
		// search user light-id inside map and create light if found
		_lights.clear();

		_lights.reserve( static_cast<ulong>( _lightsAddressList.size() ));

		for(auto & yeelightAddress : _lightsAddressList )
		{
			QString host = yeelightAddress.host;

			if ( list.contains(yeelightAddress) )
			{
				int port = yeelightAddress.port;

				Debug(_log,"Add Yeelight %s:%d", QSTRING_CSTR(host), port );
				_lights.emplace_back( _log, host, port );
			}
			else
			{
				Warning(_log,"Configured light-address %s is not available", QSTRING_CSTR(host) );
			}
		}
		setLightsCount ( static_cast<uint>( _lights.size() ));
		rc = true;
	}
	return rc;
}

bool LedDeviceYeelight::powerOn()
{
	if ( _isDeviceReady)
	{
		//Power-on all Yeelights
		for (YeelightLight& light : _lights)
		{
			if ( light.isReady() && !light.isInMusicMode() )
			{
				light.setPower(true, YeelightLight::API_EFFECT_SMOOTH, 5000);
			}
		}
	}
	return true;
}

bool LedDeviceYeelight::powerOff()
{
	if ( _isDeviceReady)
	{
		writeBlack();

		//Power-off all Yeelights
		for (YeelightLight& light : _lights)
		{
			light.setPower( false, _transitionEffect, API_PARAM_DURATION_POWERONOFF.count());
		}
	}
	return true;
}

bool LedDeviceYeelight::storeState()
{
	bool rc = true;

	for (YeelightLight& light : _lights)
	{
		light.storeState();
	}
	return rc;
}

bool LedDeviceYeelight::restoreState()
{
	bool rc = true;

	for (YeelightLight& light : _lights)
	{
		light.restoreState();
		if ( !light.wasOriginallyOn() )
		{
			light.setPower( false, _transitionEffect, API_PARAM_DURATION_POWERONOFF.count());
		}
	}
	return rc;
}

QJsonArray LedDeviceYeelight::discover()
{
	QJsonArray deviceList;

	SSDPDiscover discover;
	discover.setPort(SSDP_PORT);
	discover.skipDuplicateKeys(true);
	discover.setSearchFilter(SSDP_FILTER, SSDP_FILTER_HEADER);
	QString searchTarget = SSDP_ID;

	if ( discover.discoverServices(searchTarget) > 0 )
	{
		deviceList = discover.getServicesDiscoveredJson();
	}
	return deviceList;
}

QJsonObject LedDeviceYeelight::discover(const QJsonObject& /*params*/)
{
	QJsonObject devicesDiscovered;
	devicesDiscovered.insert("ledDeviceType", _activeDeviceType );

	QJsonArray deviceList;

#ifdef ENABLE_MDNS
	QString discoveryMethod("mDNS");
	deviceList = MdnsBrowser::getInstance().getServicesDiscoveredJson(
		MdnsServiceRegister::getServiceType(_activeDeviceType),
		MdnsServiceRegister::getServiceNameFilter(_activeDeviceType),
		DEFAULT_DISCOVER_TIMEOUT
		);
#else
	QString discoveryMethod("ssdp");
	deviceList = discover();
#endif

	devicesDiscovered.insert("discoveryMethod", discoveryMethod);
	devicesDiscovered.insert("devices", deviceList);

	DebugIf(verbose,_log, "devicesDiscovered: [%s]", QString(QJsonDocument(devicesDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return devicesDiscovered;
}

QJsonObject LedDeviceYeelight::getProperties(const QJsonObject& params)
{
	DebugIf(verbose,_log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );
	QJsonObject properties;

	QString hostName = params[CONFIG_HOST].toString("");
	quint16 apiPort = static_cast<quint16>( params[CONFIG_PORT].toInt(API_DEFAULT_PORT) );

	Info(_log, "Get properties for %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(hostName) );

	QHostAddress address;
	if (NetUtils::resolveHostToAddress(_log, hostName, address))
	{
		YeelightLight yeelight(_log, address.toString(), apiPort);
		if ( yeelight.open() )
		{
			properties.insert("properties", yeelight.getProperties());
			yeelight.close();
		}
	}
	DebugIf(verbose, _log, "properties: [%s]", QString(QJsonDocument(properties).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	return properties;
}

void LedDeviceYeelight::identify(const QJsonObject& params)
{
	DebugIf(verbose,_log,  "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData() );

	QString hostName = params[CONFIG_HOST].toString("");
	quint16 apiPort = static_cast<quint16>( params[CONFIG_PORT].toInt(API_DEFAULT_PORT) );

	Info(_log, "Identify %s, hostname (%s)", QSTRING_CSTR(_activeDeviceType), QSTRING_CSTR(hostName) );

	QHostAddress address;
	if (NetUtils::resolveHostToAddress(_log, hostName, address))
	{
		YeelightLight yeelight(_log, address.toString(), apiPort);
		if ( yeelight.open() )
		{
			yeelight.identify();
			yeelight.close();
		}
	}
}

int LedDeviceYeelight::write(const std::vector<ColorRgb> & ledValues)
{
	int rc = -1;

	//Update on all Yeelights by iterating through lights and set colors.
	unsigned int idx = 0;
	int lightsInError = 0;
	for (YeelightLight& light : _lights)
	{
		// Get color
		ColorRgb color = ledValues.at(idx);

		if ( light.isReady()  )
		{
			bool skipWrite = false;
			if ( !light.isInMusicMode() )
			{
				if ( light.setMusicMode(true, _musicModeServerAddress, _musicModeServerPort) )
				{
					// Wait for callback of the device to establish streaming socket
					if ( _tcpMusicModeServer->waitForNewConnection(CONNECT_STREAM_TIMEOUT.count()) )
					{
						light.setStreamSocket( _tcpMusicModeServer->nextPendingConnection() );
					}
					else
					{
						QString errorReason = QString("(%1) %2").arg(_tcpMusicModeServer->serverError()).arg(_tcpMusicModeServer->errorString());
						if (_tcpMusicModeServer->serverError() == QAbstractSocket::TemporaryError)
						{
							Info(_log, "Ignore write Error [%s]: _tcpMusicModeServer: %s", QSTRING_CSTR(light.getName()), QSTRING_CSTR(errorReason));
							skipWrite = true;
						}
						else
						{
							Warning(_log, "write Error [%s]: _tcpMusicModeServer: %s", QSTRING_CSTR(light.getName()), QSTRING_CSTR(errorReason));
							light.setInError("Failed to get stream socket");
						}
				}
				}
				else
				{
					DebugIf(verbose,_log, "setMusicMode failed due to command quota issue, skip write and try with next");
					skipWrite = true;
				}
			}

			if ( !skipWrite )
			{
				// Update light with given color
				if ( _outputColorModel == MODEL_RGB )
				{
					light.setColorRGB( color );
				}
				else
				{
					light.setColorHSV( color );
				}
			}
		}
		else
		{
			++lightsInError;
		}
		++idx;
	}

	if ( ! (lightsInError < static_cast<int>(_lights.size())) )
	{
		this->setInError( "All Yeelights in error - stopping device!" );
	}
	else
	{
		// Minimum one Yeelight device is working, continue updating devices
		rc = 0;
	}
	return rc;
}
