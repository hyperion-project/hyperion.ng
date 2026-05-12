#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/RgbToRgbw.h>
#include <utils/KelvinToRgb.h>

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

void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, WhiteAlgorithm algorithm, uint16_t whiteTemp)
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
			ColorRgb white = getRgbFromTemperature(whiteTemp);
			const auto sumW = static_cast<double>(white.red + white.green + white.blue);

			// Max fraction of white chromaticity we can subtract per channel without going negative
			auto safeRatio = [](double num, double denom)
			{
				return (denom > 0.0) ? num / denom : qInf();
			};
			double fRatio = qMin(safeRatio(input.red, white.red),
							qMin(safeRatio(input.green, white.green),
								 safeRatio(input.blue, white.blue)));

			// White LED efficiency model: driving w produces w*wc/sumW on each channel
			// (equivalent to "1/3 efficiency" for pure white where sumW = 3*255)
			// Cap at 255, then back-calculate the actual ratio used for RGB subtraction
			const double fWhiteDrive = qBound(0.0, fRatio * sumW, 255.0);
			const double fActualRatio = fWhiteDrive / sumW;

			output->white = static_cast<uint8_t>(qRound(fWhiteDrive));
			output->red = static_cast<uint8_t>(qBound(0, qRound(input.red - fActualRatio * white.red), 255));
			output->green = static_cast<uint8_t>(qBound(0, qRound(input.green - fActualRatio * white.green), 255));
			output->blue = static_cast<uint8_t>(qBound(0, qRound(input.blue - fActualRatio * white.blue), 255));

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
