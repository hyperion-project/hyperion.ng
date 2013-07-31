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

#include "timeutils.h"

void USleep(int64_t usecs, volatile bool* stop /*= NULL*/)
{
  if (usecs <= 0)
  {
    return;
  }
  else if (stop && usecs > 1000000) //when a pointer to a bool is passed, check it every second and stop when it's true
  {
    int64_t now = GetTimeUs();
    int64_t end = now + usecs;

    while (now < end)
    {
      struct timespec sleeptime = {};

      if (*stop)
        return;
      else if (end - now >= 1000000)
        sleeptime.tv_sec = 1;
      else
        sleeptime.tv_nsec = ((end - now) % 1000000) * 1000;

      nanosleep(&sleeptime, NULL);
      now = GetTimeUs();
    }
  }
  else
  {
    struct timespec sleeptime;
    sleeptime.tv_sec = usecs / 1000000;
    sleeptime.tv_nsec = (usecs % 1000000) * 1000;

    nanosleep(&sleeptime, NULL);
  }
}

