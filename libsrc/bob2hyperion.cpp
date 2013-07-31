
// SysLog includes
#include <syslog.h>

// Boblight includes
#include "boblight.h"

// JsonSchema includes
#include <utils/jsonschema/JsonFactory.h>

// Raspilight includes
#include <hyperion/Hyperion.h>

static std::ofstream sDebugStream;

inline Hyperion* rasp_cast(void* hyperion_ptr)
{
	return reinterpret_cast<Hyperion*>(hyperion_ptr);
}

void* boblight_init()
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
//	syslog(LOG_INFO, __PRETTY_FUNCTION__);

	const char* homeDir = getenv("RASPILIGHT_HOME");
	if (!homeDir)
	{
		homeDir = "/etc";
	}
	syslog(LOG_INFO, "RASPILIGHT HOME DIR: %s", homeDir);

	const std::string schemaFile = std::string(homeDir) + "/hyperion.schema.json";
	const std::string configFile = std::string(homeDir) + "/hyperion.config.json";

	Json::Value raspiConfig;
	if (JsonFactory::load(schemaFile, configFile, raspiConfig) < 0)
	{
		syslog(LOG_WARNING, "UNABLE TO LOAD CONFIGURATION");
		return 0;
	}

	Hyperion* raspiLight = new Hyperion(raspiConfig);
	return reinterpret_cast<void*>(raspiLight);
}

void boblight_destroy(void* hyperion_ptr)
{
	syslog(LOG_INFO, __PRETTY_FUNCTION__);

	Hyperion* raspiLight = rasp_cast(hyperion_ptr);

	// Switch all leds to black (off)
	raspiLight->setColor(RgbColor::BLACK);

	delete raspiLight;

	sDebugStream.close();
}

void boblight_setscanrange(void* hyperion_ptr, int width, int height)
{
	syslog(LOG_INFO, __PRETTY_FUNCTION__);
	syslog(LOG_INFO, "Configuring scan range [%dx%d]", width, height);

	Hyperion* raspiLight = rasp_cast(hyperion_ptr);
	raspiLight->setInputSize(width, height);
}

void boblight_addpixelxy(void* hyperion_ptr, int x, int y, int* rgb)
{
	Hyperion* raspiLight = rasp_cast(hyperion_ptr);
	const RgbColor color = {uint8_t(rgb[0]), uint8_t(rgb[1]), uint8_t(rgb[2])};
	raspiLight->image().setPixel(x, y, color);
}

int boblight_sendrgb(void* hyperion_ptr, int sync, int* outputused)
{
	Hyperion* raspiLight = rasp_cast(hyperion_ptr);
	raspiLight->commit();

	return 1;
}

int boblight_connect(void* hyperion_ptr, const char* address, int port, int usectimeout)
{
	std::cout << "SUCCESFULL NO CONNECTION WITH BOBLIGHT" << std::endl;
	return 1;
}

const char* boblight_geterror(void* hyperion_ptr)
{
	return "ERROR";
}

int boblight_setpriority(void* hyperion_ptr, int priority)
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	return 1;
}


int boblight_getnrlights(void* hyperion_ptr)
{
	return 1;
}

const char* boblight_getlightname(void* hyperion_ptr, int lightnr)
{
	return "LIGHT_NAME";
}

int boblight_getnroptions(void* hyperion_ptr)
{
	return 1;
}

const char* boblight_getoptiondescript(void* hyperion_ptr, int option)
{
	return "OPTION-DESCRIPTION";
}

int boblight_setoption(void* hyperion_ptr, int lightnr, const char* option)
{
	return 1;
}

int boblight_getoption(void* hyperion_ptr, int lightnr, const char* option, const char** output)
{
	return 1;
}

int boblight_addpixel(void* hyperion_ptr, int lightnr, int* rgb)
{
	return 1;
}

int boblight_ping(void* hyperion_ptr, int* outputused)
{
	return 1;
}
