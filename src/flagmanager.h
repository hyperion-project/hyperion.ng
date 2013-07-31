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

#ifndef FLAGMANAGER
#define FLAGMANAGER

#define VIDEOGAMMA 2.2

#include <string>
#include <vector>

//class for making a copy of argc and argv
class CArguments
{
  public:
    CArguments(int argc, char** argv);
    ~CArguments();

    int    m_argc;
    char** m_argv;
};

class CFlagManager
{
  public:
    CFlagManager();

    bool         m_printhelp;                               //if we need to print the help message
    bool         m_printboblightoptions;                    //if we need to print the boblight options

    const char*  m_address;                                 //address to connect to, set to NULL if none given for default
    int          m_port;                                    //port to connect to, set to -1 if none given for default
    int          m_priority;                                //priority, set to 128 if none given for default
    bool         m_fork;                                    //if we should fork
    bool         m_sync;                                    //if sync mode is enabled

    void         ParseFlags(int tempargc, char** tempargv); //parsing commandline flags
    virtual void PrintHelpMessage() {};

    void         PrintBoblightOptions();                    //printing of boblight options (-o [light:]option=value)
    void         ParseBoblightOptions(void* boblight);      //parsing of boblight options
    bool         SetVideoGamma();                           //set gamma to 2.2 if not given, returns true if done

  protected:

    std::string  m_flags;                                   //string to pass to getopt, for example "c:r:a:p"
    std::string  m_straddress;                              //place to store address to connect to, because CArguments deletes argv

    std::vector<std::string> m_options;                     //place to store boblight options

    //gets called from ParseFlags, for derived classes
    virtual void ParseFlagsExtended(int& argc, char**& argv, int& c, char*& optarg){};
    //gets called after getopt
    virtual void PostGetopt(int optind, int argc, char** argv) {};
};

#endif //FLAGMANAGER
