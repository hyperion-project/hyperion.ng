#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/RgbToRgbw.h>
#include <utils/Logger.h>

namespace RGBW {

WhiteAlgorithm stringToWhiteAlgorithm(const QString& str)
{
	if (str == "subtract_minimum")         return WhiteAlgorithm::SUBTRACT_MINIMUM;
	if (str == "sub_min_warm_adjust")      return WhiteAlgorithm::SUB_MIN_WARM_ADJUST;
	if (str == "sub_min_cool_adjust")      return WhiteAlgorithm::SUB_MIN_COOL_ADJUST;
	if (str.isEmpty() || str == "white_off") return WhiteAlgorithm::WHITE_OFF;
	return WhiteAlgorithm::INVALID;
}

void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, WhiteAlgorithm algorithm)
{
	switch (algorithm)
	{
		case WhiteAlgorithm::SUBTRACT_MINIMUM:
		{
			output->white = qMin(qMin(input.red, input.green), input.blue);
			output->red   = input.red   - output->white;
			output->green = input.green - output->white;
			output->blue  = input.blue  - output->white;
			break;
		}

		case WhiteAlgorithm::SUB_MIN_WARM_ADJUST:
		{
			// http://forum.garagecube.com/viewtopic.php?t=10178
			// warm white
			const float F1(0.274);
			const float F2(0.454);
			const float F3(2.333);

			output->white = qMin(input.red*F1,qMin(input.green*F2,input.blue*F3));
			output->red   = input.red   - output->white/F1;
			output->green = input.green - output->white/F2;
			output->blue  = input.blue  - output->white/F3;
			break;
		}

		case WhiteAlgorithm::SUB_MIN_COOL_ADJUST:
		{
			// http://forum.garagecube.com/viewtopic.php?t=10178
			// cold white
			const float F1(0.299);
			const float F2(0.587);
			const float F3(0.114);

			output->white = qMin(input.red*F1,qMin(input.green*F2,input.blue*F3));
			output->red   = input.red   - output->white/F1;
			output->green = input.green - output->white/F2;
			output->blue  = input.blue  - output->white/F3;
			break;
		}

		case WhiteAlgorithm::WHITE_OFF:
		{
			output->red = input.red;
			output->green = input.green;
			output->blue = input.blue;
			output->white = 0;
			break;
		}
		default:
			break;
	}
}

};
