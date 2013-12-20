
// Hyperion includes
#include "../libsrc/leddevice/LedRs232Device.h"


class TestDevice : public LedRs232Device
{
public:
	TestDevice() :
		LedRs232Device("/dev/ttyAMA0", 2000000)
	{
		// empty
	}

	int write(const std::vector<ColorRgb> &ledValues) {}
	int switchOff() {};

	void writeTestSequence()
	{
		uint8_t data = 'T';

		writeBytes(1, &data);
	}
};

int main()
{
	TestDevice device;
	device.writeTestSequence();

	return 0;
}
