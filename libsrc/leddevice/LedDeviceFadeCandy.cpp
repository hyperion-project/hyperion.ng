#include "LedDeviceFadeCandy.h"

static const unsigned MAX_NUM_LEDS    = 512;
static const unsigned OPC_BROADCAST   = 0;    // OPC broadcast channel
static const unsigned OPC_SET_PIXELS  = 0;    // OPC command codes
static const unsigned OPC_HEADER_SIZE = 4;    // OPC header size


LedDeviceFadeCandy::LedDeviceFadeCandy(const std::string& host, const uint16_t port) :
	_host(host), _port(port)
{
	_opc_data.resize( OPC_HEADER_SIZE );
	_opc_data[0] = OPC_BROADCAST;
	_opc_data[1] = OPC_SET_PIXELS;
	_opc_data[2] = 0;
	_opc_data[3] = 0;
}


LedDeviceFadeCandy::~LedDeviceFadeCandy()
{
	_client.close();
}


bool LedDeviceFadeCandy::isConnected()
{
	return _client.state() == QAbstractSocket::ConnectedState;
}


bool LedDeviceFadeCandy::tryConnect()
{
	if (  _client.state() == QAbstractSocket::UnconnectedState ) {
		qDebug("connecting to %s %i",_host.c_str(),_port);

		_client.connectToHost( _host.c_str(), _port);
		if ( _client.waitForConnected(1000) )
			qDebug("connected");
	}

	return isConnected();
}


int LedDeviceFadeCandy::write( const std::vector<ColorRgb> & ledValues )
{
	ssize_t nrLedValues = ledValues.size();
	ssize_t led_data_size = nrLedValues * 3;    // 3 color bytes
	ssize_t opc_data_size = led_data_size + OPC_HEADER_SIZE;

	if (nrLedValues > MAX_NUM_LEDS)
	{
		std::cerr << "Invalid attempt to write led values. Not more than " << MAX_NUM_LEDS << " leds are allowed." << std::endl;
		return -1;
	}

	if ( opc_data_size != _opc_data.size() )
		_opc_data.resize( opc_data_size );

	_opc_data[2] = led_data_size >> 8;
	_opc_data[3] = led_data_size & 0xff;

	uint idx = OPC_HEADER_SIZE;
	for (const ColorRgb& color : ledValues)
	{
		_opc_data[idx  ] = unsigned( color.red   );
		_opc_data[idx+1] = unsigned( color.green );
		_opc_data[idx+2] = unsigned( color.blue  );
		idx += 3;
	}

	return ( transferData()<0 ? -1 : 0 );
}


int LedDeviceFadeCandy::transferData()
{
	if ( isConnected() || tryConnect() )
		return _client.write( _opc_data, _opc_data.size() );

	return -2;
}


int LedDeviceFadeCandy::switchOff()
{
	for ( int idx=OPC_HEADER_SIZE; idx < _opc_data.size(); idx++ )
		_opc_data[idx] = 0;

	return ( transferData()<0 ? -1 : 0 );
}

