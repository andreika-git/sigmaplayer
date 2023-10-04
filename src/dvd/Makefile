#########################################################################
#
# SigmaPlayer source project - DVD module makefile
#  \file       Makefile
#  \author     bombur
#  \version    0.3
#  \date       10.12.2008
#
##########################################################################


##########################################################################

MAIN_SRC := main.cpp

PROJECT_SRC := \
	dvd.cpp \
	dvd_player.cpp \
	dvd_misc.c \
	dvd_css.c \
	module.cpp \
	module-dvd.cpp \
	module-init.cpp \
	../libsp/containers/string.cpp

PREBUILD := cd ../contrib/libdvdcss && make && cd ../libdvdnav && make && cd ../../dvd

EXTERNAL_STATIC_LINKS_WITH := \
	../contrib/memcpy.o \
	../contrib/strcmp.o \
	../contrib/strlen.o \
	../contrib/libdvdnav/src/.libs/libdvdnav.a \
	../contrib/libdvdcss/src/.libs/libdvdcss.a

SPINCLUDE = ../libsp/
DVDINCLUDE = ../contrib/libdvdnav/src/
DVDINCLUDE1 = ../contrib/libdvdnav/src/dvdread/
DVDINCLUDE2 = ../contrib/libdvdnav/src/vm/
DVDINCLUDE3 = ../contrib/libdvdcss/
DVDINCLUDE4 = ../contrib/libdvdcss/src/

ifeq "$(PLAYER_MODEL)" "Technosonic"
SPCFLAGS += -DSP_PLAYER_TECHNOSONIC=1
LIBSP_SPECIFIC := \
	../libsp/MP/sp_module.cpp \
	../libsp/MP/sp_module_crt0.S

endif

ifeq "$(PLAYER_MODEL)" "DreamX108"
SPCFLAGS += -DSP_PLAYER_DREAMX108=1
LIBSP_SPECIFIC := \
	../libsp/MP/sp_module.cpp \
	../libsp/MP/sp_module_crt0.S
endif

ifeq "$(PLAYER_MODEL)" "Mecotek"
SPCFLAGS += -DSP_PLAYER_MECOTEK=1
LIBSP_SPECIFIC := \
	../libsp/MP/sp_module.cpp \
	../libsp/MP/sp_module_crt0.S
endif

SPCFLAGS += -fno-exceptions -I.. -I$(SPINCLUDE) -I$(DVDINCLUDE) -I$(DVDINCLUDE1) -I$(DVDINCLUDE2) -I$(DVDINCLUDE3) -I$(DVDINCLUDE4) -DDVDNAV_COMPILE=1 -D__UCLIBC_CTOR_DTOR__ -DCOMPILE_MODULE -D_GNU_SOURCE
SPCXXFLAGS += -fpermissive -fno-rtti

SPCFLAGS += -D_strdup=strdup -D_lseek=lseek -D_read=read -D_open=open -D_close=close

LOCAL_MAKEFILE := Makefile

TARGET_TYPE := EXECUTABLE

SRC := $(PROJECT_SRC) $(LIBSP_SPECIFIC)

USE_STD_LIB := 1

COMPILKIND = release

PREPROCESSORFLAGS += -D__STDC_LIMIT_MACROS -DSP_ARM=1

MAKE_CLEAN = rm -fr *.gdb && cd ../contrib/libdvdnav && make clean && cd ../libdvdcss && make clean && cd ../../dvd

CROSS = arm-elf-
LDFLAGS = -elf2flt="-s128" -s --static -nostdlib
EXEFLAGS =  -lgcc


include ../Makefile.inc

