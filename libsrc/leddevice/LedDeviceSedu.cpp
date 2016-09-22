#include "LedDeviceSedu.h"

struct FrameSpec
{
	uint8_t id;
	size_t size;
};

LedDeviceSedu::LedDeviceSedu(const Json::Value &deviceConfig)
	: ProviderRs232(deviceConfig)
{
}

LedDevice* LedDeviceSedu::construct(const Json::Value &deviceConfig)
{
	return new LedDeviceSedu(deviceConfig);
}

int LedDeviceSedu::write(const std::vector<ColorRgb> &ledValues)
{
	if (_ledBuffer.size() == 0)
	{
		std::vector<FrameSpec> frameSpecs{{0xA1, 256}, {0xA2, 512}, {0xB0, 768}, {0xB1, 1536}, {0xB2, 3072} };

		const unsigned reqColorChannels = _ledCount * sizeof(ColorRgb);

		for (const FrameSpec& frameSpec : frameSpecs)
		{
			if (reqColorChannels <= frameSpec.size)
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
			return -1;
		}
	}

	memcpy(_ledBuffer.data()+2, ledValues.data(), ledValues.size() * sizeof(ColorRgb));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
