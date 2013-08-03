
// VC includes
#include <bcm_host.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

#include <json/json.h>
#include <utils/jsonschema/JsonFactory.h>


#include "dispmanx-helper.h"

static volatile bool sRunning = true;

void signal_handler(int signum)
{
	std::cout << "RECEIVED SIGNAL: " << signum << std::endl;
	sRunning = false;
}

int main(int /*argc*/, char** /*argv*/)
{
	// Install signal-handlers to exit the processing loop
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);

	const char* homeDir = getenv("RASPILIGHT_HOME");
	if (!homeDir)
	{
		homeDir = "/etc";
	}
	std::cout << "RASPILIGHT HOME DIR: " << homeDir << std::endl;

	const std::string schemaFile = std::string(homeDir) + "/hyperion.schema.json";
	const std::string configFile = std::string(homeDir) + "/hyperion.config.json";

	Json::Value raspiConfig;
	if (JsonFactory::load(schemaFile, configFile, raspiConfig) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << std::endl;
		return -1;
	}
	Hyperion hyperion(raspiConfig);

	dispmanx_process(hyperion, sRunning);

	return 0;
}
