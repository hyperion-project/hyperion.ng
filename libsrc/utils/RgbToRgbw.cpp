#include <utils/ColorRgb.h>
#include <utils/ColorRgbw.h>
#include <utils/Logger.h>

void Rgb_to_Rgbw(ColorRgb input, ColorRgbw * output, std::string _whiteAlgorithm) {
	if (_whiteAlgorithm == "subtract_minimum") {
		output->white = std::min(input.red, input.green);
		output->white = std::min(output->white, input.blue);
		output->red = input.red - output->white;
		output->green = input.green - output->white;
		output->blue = input.blue - output->white;
	}
	else if (_whiteAlgorithm == "sub_min_warm_adjust") {
	}
	else if ( (_whiteAlgorithm == "") || (_whiteAlgorithm == "white_off") ) {
		output->red = input.red;
		output->green = input.green;
		output->blue = input.blue;
		output->white = 0;
	}
	else {
		Error(Logger::getInstance("RGBtoRGBW"), "unknown whiteAlgorithm %s", _whiteAlgorithm.c_str());
	}
}

