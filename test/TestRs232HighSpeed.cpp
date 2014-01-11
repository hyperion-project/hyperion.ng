
// STL includes
#include <iostream>
#include <random>

// Serialport includes
#include <serial/serial.h>

int testSerialPortLib();
int testHyperionDevice(int argc, char** argv);
int testWs2812bDevice();

int main(int argc, char** argv)
{
//	if (argc == 1)
//	{
//		return testSerialPortLib();
//	}
//	else
//	{
//		return testHyperionDevice(argc, argv);
//	}
		return testWs2812bDevice();
}

int testSerialPortLib()
{
	serial::Serial rs232Port("/dev/ttyAMA0", 4000000);

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(1,2);

	std::vector<uint8_t> data;
	for (int i=0; i<9; ++i)
	{
		int coinFlip = distribution(generator);
		if (coinFlip == 1)
		{
			data.push_back(0xCE);
			data.push_back(0xCE);
			data.push_back(0xCE);
			data.push_back(0xCE);
		}
		else
		{
			data.push_back(0x8C);
			data.push_back(0x8C);
			data.push_back(0x8C);
			data.push_back(0x8C);
		}
	}
	std::cout << "Type 'c' to continue, 'q' or 'x' to quit: ";
	while (true)
	{
		char c = getchar();
		if (c == 'q' || c == 'x')
		{
			break;
		}
		if (c != 'c')
		{
			continue;
		}

		rs232Port.flushOutput();
		rs232Port.write(data);
		rs232Port.flush();

		data.clear();
		for (int i=0; i<9; ++i)
		{
			int coinFlip = distribution(generator);
			if (coinFlip == 1)
			{
				data.push_back(0xCE);
				data.push_back(0xCE);
				data.push_back(0xCE);
				data.push_back(0xCE);
			}
			else
			{
				data.push_back(0x8C);
				data.push_back(0x8C);
				data.push_back(0x8C);
				data.push_back(0x8C);
			}
		}
	}

	try
	{

		rs232Port.close();
	}
	catch (const std::runtime_error& excp)
	{
		std::cout << "Caught exception: " << excp.what() << std::endl;
		return -1;
	}

	return 0;
}

#include "../libsrc/leddevice/LedRs232Device.h"

class TestDevice : public LedRs232Device
{
public:
	TestDevice() :
		LedRs232Device("/dev/ttyAMA0", 4000000)
	{
		open();
	}

	int write(const std::vector<ColorRgb> &ledValues)
	{
		std::vector<uint8_t> bytes(ledValues.size() * 3 * 4);

		uint8_t * bytePtr = bytes.data();
		for (ColorRgb color : ledValues)
		{
			byte2Signal(color.green, bytePtr);
			bytePtr += 4;
			byte2Signal(color.red, bytePtr);
			bytePtr += 4;
			byte2Signal(color.blue, bytePtr);
			bytePtr += 4;
		}

		writeBytes(bytes.size(), bytes.data());

		return 0;
	}

	int switchOff() { return 0; };

	void writeTestSequence(const std::vector<uint8_t> & data)
	{
		writeBytes(data.size(), data.data());
	}

	void byte2Signal(const uint8_t byte, uint8_t * output)
	{
		output[0] = bits2Signal(byte & 0x80, byte & 0x40);
		output[1] = bits2Signal(byte & 0x20, byte & 0x10);
		output[2] = bits2Signal(byte & 0x08, byte & 0x04);
		output[3] = bits2Signal(byte & 0x02, byte & 0x01);
	}

	uint8_t bits2Signal(const bool bit1, const bool bit2)
	{
		if (bit1)
		{
			if (bit2)
			{
				return 0x8C;
			}
			else
			{
				return 0xCC;
			}
		}
		else
		{
			if (bit2)
			{
				return 0x8E;
			}
			else
			{
				return 0xCE;
			}
		}

		return 0x00;
	}
};

int testHyperionDevice(int argc, char** argv)
{
	TestDevice rs232Device;

	if (argc > 1 && strncmp(argv[1], "off", 3) == 0)
	{
		rs232Device.write(std::vector<ColorRgb>(150, {0, 0, 0}));
		return 0;
	}


	int loopCnt = 0;

	std::cout << "Type 'c' to continue, 'q' or 'x' to quit: ";
	while (true)
	{
		char c = getchar();
		if (c == 'q' || c == 'x')
		{
			break;
		}
		if (c != 'c')
		{
			continue;
		}

		rs232Device.write(std::vector<ColorRgb>(loopCnt, {255, 255, 255}));

		++loopCnt;
	}

	rs232Device.write(std::vector<ColorRgb>(150, {0, 0, 0}));

	return 0;
}

#include "../libsrc/leddevice/LedDeviceWs2812b.h"

#include <unistd.h>

int testWs2812bDevice()
{
	LedDeviceWs2812b device;
	device.open();

	std::cout << "Type 'c' to continue, 'q' or 'x' to quit: ";
	int loopCnt = 0;
	while (true)
	{
//		char c = getchar();
//		if (c == 'q' || c == 'x')
//		{
//			break;
//		}
//		if (c != 'c')
//		{
//			continue;
//		}

		if (loopCnt%4 == 0)
			device.write(std::vector<ColorRgb>(25, {255, 0, 0}));
		else if (loopCnt%4 == 1)
			device.write(std::vector<ColorRgb>(25, {0, 255, 0}));
		else if (loopCnt%4 == 2)
			device.write(std::vector<ColorRgb>(25, {0, 0, 255}));
		else if (loopCnt%4 == 3)
			device.write(std::vector<ColorRgb>(25, {17, 188, 66}));

		++loopCnt;

		usleep(200000);
		if (loopCnt > 200)
		{
			break;
		}
	}

	device.write(std::vector<ColorRgb>(150, {0, 0, 0}));
	device.switchOff();

	return 0;
}
