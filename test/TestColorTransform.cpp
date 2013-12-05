// STL includes
#include <iostream>
#include <cmath>

// Utils includes
#include <utils/RgbChannelTransform.h>

int main()
{
	{
		std::cout << "Testing linear transform" << std::endl;
		RgbChannelTransform t;
		for (int i = 0; i < 256; ++i)
		{
			uint8_t input = i;
			uint8_t output = t.transform(input);
			uint8_t expected = input;

			if (output != expected)
			{
				std::cerr << "ERROR: input (" << (int)input << ") => output (" << (int)output << ") : expected (" << (int) expected << ")" << std::endl;
				return 1;
			}
			else
			{
				std::cerr << "OK: input (" << (int)input << ") => output (" << (int)output << ")" << std::endl;
			}
		}
	}

	{
		std::cout << "Testing threshold" << std::endl;
		RgbChannelTransform t(.10, 1.0, 0.0, 1.0);
		for (int i = 0; i < 256; ++i)
		{
			uint8_t input = i;
			uint8_t output = t.transform(input);
			uint8_t expected = ((i/255.0) < t.getThreshold() ? 0 : output);

			if (output != expected)
			{
				std::cerr << "ERROR: input (" << (int)input << ") => output (" << (int)output << ") : expected (" << (int) expected << ")" << std::endl;
				return 1;
			}
			else
			{
				std::cerr << "OK: input (" << (int)input << ") => output (" << (int)output << ")" << std::endl;
			}
		}
	}

	{
		std::cout << "Testing blacklevel and whitelevel" << std::endl;
		RgbChannelTransform t(0, 1.0, 0.2, 0.8);
		for (int i = 0; i < 256; ++i)
		{
			uint8_t input = i;
			uint8_t output = t.transform(input);
			uint8_t expected = (uint8_t)(input * (t.getWhitelevel()-t.getBlacklevel()) + 255 * t.getBlacklevel());

			if (output != expected)
			{
				std::cerr << "ERROR: input (" << (int)input << ") => output (" << (int)output << ") : expected (" << (int) expected << ")" << std::endl;
				return 1;
			}
			else
			{
				std::cerr << "OK: input (" << (int)input << ") => output (" << (int)output << ")" << std::endl;
			}
		}
	}

	{
		std::cout << "Testing gamma" << std::endl;
		RgbChannelTransform t(0, 2.0, 0.0, 1.0);
		for (int i = 0; i < 256; ++i)
		{
			uint8_t input = i;
			uint8_t output = t.transform(input);
			uint8_t expected = (uint8_t)(255 * std::pow(i / 255.0, 2));

			if (output != expected)
			{
				std::cerr << "ERROR: input (" << (int)input << ") => output (" << (int)output << ") : expected (" << (int) expected << ")" << std::endl;
				return 1;
			}
			else
			{
				std::cerr << "OK: input (" << (int)input << ") => output (" << (int)output << ")" << std::endl;
			}
		}
	}

	return 0;
}
