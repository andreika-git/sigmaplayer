#########################################################################
#
# SigmaPlayer source project - player_info makefile
#  \file       player_info.make
#  \author     bombur
#  \version    0.1
#  \date       15.2.2007
#
##########################################################################


MAIN_SRC := info/player_info.cpp

PREBUILD := cd contrib/libid3tag && make && cd ../..

EXTERNAL_STATIC_LINKS_WITH := \
	contrib/libid3tag/.libs/libid3tag.a \
	contrib/libid3tag/fakezlib/libz.a

#	contrib/memcpy.o \
#	contrib/memset.o \


SPINCLUDE = libsp/
JPEGINCLUDE = contrib/libjpeg/
DVDINCLUDE = contrib/libdvdnav/src/
DVDINCLUDE1 = contrib/libdvdnav/src/dvdread/
DVDINCLUDE2 = contrib/libdvdnav/src/vm/
DVDINCLUDE3 = contrib/libdvdcss/
DVDINCLUDE4 = contrib/libdvdcss/src/
ID3V2INCLUDE = contrib/libid3tag/

SPCFLAGS += -msoft-float -Iinfo -I$(SPINCLUDE) -I$(JPEGINCLUDE) -I$(DVDINCLUDE) -I$(DVDINCLUDE1) -I$(DVDINCLUDE2) -I$(DVDINCLUDE3) -I$(DVDINCLUDE4) -I$(ID3V2INCLUDE) -DDVDNAV_COMPILE=1 
#SPCFLAGS += -I$(SPINCLUDE) -I$(ID3V2INCLUDE)

LOCAL_MAKEFILE := Makefile

TARGET_TYPE := EXECUTABLE

USE_STD_LIB := 1

COMPILKIND = release

PREPROCESSORFLAGS += -D__STDC_LIMIT_MACROS

MAKE_CLEAN = -rm -fr info/player_info.gdb && cd contrib/libid3tag && make clean && cd ../..

CROSS = arm-elf-
LDFLAGS = -elf2flt="-s262144" -s --static
EXEFLAGS = -msoft-float -lutil -lstdc++

include Makefile.inc
