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

#include <string.h>
#include <unistd.h>
#include <iostream>

#include "flagmanager.h"
#include "misc.h"

//#define BOBLIGHT_DLOPEN_EXTERN
#include "boblight.h"

using namespace std;

//very simple, store a copy of argc and argv
CArguments::CArguments(int argc, char** argv)
{
  m_argc = argc;

  if (m_argc == 0)
  {
    m_argv = NULL;
  }
  else
  {
    m_argv = new char*[m_argc];
    for (int i = 0; i < m_argc; i++)
    {
      m_argv[i] = new char[strlen(argv[i]) + 1];
      strcpy(m_argv[i], argv[i]);
    }
  }
}

//delete the copy of argv
CArguments::~CArguments()
{
  if (m_argv)
  {
    for (int i = 0; i < m_argc; i++)
    {
      delete[] m_argv[i];
    }
    delete[] m_argv;
  }
}

CFlagManager::CFlagManager()
{
  m_port = -1;                    //-1 tells libboblight to use default port
  m_address = NULL;               //NULL tells libboblight to use default address
  m_priority = 128;               //default priority
  m_printhelp = false;            //don't print helpmessage unless asked to
  m_printboblightoptions = false; //same for libboblight options
  m_fork = false;                 //don't fork by default
  m_sync = false;                 //sync mode disabled by default

  // default getopt flags, can be extended in derived classes
  // p = priority, s = address[:port], o = boblight option, l = list boblight options, h = print help message, f = fork
  m_flags = "p:s:o:lhfy:";
}

void CFlagManager::ParseFlags(int tempargc, char** tempargv)
{
  //that copy class sure comes in handy now!
  CArguments arguments(tempargc, tempargv);
  int    argc = arguments.m_argc;
  char** argv = arguments.m_argv;

  string option;
  int    c;

  opterr = 0; //we don't want to print error messages
  
  while ((c = getopt(argc, argv, m_flags.c_str())) != -1)
  {
    if (c == 'p') //priority
    {
      option = optarg;
      if (!StrToInt(option, m_priority) || m_priority < 0 || m_priority > 255)
      {
        throw string("Wrong option " + string(optarg) + " for argument -p");
      }
    }
    else if (c == 's') //address[:port]
    {
      option = optarg;
      //store address in string and set the char* to it
      m_straddress = option.substr(0, option.find(':'));
      m_address = m_straddress.c_str();

      if (option.find(':') != string::npos) //check if we have a port
      {
        option = option.substr(option.find(':') + 1);
        string word;
        if (!StrToInt(option, m_port) || m_port < 0 || m_port > 65535)
        {
          throw string("Wrong option " + string(optarg) + " for argument -s");
        }
      }
    }
    else if (c == 'o') //option for libboblight
    {
      m_options.push_back(optarg);
    }
    else if (c == 'l') //list libboblight options
    {
      m_printboblightoptions = true;
      return;
    }
    else if (c == 'h') //print help message
    {
      m_printhelp = true;
      return;
    }
    else if (c == 'f')
    {
      m_fork = true;
    }
    else if (c == 'y')
    {
      if (!StrToBool(optarg, m_sync))
      {
        throw string("Wrong value " + string(optarg) + " for sync mode");
      }
    }
    else if (c == '?') //unknown option
    {
      //check if we know this option, but expected an argument
      if (m_flags.find(ToString((char)optopt) + ":") != string::npos)
      {
        throw string("Option " + ToString((char)optopt) + "requires an argument");
      }
      else
      {
        throw string("Unkown option " + ToString((char)optopt));
      }
    }
    else
    {
      ParseFlagsExtended(argc, argv, c, optarg); //pass our argument to a derived class
    }
  }

  PostGetopt(optind, argc, argv); //some postprocessing
}

//go through all options and print the descriptions to stdout
void CFlagManager::PrintBoblightOptions()
{
  void* boblight = boblight_init();
  int nroptions = boblight_getnroptions(boblight);

  for (int i = 0; i < nroptions; i++)
  {
    cout << boblight_getoptiondescript(boblight, i) << "\n";
  }

  boblight_destroy(boblight);
}

void CFlagManager::ParseBoblightOptions(void* boblight)
{
  int nrlights = boblight_getnrlights(boblight);
  
  for (int i = 0; i < m_options.size(); i++)
  {
    string option = m_options[i];
    string lightname;
    string optionname;
    string optionvalue;
    int    lightnr = -1;

    //check if we have a lightname, otherwise we use all lights
    if (option.find(':') != string::npos)
    {
      lightname = option.substr(0, option.find(':'));
      if (option.find(':') == option.size() - 1) //check if : isn't the last char in the string
      {
        throw string("wrong option \"" + option + "\", syntax is [light:]option=value");
      }
      option = option.substr(option.find(':') + 1); //shave off the lightname

      //check which light this is
      bool lightfound = false;
      for (int j = 0; j < nrlights; j++)
      {
        if (lightname == boblight_getlightname(boblight, j))
        {
          lightfound = true;
          lightnr = j;
          break;
        }
      }
      if (!lightfound)
      {
        throw string("light \"" + lightname + "\" used in option \"" + m_options[i] + "\" doesn't exist");
      }
    }

    //check if '=' exists and it's not at the end of the string
    if (option.find('=') == string::npos || option.find('=') == option.size() - 1)
    {
      throw string("wrong option \"" + option + "\", syntax is [light:]option=value");
    }

    optionname = option.substr(0, option.find('='));   //option name is everything before = (already shaved off the lightname here)
    optionvalue = option.substr(option.find('=') + 1); //value is everything after =

    option = optionname + " " + optionvalue;           //libboblight wants syntax without =

    //bitch if we can't set this option
    if (!boblight_setoption(boblight, lightnr, option.c_str()))
    {
      throw string(boblight_geterror(boblight));
    }
  }
}

bool CFlagManager::SetVideoGamma()
{
  for (int i = 0; i < m_options.size(); i++)
  {
    string option = m_options[i];
    if (option.find(':') != string::npos)
      option = option.substr(option.find(':') + 1); //shave off the lightname

    if (option.find('=') != string::npos)
    {
      if (option.substr(0, option.find('=')) == "gamma")
        return false; //gamma set by user, don't override
    }
  }

  m_options.push_back("gamma=" + ToString(VIDEOGAMMA));

  cout << "Gamma not set, using " << VIDEOGAMMA << " since this is default for video\n";

  return true;
}

