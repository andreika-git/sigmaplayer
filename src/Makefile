#########################################################################
#
# SigmaPlayer source project - makefile
#  \file       Makefile
#  \author     bombur
#  \version    0.3
#  \date       11.12.2008 4.05.2007 4.07.2004
#
##########################################################################


##########################################################################

MAIN_SRC := init.cpp

PROJECT_SRC := \
	divx.c \
	module.cpp \
	module-dvd.cpp \
	module-init.cpp \
	audio.cpp \
	video.cpp \
	avi.cpp \
	mpg.cpp \
	bitstream.cpp \
	cdda.cpp \
	media.cpp \
	script.cpp \
	script-explorer.cpp \
	script-objects.cpp \
	script-player.cpp \
	player.cpp \
	subtitle.cpp \
	settings.cpp

GUI_SRC := \
	gui/window.cpp \
	gui/console.cpp \
	gui/res.cpp \
	gui/jpeg.cpp \
	gui/giflib.cpp \
	gui/image.cpp \
	gui/rect.cpp \
	gui/text.cpp \
	gui/font.cpp

MMSL_SRC := \
	mmsl/mmsl.cpp \
	mmsl/mmsl-file.cpp

ifeq "$(PLAYER_MODEL)" "Technosonic"
SPCFLAGS += -DSP_PLAYER_TECHNOSONIC=1
LIBSP_SPECIFIC := \
	libsp/MP/sp_module.cpp \
	libsp/MP/sp_fip.cpp \
	libsp/MP/sp_khwl.cpp \
	libsp/MP/sp_cdrom.cpp \
	libsp/MP/sp_eeprom.cpp \
	libsp/MP/sp_i2c.cpp
endif

ifeq "$(PLAYER_MODEL)" "DreamX108"
SPCFLAGS += -DSP_PLAYER_DREAMX108=1
LIBSP_SPECIFIC := \
	libsp/MP/sp_module.cpp \
	libsp/MP/sp_fip.cpp \
	libsp/MP/sp_khwl.cpp \
	libsp/MP/sp_cdrom.cpp \
	libsp/MP/sp_eeprom.cpp \
	libsp/MP/sp_i2c.cpp
endif

ifeq "$(PLAYER_MODEL)" "Mecotek"
SPCFLAGS += -DSP_PLAYER_MECOTEK=1
LIBSP_SPECIFIC := \
	libsp/MP/sp_module.cpp \
	libsp/MP/sp_fip.cpp \
	libsp/MP/sp_khwl.cpp \
	libsp/MP/sp_cdrom.cpp \
	libsp/MP/sp_eeprom.cpp \
	libsp/MP/sp_i2c.cpp
endif

LIBSP_SRC := \
	libsp/sp_misc.cpp \
	libsp/sp_msg.cpp \
	libsp/sp_mpeg.cpp \
	libsp/sp_video.cpp \
	libsp/sp_khwl_colors.cpp \
	libsp/sp_flash.cpp \
	libsp/containers/string.cpp \
	libsp/containers/membin.cpp \
	$(LIBSP_SPECIFIC)

PREBUILD := cd contrib/libjpeg && make && cd ../libmad && make && cd ../..

EXTERNAL_STATIC_LINKS_WITH := \
	contrib/libjpeg/libjpeg.a \
	contrib/libmad/.libs/libmad.a

#	contrib/memcpy.o \
#	contrib/memset.o \
#	contrib/setjmp.o \
#	contrib/longjmp.o \
#	contrib/strcmp.o \
#	contrib/strlen.o \


SPINCLUDE = libsp/
JPEGINCLUDE = contrib/libjpeg/

SPCFLAGS += -msoft-float -fno-exceptions -I$(SPINCLUDE) -I$(JPEGINCLUDE)
SPCXXFLAGS += -fpermissive -fno-rtti

LOCAL_MAKEFILE := Makefile

TARGET_TYPE := EXECUTABLE

SRC := $(PROJECT_SRC) $(GUI_SRC) $(MMSL_SRC) $(LIBSP_SRC)

USE_STD_LIB := 1

COMPILKIND = release

PREPROCESSORFLAGS += -D__STDC_LIMIT_MACROS -DSP_ARM=1 -D_GNU_SOURCE

MAKE_CLEAN = rm -fr *.gdb && cd contrib/libmad && make clean && cd ../..
OTHER_CLEAN = contrib/memcpy.o contrib/memset.o contrib/setjmp.o contrib/strlen.o contrib/strcmp.o contrib/longjmp.o contrib/libjpeg/*.o contrib/libjpeg/*.a

CROSS = arm-elf-
LDFLAGS = -elf2flt="-s262144 -r" -s --static
EXEFLAGS = -msoft-float -lutil -lstdc++


include Makefile.inc

#divx.o: divx.c
#	arm-elf-gcc -I. -I$(SPINCLUDE) -c -O1 -o $@ $<
