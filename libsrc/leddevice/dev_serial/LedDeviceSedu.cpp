#include "LedDeviceSedu.h"

struct FrameSpec
{
	uint8_t id;
	size_t size;
};

LedDeviceSedu::LedDeviceSedu(const QJsonObject &deviceConfig)
	: ProviderRs232()
{
	_deviceReady = init(deviceConfig);
}

LedDevice* LedDeviceSedu::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceSedu(deviceConfig);
}

bool LedDeviceSedu::init(const QJsonObject &deviceConfig)
{
	ProviderRs232::init(deviceConfig);

	std::vector<FrameSpec> frameSpecs{{0xA1, 256}, {0xA2, 512}, {0xB0, 768}, {0xB1, 1536}, {0xB2, 3072} };

	for (const FrameSpec& frameSpec : frameSpecs)
	{
		if ((unsigned)_ledRGBCount <= frameSpec.size)
		{
			_ledBuffer.clear();
			_ledBuffer.resize(frameSpec.size + 3, 0);
			_ledBuffer[0] = 0x5A;
			_ledBuffer[1] = frameSpec.id;
			_ledBuffer.back() = 0xA5;
			break;
		}
	}

	if (_ledBuffer.size() == 0)
	{
		Warning(_log, "More rgb-channels required then available");
		return false;
	}

	return true;
}

int LedDeviceSedu::write(const std::vector<ColorRgb> &ledValues)
{
	memcpy(_ledBuffer.data()+2, ledValues.data(), ledValues.size() * sizeof(ColorRgb));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
