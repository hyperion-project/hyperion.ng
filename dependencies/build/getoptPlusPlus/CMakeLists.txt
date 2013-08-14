# Copyright (c) 2012 TNO, The Netherlands.
#
# This file contains information proprietary to TNO.
#
# Any disclosure or use of this information or any reproduction of this document or any part thereof for
# other than the specified purpose for which it is intended is expressly prohibited except as TNO may
# otherwise agree to in writing.

project(getoptPlusPlus)

include_directories(
    ../../include/getoptPlusPlus)

add_library(getoptPlusPlus
	../../include/getoptPlusPlus/getoptpp.h
	../../include/getoptPlusPlus/parameter.include.cc
	getoptpp.cc)
