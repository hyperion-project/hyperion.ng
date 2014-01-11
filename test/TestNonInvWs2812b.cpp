
// STL includes
#include <cstdint>
#include <vector>
#include <iostream>

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

int main()
{
	std::vector<uint8_t> data(3, 0x55);

	std::vector<uint8_t> encData = encode(data);

	for (uint8_t encByte : encData)
	{
		std::cout << "0 ";
		print(encByte);
		std::cout << " 1";
	}
	std::cout << std::endl;


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

	return result;
}

void split(const uint8_t byte, uint8_t & out1, uint8_t & out2)
{
	print(byte); std::cout << " => ";
	print(out2); std::cout << " => ";
	out1 &= ~0x0F;
	out1 |= (byte & 0x0F) << 4;
//	out2 &= ~0xF0;
	print(out2); std::cout << " => ";
	out2 = (byte & 0xF0) >> 4;
	print(out2); std::cout << std::endl;
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
