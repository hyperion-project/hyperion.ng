
// STL includes
#include <fstream>
#include <sstream>
#include <iostream>

// Boblight includes
#include <boblight.h>

// PNGWriter includes
#define NO_FREETYPE
#include "pngwriter.h"

struct RaspiPng
{
	pngwriter writer;
	unsigned long fileIndex;

	unsigned frameCnt;

	std::ofstream logFile;
};

void* boblight_init()
{
	RaspiPng* raspiPng = new RaspiPng();

	raspiPng->writer.pngwriter_rename("/home/pi/RASPI_0000.png");
	raspiPng->fileIndex = 0;
	raspiPng->frameCnt  = 0;
	raspiPng->logFile.open("/home/pi/raspipng.log");

	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return reinterpret_cast<void*>(raspiPng);
}

void boblight_destroy(void* vpboblight)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	raspiPng->logFile.close();
	delete raspiPng;
}

void boblight_setscanrange(void* vpboblight, int width, int height)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << "(" << width << ", " << height << ")" << std::endl;

	raspiPng->writer.resize(width, height);
}

void boblight_addpixelxy(void* vpboblight, int x, int y, int* rgb)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);

	if (raspiPng->frameCnt%50 == 0)
	{
		// NB libpngwriter uses a one-based indexing scheme
		raspiPng->writer.plot(x+1,y+1, rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0);
	}
}

int boblight_sendrgb(void* vpboblight, int sync, int* outputused)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << "(" << sync << ", outputused) FRAME " << raspiPng->frameCnt++ << std::endl;

	if (raspiPng->frameCnt%50 == 0)
	{
		// Write-out the current frame and prepare for the next
		raspiPng->writer.write_png();

		++raspiPng->fileIndex;
		char filename[64];

		sprintf(filename, "/home/pi/RASPI_%04ld.png", raspiPng->fileIndex);

		raspiPng->writer.pngwriter_rename(filename);
	}

	return 1;
}

int boblight_connect(void* vpboblight, const char* address, int port, int usectimeout)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 1;
}

int boblight_setpriority(void* vpboblight, int priority)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 1;
}

const char* boblight_geterror(void* vpboblight)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return "ERROR";
}

int boblight_getnrlights(void* vpboblight)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 50;
}

const char* boblight_getlightname(void* vpboblight, int lightnr)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return "LIGHT";
}

int boblight_getnroptions(void* vpboblight)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 1;
}

const char* boblight_getoptiondescript(void* vpboblight, int option)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return "";
}

int boblight_setoption(void* vpboblight, int lightnr, const char* option)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 1;
}

int boblight_getoption(void* vpboblight, int lightnr, const char* option, const char** output)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 1;
}

int boblight_addpixel(void* vpboblight, int lightnr, int* rgb)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 1;
}


int boblight_ping(void* vpboblight, int* outputused)
{
	RaspiPng* raspiPng = reinterpret_cast<RaspiPng*>(vpboblight);
	raspiPng->logFile << __PRETTY_FUNCTION__ << std::endl;

	return 1;
}
