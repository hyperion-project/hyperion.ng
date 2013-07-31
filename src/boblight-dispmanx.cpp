/*
 * boblight
 * Copyright (C) Bob  2009 
 * 
 * boblight is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * boblight is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#define BOBLIGHT_DLOPEN
#include "boblight.h"

#include <iostream>
#include <string>
#include <vector>
#include <signal.h>
#include <unistd.h>

//#include "config.h"
#include "misc.h"
#include "flagmanager-dispmanx.h"
#include "grabber-dispmanx.h"

//OpenMax includes
#include "bcm_host.h"
#include <assert.h>

using namespace std;

#define TEST_SAVE_IMAGE 0
#define PRINT_RET_VAL(ret, func, ...) ret = func(__VA_ARGS__); printf(#func " returned %d\n", ret);

int  Run();
void SignalHandler(int signum);

volatile bool stop = false;

CFlagManagerDispmanX g_flagmanager;

int main(int argc, char *argv[])
{
	std::cout << "HACKED VERSION WITH TIMOLIGHT" << std::endl;
  //load the boblight lib, if it fails we get a char* from dlerror()
//  char* boblight_error = boblight_loadlibrary(NULL);
//  if (boblight_error)
//  {
//    PrintError(boblight_error);
//    return 1;
//  }

  //try to parse the flags and bitch to stderr if there's an error
  try
  {
    g_flagmanager.ParseFlags(argc, argv);
  }
  catch (string error)
  {
    PrintError(error);
    g_flagmanager.PrintHelpMessage();
    return 1;
  }

  if (g_flagmanager.m_printhelp) //print help message
  {
    g_flagmanager.PrintHelpMessage();
    return 1;
  }

  if (g_flagmanager.m_printboblightoptions) //print boblight options (-o [light:]option=value)
  {
    g_flagmanager.PrintBoblightOptions();
    return 1;
  }

  if (g_flagmanager.m_fork)
  {
    if (fork())
      return 0;
  }

  //set up signal handlers
  signal(SIGTERM, SignalHandler);
  signal(SIGINT, SignalHandler);

  //keep running until we want to quit
  return Run();
}

int Run()
{
	while(!stop)
	{
		//init boblight
		void* boblight = boblight_init();

		cout << "Connecting to boblightd\n";

		//try to connect, if we can't then bitch to stderr and destroy boblight
		if (!boblight_connect(boblight, g_flagmanager.m_address, g_flagmanager.m_port, 5000000) ||
				!boblight_setpriority(boblight, g_flagmanager.m_priority))
		{
			PrintError(boblight_geterror(boblight));
			cout << "Waiting 10 seconds before trying again\n";
			boblight_destroy(boblight);
			sleep(10);
			continue;
		}

		cout << "Connection to boblightd opened\n";

		//try to parse the boblight flags and bitch to stderr if we can't
		try
		{
			g_flagmanager.ParseBoblightOptions(boblight);
		}
		catch (string error)
		{
			PrintError(error);
			return 1;
		}

		CGrabberDispmanX *grabber = new CGrabberDispmanX(boblight, stop, g_flagmanager.m_sync);

		grabber->SetInterval(g_flagmanager.m_interval);
		grabber->SetSize(g_flagmanager.m_pixels);

		if (!grabber->Setup()) //just exit if we can't set up the grabber
		{
			PrintError(grabber->GetError());
			delete grabber;
			boblight_destroy(boblight);
			return 1;
		}

		if (!grabber->Run()) //just exit if some unrecoverable error happens
		{
			PrintError(grabber->GetError());
			delete grabber;
			boblight_destroy(boblight);
			return 1;
		}
		else //boblightd probably timed out, so just try to reconnect
		{
			if (!grabber->GetError().empty())
				PrintError(grabber->GetError());
		}

		delete grabber;

		boblight_destroy(boblight);
	}

	cout << "Exiting\n";

	return 0;
}

void SignalHandler(int signum)
{
  if (signum == SIGTERM)
  {
    cout << "caught SIGTERM\n";
    stop = true;
  }
  else if (signum == SIGINT)
  {
    cout << "caught SIGINT\n";
    stop = true;
  }
}
