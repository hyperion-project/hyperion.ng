
// STL includes
#include <vector>
#include <ctime>
#include <cstring>
#include <string>
#include <unistd.h>
#include <iostream>

// Local includes
#include <utils/RgbColor.h>

#include "../libsrc/hyperion/device/LedDeviceWs2801.h"

void setColor(char* colorStr)
{
	RgbColor color = RgbColor::BLACK;
	std::cout << "Switching all leds to: ";
	if (strncmp("red", colorStr, 3) == 0)
	{
		std::cout << "red";
		color = RgbColor::RED;
	}
	else if (strncmp("green", colorStr, 5) == 0)
	{
		std::cout << "green";
		color = RgbColor::GREEN;
	}
	else if (strncmp("blue", colorStr, 5) == 0)
	{
		std::cout << "blue";
		color = RgbColor::BLUE;
	}
	else if (strncmp("cyan", colorStr, 5) == 0)
	{
		std::cout << "cyan";
	}
	else if (strncmp("gray", colorStr, 4) == 0)
	{
		std::cout << "gray";
	}
	else if (strncmp("white", colorStr, 5) == 0)
	{
		std::cout << "white";
		color = RgbColor::WHITE;
	}
	else if (strncmp("black", colorStr, 5) == 0)
	{
		std::cout << "black";
		color = RgbColor::BLACK;
	}
	std::cout << std::endl;

	unsigned ledCnt = 50;
	std::vector<RgbColor> buff(ledCnt, color);

	LedDeviceWs2801 ledDevice("/dev/spidev0.0", 40000);
	ledDevice.open();
	ledDevice.write(buff);
}

bool _running = true;
void doCircle()
{
	RgbColor color_1 = RgbColor::RED;
	RgbColor color_2 = RgbColor::YELLOW;

	unsigned ledCnt = 50;
	std::vector<RgbColor> data(ledCnt, RgbColor::BLACK);

	LedDeviceWs2801 ledDevice("/dev/spidev0.0", 40000);
	ledDevice.open();

	timespec loopTime;
	loopTime.tv_sec  = 0;
	loopTime.tv_nsec = 100000000; // 100 ms

	int curLed_1  = 0;
	int nextLed_1 = 1;

	int curLed_2  = 49;
	int nextLed_2 = 48;


	while (_running)
	{
		data[curLed_1] = RgbColor::BLACK;
		data[curLed_2] = RgbColor::BLACK;

		// Move the current and the next pointer
		curLed_1 = nextLed_1;
		curLed_2 = nextLed_2;
		++nextLed_1;
		--nextLed_2;
		if (nextLed_1 == int(ledCnt))
		{
			nextLed_1 = 0;
		}
		if (nextLed_2 < 0)
		{
			nextLed_2 = 49;
		}

		data[curLed_1] = color_1;

		data[curLed_2] = color_2;

		ledDevice.write(data);

		nanosleep(&loopTime, NULL);
	}

	// Switch the current leds off
	data[curLed_1] = RgbColor::BLACK;
	data[curLed_2] = RgbColor::BLACK;

	ledDevice.write(data);
}

#include <csignal>

void signal_handler(int signum)
{
	_running = false;
}

int main(int argc, char** argv)
{
	if (sizeof(RgbColor) != 3)
	{
		std::cout << "sizeof(RgbColor) = " << sizeof(RgbColor) << std::endl;
		return -1;
	}

	// Install signal handlers to stop loops
	signal(SIGTERM, &signal_handler);
	signal(SIGINT,  &signal_handler);

	if (argc < 2)
	{
		std::cerr << "Missing argument" << std::endl;
		return -1;
	}

	if (strncmp("fixed", argv[1], 5) == 0)
	{
		setColor(argv[2]);
		return 0;
	}
	else if (strncmp("circle", argv[1], 6) == 0)
	{
		doCircle();
	}
	else
	{
		std::cerr << "Unknown option: " << argv[1] << std::endl;
	}

	return 0;
}

