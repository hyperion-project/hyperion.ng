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

#ifndef FLAGMANAGERDISPMANX
#define FLAGMANAGERDISPMANX

#include "flagmanager.h"

class CFlagManagerDispmanX : public CFlagManager
{
public:
	CFlagManagerDispmanX();

	int  m_color;

	void        ParseFlagsExtended(int& argc, char**& argv, int& c, char*& optarg); //we load our own flags here
	void        PrintHelpMessage();

	double      m_interval;           //grab interval in seconds, or vertical blanks when negative
	int         m_pixels;             //number of pixels on lines to capture
	bool        m_debug;              //if true we make a little window where we put our captured output on, looks pretty
	const char* m_debugdpy;           //display to place the debug window on, default is NULL

private:

	std::string m_strdebugdpy;        //place to store the debug dpy, cause our copied argv is destroyed
};

#endif //FLAGMANAGEROPENMAX
