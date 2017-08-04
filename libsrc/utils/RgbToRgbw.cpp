#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/RgbToRgbw.h>
#include <utils/Logger.h>

namespace RGBW {

WhiteAlgorithm stringToWhiteAlgorithm(QString str)
{
	if (str == "subtract_minimum")         return SUBTRACT_MINIMUM;
	if (str == "sub_min_warm_adjust")      return SUB_MIN_WARM_ADJUST;
	if (str.isEmpty() || str == "white_off") return WHITE_OFF;
	return INVALID;
}

void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, const WhiteAlgorithm algorithm)
{
	switch (algorithm)
	{
		case SUBTRACT_MINIMUM:
		{
			output->white = qMin(qMin(input.red, input.green), input.blue);
			output->red   = input.red   - output->white;
			output->green = input.green - output->white;
			output->blue  = input.blue  - output->white;
			break;
		}
		
		case SUB_MIN_WARM_ADJUST:
		{
			Error(Logger::getInstance("RGBtoRGBW"), "white algorithm 'sub_min_warm_adjust' is not implemented yet." );
			break;
		}

		case WHITE_OFF:
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
