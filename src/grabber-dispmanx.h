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

#ifndef GRABBERDISPMANX
#define GRABBERDISPMANX

#include "bcm_host.h"

#include <string>

//#include "config.h"
#include "timer.h"

#ifndef ALIGN_UP
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#endif

//class for grabbing with DispmanX
class CGrabberDispmanX
{
  public:
	CGrabberDispmanX(void* boblight, volatile bool& stop, bool sync);
    ~CGrabberDispmanX();

    bool ExtendedSetup();
    bool Run();

    std::string& GetError()           { return m_error; }        //retrieves the latest error

    void SetInterval(double interval) { m_interval = interval; } //sets interval, negative means vblanks
    void SetSize(int size)            { m_size = size; }         //sets how many pixels we want to grab

    bool Setup();                                                //base setup function

    void SetDebug(const char* display);                          //turn on debug window

  protected:

    bool              Wait();                                    //wait for the timer or on the vblank
    volatile bool&    m_stop;

    std::string       m_error;                                   //latest error

    void*             m_boblight;                                //our handle from libboblight

    int               m_size;                                    //nr of pixels on lines to grab

    bool              m_debug;                                   //if we have debug mode on

    long double       m_lastupdate;
    long double       m_lastmeasurement;
    long double       m_measurements;
    int               m_nrmeasurements;

    double            m_interval;                                //interval in seconds, or negative for vblanks
    CTimer            m_timer;                                   //our timer
    bool              m_sync;                                    //sync mode for libboblight


  private:

    bool CheckExtensions();

    DISPMANX_DISPLAY_HANDLE_T   display;
    DISPMANX_MODEINFO_T         info;
    void                       *image;
    DISPMANX_UPDATE_HANDLE_T    update;
    DISPMANX_RESOURCE_HANDLE_T  resource;
    DISPMANX_ELEMENT_HANDLE_T   element;
    uint32_t                    vc_image_ptr;
    VC_IMAGE_TYPE_T             type;
    int 						pitch;
};

#endif //GRABBEROPENMAX
