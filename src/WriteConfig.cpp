// STL includes
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

ofstream& indent(ofstream& ofs, unsigned indent)
{
	for (unsigned i=0; i<indent; ++i)
	{
		ofs << '\t';
	}
	return ofs;
}

struct LedFrame
{
	unsigned topLedCnt    = 17;
	unsigned rightLedCnt  = 8;
	unsigned bottomLedCnt = 17;
	unsigned leftLedCnt   = 8;

	unsigned topLeftOffset = 17;

	unsigned borderWidth  = 10;
	unsigned borderHeight = 10;

	unsigned totalLedCnt() const
	{
		return topLeftOffset + rightLedCnt + bottomLedCnt + leftLedCnt;
	}
};

struct Led
{
	unsigned index;
	double minX;
	double maxX;
	double minY;
	double maxY;
};

void determineBoundaries(const LedFrame& ledFrame, const unsigned iLed, double& minX, double& maxX, double& minY, double& maxY)
{
	if (iLed < ledFrame.topLedCnt)
	{
		// TOP of screen
		minX =     iLed * 100.0/ledFrame.topLedCnt;
		maxX = (iLed+1) * 100.0/ledFrame.topLedCnt;

		minY = 100.0 - ledFrame.borderHeight;
		maxY = 100.0;
	}
	else if (iLed < (ledFrame.topLedCnt+ledFrame.rightLedCnt))
	{
		// RIGHT of screen
		minX = 100.0 - ledFrame.borderWidth;
		maxX = 100.0;

		minY = 100.0 - (iLed-15) * 100.0/(ledFrame.rightLedCnt+2);
		maxY = 100.0 - (iLed-16) * 100.0/(ledFrame.rightLedCnt+2);
	}
	else if (iLed < (ledFrame.topLedCnt+ledFrame.rightLedCnt+ledFrame.bottomLedCnt))
	{
		// BOTTOM of screen
		minX = 100.0 - (iLed-24) * 100.0/ledFrame.bottomLedCnt;
		maxX = 100.0 - (iLed-25) * 100.0/ledFrame.bottomLedCnt;

		minY =  0.0;
		maxY = ledFrame.borderHeight;
	}
	else if (iLed < (ledFrame.topLedCnt+ledFrame.rightLedCnt+ledFrame.bottomLedCnt+ledFrame.leftLedCnt))
	{
		// LEFT of screen
		minX =  0.0;
		maxX = ledFrame.borderWidth;

		minY = (iLed-41) * 100.0/(ledFrame.leftLedCnt+2);
		maxY = (iLed-40) * 100.0/(ledFrame.leftLedCnt+2);
	}
	else
	{
		std::cerr << "Requested led index(" << iLed << ") out of bound" << endl;
	}
}

std::vector<Led> createLedMapping(const LedFrame ledFrame)
{
	std::vector<Led> leds;
	for (unsigned iLed=0; iLed<ledFrame.totalLedCnt(); ++iLed)
	{
		Led led;

		led.index = (iLed + ledFrame.topLeftOffset)%ledFrame.totalLedCnt();
		determineBoundaries(ledFrame, iLed, led.minX, led.maxX, led.minY, led.maxY);

		leds.push_back(led);
	}

	std::sort(leds.begin(), leds.end(), [](const Led& lhs, const Led& rhs){ return lhs.index < rhs.index; });

	return leds;
}

int main(int argc, char** argv)
{
	const std::string filename = "hyperion.config.json";

	LedFrame myFrame;
	std::vector<Led> leds = createLedMapping(myFrame);
	ofstream configOfs(filename.c_str());

	configOfs << "// Automatically generated configuration file for 'Hyperion'" << endl;
	configOfs << "// Generation script: " << argv[0] << endl;
	configOfs << endl;
	configOfs << "{" << endl;

	// Write the device section
	indent(configOfs, 1) << R"("device" : )" << endl;
	indent(configOfs, 1) << "{" << endl;
	indent(configOfs, 2) << R"("name"     : "MyPi",)" << endl;
	indent(configOfs, 2) << R"("type"     : "ws2801",)" << endl;
	indent(configOfs, 2) << R"("output"   : "/dev/spidev0.0",)" << endl;
	indent(configOfs, 2) << R"("interval" : 20000,)" << endl;
	indent(configOfs, 2) << R"("rate"     : 48000)" << endl;
	indent(configOfs, 1) << "}," << endl;

	// Write the color-correction section
	indent(configOfs, 1) << R"("color" : )" << endl;
	indent(configOfs, 1) << "{" << endl;
	indent(configOfs, 2) << R"("red" : )" << endl;
	indent(configOfs, 2) << "{" << endl;
	indent(configOfs, 3) << R"("gamma"      : 1.0,)" << endl;
	indent(configOfs, 3) << R"("adjust"     : 1.0,)" << endl;
	indent(configOfs, 3) << R"("blacklevel" : 0.0)" << endl;
	indent(configOfs, 2) << "}," << endl;
	indent(configOfs, 2) << R"("green" : )" << endl;
	indent(configOfs, 2) << "{" << endl;
	indent(configOfs, 3) << R"("gamma"      : 1.0,)" << endl;
	indent(configOfs, 3) << R"("adjust"     : 1.0,)" << endl;
	indent(configOfs, 3) << R"("blacklevel" : 0.0)" << endl;
	indent(configOfs, 2) << "}," << endl;
	indent(configOfs, 2) << R"("blue" : )" << endl;
	indent(configOfs, 2) << "{" << endl;
	indent(configOfs, 3) << R"("gamma"      : 1.0,)" << endl;
	indent(configOfs, 3) << R"("adjust"     : 1.0,)" << endl;
	indent(configOfs, 3) << R"("blacklevel" : 0.0)" << endl;
	indent(configOfs, 2) << "}" << endl;
	indent(configOfs, 1) << "}," << endl;

	// Write the leds section
	indent(configOfs, 1) << R"("leds" : )" << endl;
	indent(configOfs, 1) << "[" << endl;
	for (const Led& led : leds)
	{
		// Add comments indicating the corners of the configuration
		if (led.minX == 0.0)
		{
			if (led.minY == 0.0)
			{
				indent(configOfs, 2) << "// TOP-LEFT Corner" << endl;
			}
			else if (led.maxY == 100.0)
			{
				indent(configOfs, 2) << "// BOTTOM-LEFT Corner" << endl;
			}
		}
		else if (led.maxX == 100.0)
		{
			if (led.minY == 0.0)
			{
				indent(configOfs, 2) << "// TOP-RIGHT Corner" << endl;
			}
			else if (led.maxY == 100.0)
			{
				indent(configOfs, 2) << "// BOTTOM-RIGHT Corner" << endl;
			}
		}

		// Write the configuration of the led
		indent(configOfs, 2) << "{" << endl;
		indent(configOfs, 3) << R"("index" : )" << led.index << "," << endl;
		indent(configOfs, 3) << R"("hscan" : { "minimum" : )" << led.minX << R"(, "maximum" : )" << led.maxX << R"( },)" << endl;
		indent(configOfs, 3) << R"("vscan" : { "minimum" : )" << led.minY << R"(, "maximum" : )" << led.maxY << R"( })" << endl;
		indent(configOfs, 2) << "}";
		if (led.index != leds.back().index)
		{
			configOfs << ",";
		}
		configOfs << endl;
	}
	indent(configOfs, 1) << "]" << endl;

	configOfs << "}" << endl;
}
