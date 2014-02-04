
// STL includes
#include <cstring>
#include <cstdio>
#include <iostream>

// Linux includes
#include <fcntl.h>
#include <sys/ioctl.h>

// hyperion local includes
#include "LedDeviceSedu.h"

struct FrameSpec
{
	uint8_t id;
	size_t size;
};

LedDeviceSedu::LedDeviceSedu(const std::string& outputDevice, const unsigned baudrate) :
	LedRs232Device(outputDevice, baudrate),
	_ledBuffer(0)
{
	// empty
}

int LedDeviceSedu::write(const std::vector<ColorRgb> &ledValues)
{
	if (_ledBuffer.size() == 0)
	{
		std::vector<FrameSpec> frameSpecs{{0xA1, 256}, {0xA2, 512}, {0xB0, 768}, {0xB1, 1536}, {0xB2, 3072} };

		const unsigned reqColorChannels = ledValues.size() * sizeof(ColorRgb);

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
			std::cout << "More rgb-channels required then available" << std::endl;
			return -1;
		}
	}

	memcpy(_ledBuffer.data()+2, ledValues.data(), ledValues.size() * sizeof(ColorRgb));
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}

int LedDeviceSedu::switchOff()
{
	memset(_ledBuffer.data()+2, 0, _ledBuffer.size()-3);
	return writeBytes(_ledBuffer.size(), _ledBuffer.data());
}
