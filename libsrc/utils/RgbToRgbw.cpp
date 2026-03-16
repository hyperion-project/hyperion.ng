#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/RgbToRgbw.h>
#include <utils/Logger.h>

#define ROUND_DIVIDE(number, denom) (((number) + (denom) / 2) / (denom))

namespace RGBW {

WhiteAlgorithm stringToWhiteAlgorithm(const QString& str)
{
	if (str == "subtract_minimum")
	{
		return WhiteAlgorithm::SUBTRACT_MINIMUM;
	}
	if (str == "sub_min_warm_adjust")
	{
		return WhiteAlgorithm::SUB_MIN_WARM_ADJUST;
	}
	if (str == "sub_min_cool_adjust")
	{
		return WhiteAlgorithm::SUB_MIN_COOL_ADJUST;
	}
	if (str == "sub_ktemp_white")
	{
		return WhiteAlgorithm::SUB_KTEMP_WHITE;
	}
    if (str == "cold_white")
    {
        return WhiteAlgorithm::COLD_WHITE;
    }
    if (str == "neutral_white")
    {
        return WhiteAlgorithm::NEUTRAL_WHITE;
    }
    if (str == "auto")
    {
        return WhiteAlgorithm::AUTO;
    }
    if (str == "auto_max")
    {
        return WhiteAlgorithm::AUTO_MAX;
    }
    if (str == "auto_accurate")
    {
        return WhiteAlgorithm::AUTO_ACCURATE;
    }
    if (str.isEmpty() || str == "white_off")
	{
		return WhiteAlgorithm::WHITE_OFF;
	}
	return WhiteAlgorithm::INVALID;
}

void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, WhiteAlgorithm algorithm, u_int16_t whiteTemp)
{
	switch (algorithm)
	{
		case WhiteAlgorithm::SUBTRACT_MINIMUM:
		{
			output->white = static_cast<uint8_t>(qMin(qMin(input.red, input.green), input.blue));
			output->red   = input.red   - output->white;
			output->green = input.green - output->white;
			output->blue  = input.blue  - output->white;
			break;
		}

		case WhiteAlgorithm::SUB_MIN_WARM_ADJUST:
		{
			// http://forum.garagecube.com/viewtopic.php?t=10178
			// warm white
			const double F1(0.274);
			const double F2(0.454);
			const double F3(2.333);

			output->white = static_cast<uint8_t>(qMin(input.red*F1,qMin(input.green*F2,input.blue*F3)));
			output->red   = input.red   - static_cast<uint8_t>(output->white/F1);
			output->green = input.green - static_cast<uint8_t>(output->white/F2);
			output->blue  = input.blue  - static_cast<uint8_t>(output->white/F3);
			break;
		}

		case WhiteAlgorithm::SUB_MIN_COOL_ADJUST:
		{
			// http://forum.garagecube.com/viewtopic.php?t=10178
			// cold white
			const double F1(0.299);
			const double F2(0.587);
			const double F3(0.114);

			output->white = static_cast<uint8_t>(qMin(input.red*F1,qMin(input.green*F2,input.blue*F3)));
			output->red   = input.red   - static_cast<uint8_t>(output->white/F1);
			output->green = input.green - static_cast<uint8_t>(output->white/F2);
			output->blue  = input.blue  - static_cast<uint8_t>(output->white/F3);
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

        case WhiteAlgorithm::AUTO_MAX:
        {
            output->red = input.red;
            output->green = input.green;
            output->blue = input.blue;
            output->white = input.red > input.green ? (input.red > input.blue ? input.red : input.blue) : (input.green > input.blue ? input.green : input.blue);
            break;
        }

        case WhiteAlgorithm::AUTO_ACCURATE:
        {
            output->white = input.red < input.green ? (input.red < input.blue ? input.red : input.blue) : (input.green < input.blue ? input.green : input.blue);
            output->red = input.red - output->white;
            output->green = input.green - output->white;
            output->blue = input.blue - output->white;
            break;
        }

        case WhiteAlgorithm::AUTO:
        {

            output->red = input.red;
            output->green = input.green;
            output->blue = input.blue;
            output->white = input.red < input.green ? (input.red < input.blue ? input.red : input.blue) : (input.green < input.blue ? input.green : input.blue);
            break;
        }
		case WhiteAlgorithm::SUB_KTEMP_WHITE:
		{
			ColorRgb white = ColorRgb::white(whiteTemp);

			// calculating per channel ratio between color we convert and our custom temperature white. Always in range [0,1]
			float fRatioRed = static_cast<float>(input.red) / static_cast<float>(white.red);
			float fRatioGreen = static_cast<float>(input.green) / static_cast<float>(white.green);
			float fRatioBlue = static_cast<float>(input.blue)  / static_cast<float>(white.blue);

			// smallest ratio
			float fRatio = qMin(fRatioRed, qMin(fRatioGreen, fRatioBlue));

			// typically brigtness of x value on white channel is lower than (x,x,x) RGB. Depends on LED. But 1/3 is a reasonable guess
			float fWhiteToRGBOutputRatio = 255.0f/static_cast<float>(white.red + white.green + white.blue);
			// Make the color with the smallest white ratio to be the output white value
			uint8_t uScale;
			if (fRatio == fRatioRed) {
				uScale = input.red;
			} else if (fRatio == fRatioGreen) {
				uScale = input.green;
			}else {
				uScale =  input.blue;
			}

			float fUpscale = qBound(1.0f, 255.0f / static_cast<float>(uScale), 3.0f);

			// Calculate the output red, green and blue values, taking into account the white color temperature.
			output->red = static_cast<u_int8_t>(qBound(0.0f, round(input.red - uScale * (white.red / 255.0f)*fWhiteToRGBOutputRatio), 255.0f));
			output->green = static_cast<u_int8_t>(qBound(0.0f, round(input.green - uScale * (white.green / 255.0f)*fWhiteToRGBOutputRatio), 255.0f));
			output->blue = static_cast<u_int8_t>(qBound(0.0f, round(input.blue - uScale * (white.blue / 255.0f)*fWhiteToRGBOutputRatio), 255.0f));
			output->white = static_cast<u_int8_t>(qBound(0.0f, round(uScale), 255.0f));

			break;
		}
        case WhiteAlgorithm::NEUTRAL_WHITE:
        case WhiteAlgorithm::COLD_WHITE:
        {
            //cold white config
            uint8_t gain = 0xFF;
            uint8_t red = 0xA0;
            uint8_t green = 0xA0;
            uint8_t blue = 0xA0;

            if (algorithm == WhiteAlgorithm::NEUTRAL_WHITE) {
                gain = 0xFF;
                red = 0xB0;
                green = 0xB0;
                blue = 0x70;
            }

            uint8_t _r = qMin((uint32_t)(ROUND_DIVIDE(red * input.red,  0xFF)), (uint32_t)0xFF);
            uint8_t _g = qMin((uint32_t)(ROUND_DIVIDE(green * input.green,  0xFF)), (uint32_t)0xFF);
            uint8_t _b = qMin((uint32_t)(ROUND_DIVIDE(blue * input.blue,  0xFF)), (uint32_t)0xFF);

            output->white = qMin(_r, qMin(_g, _b));
            output->red = input.red - _r;
            output->green = input.green - _g;
            output->blue = input.blue - _b;

            uint8_t _w = qMin((uint32_t)(ROUND_DIVIDE(gain * output->white,  0xFF)), (uint32_t)0xFF);
            output->white = _w;
            break;
        }
		default:
			break;
	}
}

};
