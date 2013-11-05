project(getoptPlusPlus)

include_directories(
    ../../include/getoptPlusPlus)

add_library(getoptPlusPlus
	../../include/getoptPlusPlus/getoptpp.h
	../../include/getoptPlusPlus/parameter.include.cc
	getoptpp.cc)
