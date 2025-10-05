#include "LedDeviceSkydimo.h"
#include "utils/Logger.h"

#include <QtEndian>

// Constants
namespace {

constexpr int HEADER_SIZE {6};

} //End of constants

LedDeviceSkydimo::LedDeviceSkydimo(const QJsonObject &deviceConfig)
	: ProviderRs232(deviceConfig)
{
}

LedDevice* LedDeviceSkydimo::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceSkydimo(deviceConfig);
}

bool LedDeviceSkydimo::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if ( !ProviderRs232::init(deviceConfig) )
	{
		return false;
	}

	prepareHeader();

	return true;
}

void LedDeviceSkydimo::prepareHeader()
{
	_bufferLength = static_cast<qint64>(HEADER_SIZE + _ledRGBCount);
	_ledBuffer.fill(0x00, _bufferLength);
	_ledBuffer[0] = 'A';
	_ledBuffer[1] = 'd';
	_ledBuffer[2] = 'a';
	_ledBuffer[3] = 0;
	_ledBuffer[4] = 0;
	_ledBuffer[5] = static_cast<quint8>(_ledCount);

	Debug( _log, "Skydimo header for %d leds (size: %d): %c%c%c 0x%02x 0x%02x 0x%02x", _ledCount, _ledBuffer.size(),
		   _ledBuffer[0], _ledBuffer[1], _ledBuffer[2], _ledBuffer[3], _ledBuffer[4], _ledBuffer[5] );
}

int LedDeviceSkydimo::write(const QVector<ColorRgb> & ledValues)
{
	if (_ledCount != ledValues.size())
	{
		Warning(_log, "Skydimo LED count has changed (old: %d, new: %d). Rebuilding header.", _ledCount, ledValues.size());
		_ledCount = static_cast<uint>(ledValues.size());
		_ledRGBCount = _ledCount * 3;
		prepareHeader();
	}

	if (_bufferLength >  _ledBuffer.size())
	{
		Warning(_log, "Skydimo buffer's size has changed. Skipping refresh.");
		return 0;
	}

	assert(HEADER_SIZE + ledValues.size() * sizeof(ColorRgb) <= _ledBuffer.size());

	memcpy(HEADER_SIZE + _ledBuffer.data(), ledValues.data(), ledValues.size() * sizeof(ColorRgb));

	return writeBytes(_bufferLength, _ledBuffer.data());
}
