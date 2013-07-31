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

#ifndef CTIMER
#define CTIMER

#include "stdint.h"

#include <unistd.h>

class CTimer
{
  public:
    CTimer(volatile bool* stop = NULL);
    void SetInterval(int64_t usecs);
    virtual void Wait();
    void Reset();

    int64_t GetInterval();

  protected:
    int64_t        m_time;
    int64_t        m_interval;
    volatile bool* m_timerstop;
};

#endif
