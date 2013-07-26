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

//if you define BOBLIGHT_DLOPEN, all boblight functions are defined as pointers
//you can then call boblight_loadlibrary to load libboblight with dlopen and the function pointers with dlsym
//if you pass NULL to boblight_loadlibrary's first argument, the default filename for libboblight is used
//if boblight_loadlibrary returns NULL, dlopen and dlsym went ok, if not it returns a char* from dlerror

//if you want to use the boblight functions from multiple files, you can define BOBLIGHT_DLOPEN in one file,
//and define BOBLIGHT_DLOPEN_EXTERN in the other file, the functionpointers are then defined as extern

#ifndef LIBBOBLIGHT
#define LIBBOBLIGHT

  #if !defined(BOBLIGHT_DLOPEN) && !defined(BOBLIGHT_DLOPEN_EXTERN)

    #ifdef __cplusplus
      extern "C" {
    #endif

    //generate normal prototypes
    #define BOBLIGHT_FUNCTION(returnvalue, name, arguments) returnvalue name arguments
    #include "boblight-functions.h"
    #undef BOBLIGHT_FUNCTION

    #ifdef __cplusplus
      }
    #endif

  #elif defined(BOBLIGHT_DLOPEN)

    #include <dlfcn.h>
    #include <stddef.h>

    //generate function pointers
    #define BOBLIGHT_FUNCTION(returnvalue, name, arguments) returnvalue (* name ) arguments = NULL
    #include "boblight-functions.h"
    #undef BOBLIGHT_FUNCTION

    #ifdef __cplusplus
      #define BOBLIGHT_CAST(value) reinterpret_cast<value>
    #else
      #define BOBLIGHT_CAST(value) (value)
    #endif

    //gets a functionpointer from dlsym, and returns char* from dlerror if it didn't work
    #define BOBLIGHT_FUNCTION(returnvalue, name, arguments) \
    name = BOBLIGHT_CAST(returnvalue (*) arguments)(dlsym(p_boblight, #name)); \
                        { char* error = dlerror(); if (error) return error; }

    void* p_boblight = NULL; //where we put the lib

    //load function pointers
    char* boblight_loadlibrary(const char* filename)
    {
      if (filename == NULL)
        filename = "libboblight.so";

      if (p_boblight != NULL)
      {
        dlclose(p_boblight);
        p_boblight = NULL;
      }

      p_boblight = dlopen(filename, RTLD_NOW);
      if (p_boblight == NULL)
        return dlerror();

      //generate dlsym lines
      #include "boblight-functions.h"

      return NULL;
    }
    #undef BOBLIGHT_FUNCTION
    #undef BOBLIGHT_CAST

  //you can define BOBLIGHT_DLOPEN_EXTERN when you load the library in another file
  #elif defined(BOBLIGHT_DLOPEN_EXTERN)

    extern char* boblight_loadlibrary(const char* filename);
    extern void* p_boblight;
    #define BOBLIGHT_FUNCTION(returnvalue, name, arguments) extern returnvalue (* name ) arguments
    #include "boblight-functions.h"
    #undef BOBLIGHT_FUNCTION

  #endif //BOBLIGHT_DLOPEN_EXTERN
#endif //LIBBOBLIGHT

