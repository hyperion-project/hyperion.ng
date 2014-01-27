
// STL includes
#include <cstdint>
#include <vector>
#include <iostream>

#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <sys/ioctl.h>

std::vector<uint8_t> encode(const std::vector<uint8_t> & data);
uint8_t encode(const bool bit1, const bool bit2, const bool bit3);

void printClockSignal(const std::vector<uint8_t> & signal)
{
	bool prevBit = true;
	bool nextBit = true;

	for (uint8_t byte : signal)
	{

		for (int i=-1; i<9; ++i)
		{
			if (i == -1) // Start bit
				nextBit = true;
			else if (i == 8) // Stop bit
				nextBit = false;
			else
				nextBit = ~byte & (1 << i);

			if (!prevBit && nextBit)
			{
				std::cout << ' ';
			}

			if (nextBit)
				std::cout << '1';
			else
				std::cout << '0';

			prevBit = nextBit;
		}
	}
}

int main()
{
	const std::vector<uint8_t> white{0xff,0xff,0xff, 0xff,0xff,0xff, 0xff,0xff,0xff};
	const std::vector<uint8_t> green{0xff, 0x00, 0x00};
	const std::vector<uint8_t> red  {0x00, 0xff, 0x00};
	const std::vector<uint8_t> blue {0x00, 0x00, 0xff};
	const std::vector<uint8_t> cyan {0xff, 0x00, 0xff};
	const std::vector<uint8_t> mix  {0x55, 0x55, 0x55};
	const std::vector<uint8_t> black{0x00, 0x00, 0x00};
	const std::vector<uint8_t> gray{0x01, 0x01, 0x01};

//	printClockSignal(encode(mix));std::cout << std::endl;

	//OPEN THE UART
//	int uart0_filestream = open("/dev/ttyAMA0", O_WRONLY | O_NOCTTY | O_NDELAY);
	int uart0_filestream = open("/dev/ttyUSB0", O_WRONLY | O_NOCTTY | O_NDELAY);
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
		return -1;
	}

	// Configure the port
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B2500000 | CS8 | CLOCAL;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);

	{
		getchar();
		const std::vector<uint8_t> encGreenData = encode(green);
		const std::vector<uint8_t> encBlueData = encode(blue);
		const std::vector<uint8_t> encRedData = encode(red);
		const std::vector<uint8_t> encGrayData = encode(gray);
		const std::vector<uint8_t> encBlackData = encode(black);

		//std::cout << "Writing GREEN ("; printClockSignal(encode(green)); std::cout << ")" << std::endl;
//		const std::vector<uint8_t> garbage {0x0f};
//		write(uart0_filestream, garbage.data(), garbage.size());
//		write(uart0_filestream, encGreenData.data(), encGreenData.size());
//		write(uart0_filestream, encRedData.data(), encRedData.size());
//		write(uart0_filestream, encBlueData.data(), encBlueData.size());
//		write(uart0_filestream, encGrayData.data(), encGrayData.size());
//		write(uart0_filestream, encBlackData.data(), encBlackData.size());
//	}
//	{
//		getchar();
		const std::vector<uint8_t> encData = encode(white);
		std::cout << "Writing WHITE ("; printClockSignal(encode(white)); std::cout << ")" << std::endl;
//		const std::vector<uint8_t> garbage {0x0f};
//		write(uart0_filestream, garbage.data(), garbage.size());
		write(uart0_filestream, encData.data(), encData.size());
		write(uart0_filestream, encData.data(), encData.size());
		write(uart0_filestream, encData.data(), encData.size());
	}
	{
		getchar();
		const std::vector<uint8_t> encData = encode(green);
		std::cout << "Writing GREEN ("; printClockSignal(encode(green)); std::cout << ")" << std::endl;
		write(uart0_filestream, encData.data(), encData.size());
	}
	{
		getchar();
		const std::vector<uint8_t> encData = encode(red);
		std::cout << "Writing RED ("; printClockSignal(encode(red)); std::cout << ")" << std::endl;
		write(uart0_filestream, encData.data(), encData.size());
	}
	{
		getchar();
		const std::vector<uint8_t> encData = encode(blue);
		std::cout << "Writing BLUE ("; printClockSignal(encode(blue)); std::cout << ")" << std::endl;
		write(uart0_filestream, encData.data(), encData.size());
	}
	{
		getchar();
		const std::vector<uint8_t> encData = encode(cyan);
		std::cout << "Writing CYAN? ("; printClockSignal(encode(cyan)); std::cout << ")" << std::endl;
		write(uart0_filestream, encData.data(), encData.size());
	}
	{
		getchar();
		const std::vector<uint8_t> encData = encode(mix);
		std::cout << "Writing MIX ("; printClockSignal(encode(mix)); std::cout << ")" << std::endl;
		write(uart0_filestream, encData.data(), encData.size());
	}
	{
		getchar();
		const std::vector<uint8_t> encData = encode(black);
		std::cout << "Writing BLACK ("; printClockSignal(encode(black)); std::cout << ")" << std::endl;
		write(uart0_filestream, encData.data(), encData.size());
		write(uart0_filestream, encData.data(), encData.size());
		write(uart0_filestream, encData.data(), encData.size());
		write(uart0_filestream, encData.data(), encData.size());
	}

	close(uart0_filestream);

	return 0;
}

std::vector<uint8_t> encode(const std::vector<uint8_t> & data)
{
	std::vector<uint8_t> result;
	for (size_t iByte=0; iByte<data.size(); iByte+=3)
	{
		const uint8_t byte1 = data[iByte];
		const uint8_t byte2 = data[iByte+1];
		const uint8_t byte3 = data[iByte+2];

		result.push_back(encode(byte1 & 0x80, byte1 & 0x40, byte1 & 0x20));
		result.push_back(encode(byte1 & 0x10, byte1 & 0x08, byte1 & 0x04));
		result.push_back(encode(byte1 & 0x02, byte1 & 0x01, byte2 & 0x80));
		result.push_back(encode(byte2 & 0x40, byte2 & 0x20, byte2 & 0x10));
		result.push_back(encode(byte2 & 0x08, byte2 & 0x04, byte2 & 0x02));
		result.push_back(encode(byte2 & 0x01, byte3 & 0x80, byte3 & 0x40));
		result.push_back(encode(byte3 & 0x20, byte3 & 0x10, byte3 & 0x08));
		result.push_back(encode(byte3 & 0x04, byte3 & 0x02, byte3 & 0x01));
	}
	return result;
}

uint8_t encode(const bool bit1, const bool bit2, const bool bit3)
{
	uint8_t result = 0x44; // 0100 0100

	if (bit1)
		result |= 0x01; // 0000 0001

	if (bit2)
		result |= 0x18; // 0001 1000

	if (bit3)
		result |= 0x80; // 1000 0000

	return ~result;
}
