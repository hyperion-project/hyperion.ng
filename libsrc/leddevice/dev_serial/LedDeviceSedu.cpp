#include "LedDeviceSedu.h"

struct FrameSpec
{
	uint8_t id;
	size_t size;
};

LedDeviceSedu::LedDeviceSedu(const QJsonObject &deviceConfig)
	: ProviderRs232(deviceConfig)
{
}

LedDevice* LedDeviceSedu::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceSedu(deviceConfig);
}

bool LedDeviceSedu::init(const QJsonObject &deviceConfig)
{
	// Initialise sub-class
	if ( !ProviderRs232::init(deviceConfig) )
	{
		return false;
	}
	
	std::vector<FrameSpec> frameSpecs{{0xA1, 256}, {0xA2, 512}, {0xB0, 768}, {0xB1, 1536}, {0xB2, 3072} };

	for (const FrameSpec& frameSpec : frameSpecs)
	{
		if ((unsigned)_ledRGBCount <= frameSpec.size)
		{
			_ledBuffer.clear();
			_ledBuffer.fill(0x00, frameSpec.size + 3);
			_ledBuffer[0] = 0x5A;
			_ledBuffer[1] = frameSpec.id;
			_ledBuffer.back() = 0xA5;
			break;
		}
	}

	if (_ledBuffer.empty())
	{
		QString errortext = "More rgb-channels required then available";
		this->setInError(errortext);
		return false;
	}

	return true;
}

int LedDeviceSedu::write(const QVector<ColorRgb> &ledValues)
{
	memcpy(_ledBuffer.data()+2, ledValues.data(), ledValues.size() * sizeof(ColorRgb));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
