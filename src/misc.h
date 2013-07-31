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

#ifndef MISCUTIL
#define MISCUTIL

#include "stdint.h"

#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void PrintError(const std::string& error);
bool GetWord(std::string& data, std::string& word);
void ConvertFloatLocale(std::string& strfloat);

template <class Value>
inline std::string ToString(Value value)
{
  std::string data;
  std::stringstream valuestream;
  valuestream << value;
  valuestream >> data;
  return data;
}

inline std::string GetErrno()
{
  return strerror(errno);
}

inline std::string GetErrno(int err)
{
  return strerror(err);
}

template <class A, class B, class C>
inline A Clamp(A value, B min, C max)
{
  return value < max ? (value > min ? value : min) : max;
}

template <class A, class B>
inline A Max(A value1, B value2)
{
  return value1 > value2 ? value1 : value2;
}

template <class A, class B, class C>
inline A Max(A value1, B value2, C value3)
{
  return (value1 > value2) ? (value1 > value3 ? value1 : value3) : (value2 > value3 ? value2 : value3);
}

template <class A, class B>
inline A Min(A value1, B value2)
{
  return value1 < value2 ? value1 : value2;
}

template <class A, class B, class C>
inline A Min(A value1, B value2, C value3)
{
  return (value1 < value2) ? (value1 < value3 ? value1 : value3) : (value2 < value3 ? value2 : value3);
}

template <class T>
inline T Abs(T value)
{
  return value > 0 ? value : -value;
}

template <class A, class B>
inline A Round(B value)
{
  if (value == 0.0)
  {
    return 0;
  }
  else if (value > 0.0)
  {
    return (A)(value + 0.5);
  }
  else
  {
    return (A)(value - 0.5);
  }
}

inline int32_t Round32(float value)
{
  return lroundf(value);
}

inline int32_t Round32(double value)
{
  return lround(value);
}

inline int64_t Round64(float value)
{
  return llroundf(value);
}

inline int64_t Round64(double value)
{
  return llround(value);
}

inline bool StrToInt(const std::string& data, int& value)
{
  return sscanf(data.c_str(), "%i", &value) == 1;
}

inline bool StrToInt(const std::string& data, int64_t& value)
{
  return sscanf(data.c_str(), "%ld", &value) == 1;
}

inline bool HexStrToInt(const std::string& data, int& value)
{
  return sscanf(data.c_str(), "%x", &value) == 1;
}

inline bool HexStrToInt(const std::string& data, int64_t& value)
{
  return sscanf(data.c_str(), "%x", &value) == 1;
}

inline bool StrToFloat(const std::string& data, float& value)
{
  return sscanf(data.c_str(), "%f", &value) == 1;
}

inline bool StrToFloat(const std::string& data, double& value)
{
  return sscanf(data.c_str(), "%lf", &value) == 1;
}

inline bool StrToBool(const std::string& data, bool& value)
{
  std::string data2 = data;
  std::string word;
  if (!GetWord(data2, word))
    return false;
  
  if (word == "1" || word == "true" || word == "on" || word == "yes")
  {
    value = true;
    return true;
  }
  else if (word == "0" || word == "false" || word == "off" || word == "no")
  {
    value = false;
    return true;
  }
  else
  {
    int ivalue;
    if (StrToInt(word, ivalue))
    {
      value = ivalue != 0;
      return true;
    }
  }

  return false;
}

#endif //MISCUTIL
