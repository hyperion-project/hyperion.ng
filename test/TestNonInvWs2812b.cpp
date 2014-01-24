
// STL includes
#include <cstdint>
#include <vector>
#include <iostream>

#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <sys/ioctl.h>

std::vector<uint8_t> encode(const std::vector<uint8_t> & data);
void split(const uint8_t byte, uint8_t & out1, uint8_t & out2);
uint8_t encode(const bool bit1, const bool bit2, const bool bit3);

void print(uint8_t byte)
{
	for (int i=0; i<8; ++i)
	{
		if (byte & (1 << i))
		{
			std::cout << '1';
		}
		else
		{
			std::cout << '0';
		}
	}
}

void printClockSignal(const std::vector<uint8_t> & signal)
{
	bool prevBit = true;
	bool nextBit = true;

	for (uint8_t byte : signal)
	{

		for (int i=-1; i<9; ++i)
		{
			if (i == -1) // Start bit
				nextBit = false;
			else if (i == 8) // Stop bit
				nextBit = true;
			else
				nextBit = byte & (1 << i);

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
	const std::vector<uint8_t> data(9, 0xff);
	std::vector<uint8_t> encData = encode(data);

	for (uint8_t encByte : encData)
	{
		std::cout << "0 ";
		print(encByte);
		std::cout << " 1";
	}
	std::cout << std::endl;
	printClockSignal(encData);
	std::cout << std::endl;

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
	options.c_cflag = B4000000 | CS8 | CLOCAL;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);

	char c = getchar();

	const int breakLength_ms = 1;

	encData = std::vector<uint8_t>(128, 0x10);

	write(uart0_filestream, encData.data(), encData.size());

	tcsendbreak(uart0_filestream, breakLength_ms);

	//tcdrain(uart0_filestream);
//	res = write(uart0_filestream, encData.data(), encData.size());
//	(void)res;

	close(uart0_filestream);

	return 0;
}

std::vector<uint8_t> encode(const std::vector<uint8_t> & data)
{
	std::vector<uint8_t> result;

	uint8_t previousByte = 0x00;
	uint8_t nextByte     = 0x00;
	for (unsigned iData=0; iData<data.size(); iData+=3)
	{
		const uint8_t byte1 = data[iData];
		const uint8_t byte2 = data[iData+1];
		const uint8_t byte3 = data[iData+2];

		uint8_t encByte;
		encByte = encode(byte1 & 0x80, byte1 & 0x40, byte1 & 0x20);
		std::cout << "Encoded byte 1: "; print(encByte); std::cout << std::endl;
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;

		encByte = encode(byte1 & 0x10, byte1 & 0x08, byte1 & 0x04);
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;

		encByte = encode(byte1 & 0x02, byte1 & 0x01, byte2 & 0x80);
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;

		encByte = encode(byte2 & 0x40, byte2 & 0x20, byte2 & 0x10);
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;

		encByte = encode(byte2 & 0x08, byte2 & 0x04, byte2 & 0x02);
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;

		encByte = encode(byte2 & 0x01, byte3 & 0x80, byte3 & 0x40);
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;

		encByte = encode(byte3 & 0x20, byte3 & 0x10, byte3 & 0x08);
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;

		encByte = encode(byte3 & 0x04, byte3 & 0x02, byte3 & 0x01);
		split(encByte, previousByte, nextByte);
		result.push_back(previousByte);
		previousByte = nextByte;
	}

	result.push_back(previousByte);


	return result;
}

void split(const uint8_t byte, uint8_t & out1, uint8_t & out2)
{
	out1 |= (byte & 0x0F) << 4;
	out2 = (byte & 0xF0) >> 4;
}

uint8_t encode(const bool bit1, const bool bit2, const bool bit3)
{
	if (bit2)
	{
		uint8_t result = 0x19; // 0--1 01 10-1
		if (bit1) result |=  0x02;
//		else      result &= ~0x02;

		if (bit3) result |=  0x60;
//		else      result &= ~0x60;

		return result;
	}
	else
	{
		uint8_t result = 0x21;// 0x21 (0-10 01 0--1)
		if (bit1) result |=  0x06;
//		else      result &= ~0x06;

		if (bit3) result |=  0x40;
//		else      result &= ~0x40;

		return result;
	}
}
