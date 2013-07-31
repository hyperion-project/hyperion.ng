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

#include "timer.h"
#include "misc.h"
#include "timeutils.h"

#include <iostream>
using namespace std;

CTimer::CTimer(volatile bool* stop /*=NULL*/)
{
  m_interval = -1;
  m_timerstop = stop;
}

void CTimer::SetInterval(int64_t usecs)
{
  m_interval = usecs;
  Reset();
}

int64_t CTimer::GetInterval()
{
  return m_interval;
}

void CTimer::Reset()
{
  m_time = GetTimeUs();
}

void CTimer::Wait()
{
  int64_t sleeptime;

  //keep looping until we have a timestamp that's not too old
  int64_t now = GetTimeUs();
  do
  {
    m_time += m_interval;
    sleeptime = m_time - now;
  }
  while(sleeptime <= m_interval * -2LL);
  
  if (sleeptime > m_interval * 2LL) //failsafe, m_time must be bork if we get here
  {
    sleeptime = m_interval * 2LL;
    Reset();
  }

  USleep(sleeptime, m_timerstop);
}

