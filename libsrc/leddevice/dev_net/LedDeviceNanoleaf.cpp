// Local-Hyperion includes
#include "LedDeviceNanoleaf.h"

// ssdp discover
#include <ssdp/SSDPDiscover.h>

// Qt includes
#include <QEventLoop>
#include <QNetworkReply>

// Controller configuration settings
const QString CONFIG_ADDRESS = "output";
const QString CONFIG_PORT = "port";
const QString CONFIG_AUTH_TOKEN ="token";

// Panel configuration settings
const QString PANEL_LAYOUT = "layout";
const QString PANEL_NUM = "numPanels";
const QString PANEL_ID = "panelId";
const QString PANEL_POSITIONDATA = "positionData";
const QString PANEL_SHAPE_TYPE = "shapeType";
const QString PANEL_ORIENTATION = "0";
const QString PANEL_POS_X = "x";
const QString PANEL_POS_Y = "y";

// List of State Information
const QString STATE_ON = "on";
const QString STATE_ONOFF_VALUE = "value";
const QString STATE_VALUE_TRUE = "true";
const QString STATE_VALUE_FALSE = "false";

//Device Data elements
const QString DEV_DATA_NAME = "name";
const QString DEV_DATA_MODEL = "model";
const QString DEV_DATA_MANUFACTURER = "manufacturer";
const QString DEV_DATA_FIRMWAREVERSION = "firmwareVersion";

//Nanoleaf Stream Control elements
const QString STREAM_CONTROL_IP = "streamControlIpAddr";
const QString STREAM_CONTROL_PORT = "streamControlPort";
const QString STREAM_CONTROL_PROTOCOL = "streamControlProtocol";
const quint16 STREAM_CONTROL_DEFAULT_PORT = 60222; //Fixed port for Canvas;

// Nanoleaf OpenAPI URLs
const QString API_DEFAULT_PORT = "16021";
const QString API_URL_FORMAT = "http://%1:%2/api/v1/%3/%4";
const QString API_ROOT = "";
const QString API_EXT_MODE_STRING_V1 = "{\"write\" : {\"command\" : \"display\", \"animType\" : \"extControl\"}}";
const QString API_EXT_MODE_STRING_V2 = "{\"write\" : {\"command\" : \"display\", \"animType\" : \"extControl\", \"extControlVersion\" : \"v2\"}}";
const QString API_STATE ="state";
const QString API_PANELLAYOUT = "panelLayout";
const QString API_EFFECT = "effects";

//Nanoleaf ssdp services
const QString SSDP_CANVAS = "nanoleaf:nl29";
const QString SSDP_LIGHTPANELS = "nanoleaf_aurora:light";
const int SSDP_TIMEOUT = 5000; // timout in ms

// Nanoleaf Panel Shapetypes
enum SHAPETYPES {
    TRIANGLE,
    RHYTM,
    SQUARE,
    CONTROL_SQUARE_PRIMARY,
    CONTROL_SQUARE_PASSIVE,
    POWER_SUPPLY,
};

// Nanoleaf external control versions
enum EXTCONTROLVERSIONS {
    EXTCTRLVER_V1 = 1,
    EXTCTRLVER_V2
};

LedDevice* LedDeviceNanoleaf::construct(const QJsonObject &deviceConfig)
{
    return new LedDeviceNanoleaf(deviceConfig);
}

LedDeviceNanoleaf::LedDeviceNanoleaf(const QJsonObject &deviceConfig)
    : ProviderUdp()
{
    init(deviceConfig);
}

bool LedDeviceNanoleaf::init(const QJsonObject &deviceConfig) {

    LedDevice::init(deviceConfig);

    int configuredLedCount = this->getLedCount();
    Debug(_log, "ActiveDevice : %s", QSTRING_CSTR( this->getActiveDevice() ));
    Debug(_log, "LedCount     : %d", configuredLedCount);
    Debug(_log, "ColorOrder   : %s", QSTRING_CSTR( this->getColorOrder() ));
    Debug(_log, "LatchTime    : %d", this->getLatchTime());

    //Set hostname as per configuration and default port
    _hostname   = deviceConfig[ CONFIG_ADDRESS ].toString();
    _api_port   = API_DEFAULT_PORT;
    _auth_token = deviceConfig[ CONFIG_AUTH_TOKEN ].toString();

    //If host not configured then discover device
    if ( _hostname.isEmpty() )
        //Discover Nanoleaf device
        if ( !discoverNanoleafDevice() ) {
            Error(_log, "No target IP defined nor Nanoleaf device discovered");
            return false;
        }

    //Get Nanoleaf device details and configuration
    _networkmanager = new QNetworkAccessManager();

    // Read Panel count and panel Ids
    QString url = getUrl(_hostname, _api_port, _auth_token, API_ROOT );
    QJsonDocument doc = getJson( url );

    QJsonObject jsonAllPanelInfo = doc.object();

    QString deviceName = jsonAllPanelInfo[DEV_DATA_NAME].toString();
    _deviceModel = jsonAllPanelInfo[DEV_DATA_MODEL].toString();
    QString deviceManufacturer = jsonAllPanelInfo[DEV_DATA_MANUFACTURER].toString();
    _deviceFirmwareVersion = jsonAllPanelInfo[DEV_DATA_FIRMWAREVERSION].toString();

    Debug(_log, "Name           : %s", QSTRING_CSTR( deviceName ));
    Debug(_log, "Model          : %s", QSTRING_CSTR( _deviceModel ));
    Debug(_log, "Manufacturer   : %s", QSTRING_CSTR( deviceManufacturer ));
    Debug(_log, "FirmwareVersion: %s", QSTRING_CSTR( _deviceFirmwareVersion));

    // Get panel details from /panelLayout/layout
    QJsonObject jsonPanelLayout = jsonAllPanelInfo[API_PANELLAYOUT].toObject();
    QJsonObject jsonLayout = jsonPanelLayout[PANEL_LAYOUT].toObject();

    int panelNum = jsonLayout[PANEL_NUM].toInt();
    QJsonArray positionData = jsonLayout[PANEL_POSITIONDATA].toArray();

    std::map<int, std::map<int, int>> panelMap;

    // Loop over all children.
    foreach (const QJsonValue & value, positionData) {
        QJsonObject panelObj = value.toObject();

        int panelId = panelObj[PANEL_ID].toInt();
        int panelX  = panelObj[PANEL_POS_X].toInt();
        int panelY  = panelObj[PANEL_POS_Y].toInt();
        int panelshapeType = panelObj[PANEL_SHAPE_TYPE].toInt();
        //int panelOrientation = panelObj[PANEL_ORIENTATION].toInt();
        //std::cout << "Panel [" << panelId << "]" << " (" << panelX << "," << panelY << ") - Type: [" << panelshapeType << "]" << std::endl;

        // Skip Rhythm panels
        if ( panelshapeType != RHYTM ) {
            panelMap[panelY][panelX] = panelId;
        } else  {
            Info(_log, "Rhythm panel skipped.");
        }
    }

    // Sort panels top down, left right
    for(auto posY = panelMap.crbegin(); posY != panelMap.crend(); ++posY) {
        // posY.first is the first key
        for(auto const &posX : posY->second) {
            // posX.first is the second key, posX.second is the data
            //std::cout << "panelMap[" << posY->first << "][" << posX.first << "]=" << posX.second << std::endl;
            _panelIds.push_back(posX.second);
        }
    }
    this->_panelLedCount = _panelIds.size();


    Debug(_log, "PanelsNum      : %d", panelNum);
    Debug(_log, "PanelLedCount  : %d", _panelLedCount);

    // Check. if enough panelds were found.
    if (_panelLedCount < configuredLedCount) {

        throw std::runtime_error ( (QString ("Not enough panels [%1] for configured LEDs [%2] found!").arg(_panelLedCount).arg(configuredLedCount)).toStdString() );
    } else {
        if ( _panelLedCount > this->getLedCount() ) {
            Warning(_log, "Nanoleaf: More panels [%d] than configured LEDs [%d].", _panelLedCount, configuredLedCount );
        }
    }

    switchOn();

    // Set Nanoleaf to External Control (UDP) mode
    Debug(_log, "Set Nanoleaf to External Control (UDP) streaming mode");
    QJsonDocument responseDoc = changeToExternalControlMode();

    // Set UDP streaming port
    _port = STREAM_CONTROL_DEFAULT_PORT;

    // Resolve port for Ligh Panels
    QJsonObject jsonStreamControllInfo = responseDoc.object();
    if ( ! jsonStreamControllInfo.isEmpty() ) {
        _port = jsonStreamControllInfo[STREAM_CONTROL_PORT].toInt();
    }

    _defaultHost = _hostname;
    ProviderUdp::init(deviceConfig);

    Debug(_log, "Started successfully" );
    return true;
}

bool LedDeviceNanoleaf::discoverNanoleafDevice() {

    bool isDeviceFound (false);
    // device searching by ssdp
    QString address;
    SSDPDiscover discover;

    // Discover Canvas device
    address = discover.getFirstService(STY_WEBSERVER, SSDP_CANVAS, SSDP_TIMEOUT);

    //No Canvas device not found
    if ( address.isEmpty() ) {
        // Discover Light Panels (Aurora) device
        address = discover.getFirstService(STY_WEBSERVER, SSDP_LIGHTPANELS, SSDP_TIMEOUT);

        if ( address.isEmpty() ) {
            Warning(_log, "No Nanoleaf device discovered");
        }
    }

    // Canvas or Light Panels found
    if ( ! address.isEmpty() ) {
        Info(_log, "Nanoleaf device discovered at [%s]", QSTRING_CSTR( address ));
        isDeviceFound = true;
        QStringList addressparts = address.split(":", QString::SkipEmptyParts);
        _hostname = addressparts[0];
        _api_port = addressparts[1];
    }
    return isDeviceFound;
}

QJsonDocument LedDeviceNanoleaf::changeToExternalControlMode() {

    QString url = getUrl(_hostname, _api_port, _auth_token, API_EFFECT );
    QJsonDocument jsonDoc;
    // If device model is Light Panels (Aurora)
    if ( _deviceModel == "NL22") {
        _extControlVersion = EXTCTRLVER_V1;
        //Enable UDP Mode v1
        jsonDoc = putJson(url, API_EXT_MODE_STRING_V1);
    }
    else {
        _extControlVersion = EXTCTRLVER_V2;
        //Enable UDP Mode v2
        jsonDoc= putJson(url, API_EXT_MODE_STRING_V2);
    }
    return jsonDoc;
}

QString LedDeviceNanoleaf::getUrl(QString host, QString port, QString auth_token, QString endpoint) const {
    return API_URL_FORMAT.arg(host).arg(port).arg(auth_token).arg(endpoint);
}

QJsonDocument LedDeviceNanoleaf::getJson(QString url) const {

    Debug(_log, "GET: [%s]", QSTRING_CSTR( url ));

    // Perfrom request
    QNetworkRequest request(url);
    QNetworkReply* reply = _networkmanager->get(request);
    // Connect requestFinished signal to quit slot of the loop.
    QEventLoop loop;
    loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
    // Go into the loop until the request is finished.
    loop.exec();

    QJsonDocument jsonDoc;
    if(reply->operation() == QNetworkAccessManager::GetOperation)
    {
        jsonDoc = handleReply( reply );
    }
    // Free space.
    reply->deleteLater();
    // Return response
    return jsonDoc;
}

QJsonDocument LedDeviceNanoleaf::putJson(QString url, QString json) const {

    Debug(_log, "PUT: [%s] [%s]", QSTRING_CSTR( url ), QSTRING_CSTR( json ) );
    // Perfrom request
    QNetworkRequest request(url);
    QNetworkReply* reply = _networkmanager->put(request, json.toUtf8());
    // Connect requestFinished signal to quit slot of the loop.
    QEventLoop loop;
    loop.connect(reply, SIGNAL(finished()), SLOT(quit()));
    // Go into the loop until the request is finished.
    loop.exec();

    QJsonDocument jsonDoc;
    if(reply->operation() == QNetworkAccessManager::PutOperation)
    {
        jsonDoc = handleReply( reply );
    }
    // Free space.
    reply->deleteLater();

    // Return response
    return jsonDoc;
}

QJsonDocument LedDeviceNanoleaf::handleReply(QNetworkReply* const &reply ) const  {

    QJsonDocument jsonDoc;

    int httpStatusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if(reply->error() ==
            QNetworkReply::NoError)
    {

        if ( httpStatusCode != 204 ){
            QByteArray response = reply->readAll();
            QJsonParseError error;
            jsonDoc = QJsonDocument::fromJson(response, &error);
            if (error.error != QJsonParseError::NoError)
            {
                Error (_log, "Got invalid response");
                throw std::runtime_error("");
            }
            else {
                //Debug
                //  QString strJson(jsonDoc.toJson(QJsonDocument::Compact));
                //  std::cout << strJson.toUtf8().constData() << std::endl;
            }
        }
    }
    else
    {
        QString errorReason;
        if ( httpStatusCode > 0 ) {
            QString httpReason = reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
            QString advise;
            switch ( httpStatusCode ) {
            case 400:
                advise = "Check Request Body";
                break;
            case 401:
                advise = "Check Authentication Token (API Key)";
                break;
            case 404:
                advise = "Check Resource given";
                break;
            default:
                break;
            }
            errorReason = QString ("%1:%2 [%3 %4] - %5").arg(_hostname).arg(_api_port).arg(httpStatusCode).arg(httpReason).arg(advise);
        }
        else {
            errorReason = QString ("%1:%2 - %3").arg(_hostname).arg(_api_port).arg(reply->errorString());
        }
        Error (_log, "%s", QSTRING_CSTR( errorReason ));
        throw std::runtime_error("Network Error");
    }
    // Return response
    return jsonDoc;
}


LedDeviceNanoleaf::~LedDeviceNanoleaf()
{
    delete _networkmanager;
}

int LedDeviceNanoleaf::write(const std::vector<ColorRgb> & ledValues)
{

    int retVal = 0;
    uint udpBufferSize;

    //Light Panels
    //    nPanels         1B
    //    nFrames         1B
    //    panelID         1B
    //    <R> <G> <B>     3B
    //    <W>             1B
    //    tranitionTime   1B
    //
    //Canvas
    //In order to support the much larger number of panels on Canvas, the size of the nPanels,
    //panelId and tranitionTime fields have been been increased from 1B to 2B.
    //The nFrames field has been dropped as it was set to 1 in v1 anyway
    //
    //    nPanels         2B
    //    panelID         2B
    //    <R> <G> <B>     3B
    //    <W>             1B
    //    tranitionTime   2B


    //udpBufferSize = _panelLedCount * 7 + 1; // Buffersize for LightPanels

    udpBufferSize = _panelLedCount * 8 + 2;
    uint8_t udpbuffer[udpBufferSize];

    uchar lowByte;  // lower byte
    uchar highByte; // upper byte

    uint i=0;

    // Set number of panels
    highByte = (uchar) (_panelLedCount >>8   );
    lowByte  = (uchar) (_panelLedCount & 0xFF);

    if ( _extControlVersion == EXTCTRLVER_V2 ) {
        udpbuffer[i++] = highByte;
    }
    udpbuffer[i++] = lowByte;

    ColorRgb color;
    for ( int panelCounter=0; panelCounter < _panelLedCount; panelCounter++ )
    {
        uint panelID = _panelIds[panelCounter];

        highByte = (uchar) (panelID >>8   );
        lowByte  = (uchar) (panelID & 0xFF);

        // Set panels configured
        if( panelCounter < this->getLedCount() ) {
            color = (ColorRgb) ledValues.at(panelCounter);
        }
        else
        {
            // Set panels not configed to black;
            color = ColorRgb::BLACK;
            //printf ("panelCounter [%d] >= panelLedCount [%d]\n", panelCounter, _panelLedCount );
        }

        // Set panelID
        if ( _extControlVersion == EXTCTRLVER_V2 ) {
            udpbuffer[i++] = highByte;
        }
        udpbuffer[i++] = lowByte;

        // Set number of frames - V1 only
        if ( _extControlVersion == EXTCTRLVER_V1 ) {
            udpbuffer[i++] = 1; // No of Frames
        }

        // Set panel's color LEDs
        udpbuffer[i++] = color.red;
        udpbuffer[i++] = color.green;
        udpbuffer[i++] = color.blue;

        // Set white LED
        udpbuffer[i++] = 0; // W not set manually

        // Set transition time
        unsigned char tranitionTime = 1; // currently fixed at value 1 which corresponds to 100ms

        highByte = (uchar) (tranitionTime >>8   );
        lowByte  = (uchar) (tranitionTime & 0xFF);

        if ( _extControlVersion == EXTCTRLVER_V2 ) {
            udpbuffer[i++] = highByte;
        }
        udpbuffer[i++] = lowByte;

        //std::cout << "[" << panelCounter << "]" << " Color: " << color << std::endl;
    }

    //     printf ("udpBufferSize[%d], Bytes to send [%d]\n", udpBufferSize, i);
    //     for ( uint c= 0; c < udpBufferSize;c++ )
    //     {
    //        printf ("%x ",  (uchar) udpbuffer[c]);
    //     }
    //     printf("\n");

    retVal &= writeBytes( i , udpbuffer);
    return retVal;
}

QString LedDeviceNanoleaf::getOnOffRequest (bool isOn ) const {
    QString state = isOn ? STATE_VALUE_TRUE : STATE_VALUE_FALSE;
    return QString( "{\"%1\":{\"%2\":%3}}" ).arg(STATE_ON).arg(STATE_ONOFF_VALUE).arg(state);
}

int LedDeviceNanoleaf::switchOn() {
    Debug(_log, "switchOn()");
    //Switch on Nanoleaf device
    QString url = getUrl(_hostname, _api_port, _auth_token, API_STATE );
    putJson(url, this->getOnOffRequest(true) );

    return 0;
}

int LedDeviceNanoleaf::switchOff() {
    Debug(_log, "switchOff()");

    //Set all LEDs to Black
    LedDevice::switchOff();

    //Switch off Nanoleaf device physically
    QString url = getUrl(_hostname, _api_port, _auth_token, API_STATE );
    putJson(url, getOnOffRequest(false) );

    return _deviceReady ? write(std::vector<ColorRgb>(_ledCount, ColorRgb::BLACK )) : -1;

    return 0;
}
