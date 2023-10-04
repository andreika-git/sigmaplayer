#########################################################################
#
# SigmaPlayer source project - makefile additional code
#  \file       Makefile.inc
#  \author     bombur
#  \version    0.1
#  \date       4.07.2004
#
##########################################################################

ifndef DEPEND_FILE
	DEPEND_FILE = Makefile.d
endif

ifndef MAKEFILE_NAME
	MAKEFILE_NAME = Makefile
endif	

ifndef LOCAL_MAKEFILE
	LOCAL_MAKEFILE = $(MAKEFILE_NAME)
endif

ifndef CCOMPILER
	CCOMPILER := gcc
endif

ifndef CXXCOMPILER
	CXXCOMPILER := g++
endif

CC := $(CROSS)$(CCOMPILER)
CXX := $(CROSS)$(CXXCOMPILER)
LD := $(CROSS)$(LD)
OBJCOPY := $(CROSS)objcopy
AR := $(CROSS)ar

ifndef LINKER
	LINKER = $(CC)   # must be = and not :=
else
	LINKER := $(CROSS)$(LINKER)
endif

LOCALDIR = $(shell pwd)

MAKEARGS = -f $(MAKEFILE_NAME)
PREPROCESSORFLAGS += $(SPCFLAGS) -I. -D_REENTRANT

ASFLAGS = $(PREPROCESSORFLAGS)

ifeq ($(findstring leakchecker, $(COMPILKIND)), leakchecker)
EXEFLAGS += `glib-config --libs glib`
PREPROCESSORFLAGS += -DUSE_LEAK_CHECKER=1
endif

ifeq ($(findstring withthreads, $(COMPILKIND)), withthreads)
PREPROCESSORFLAGS += -DWITH_THREADS=1
LDFLAGS += -lpthread 
endif

ifeq ($(findstring withfeeblemm, $(COMPILKIND)), withfeeblemm)
PREPROCESSORFLAGS += -DWITH_FEEBLEMM=1
endif

ifeq ($(findstring withdl, $(COMPILKIND)), withdl)
EXEFLAGS += -ldl -rdynamic
endif

ifeq ($(findstring kernel, $(COMPILKIND)), kernel)
PREPROCESSORFLAGS += -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB
ifeq ($(findstring uclinux, $(COMPILKIND)), uclinux)
ifndef MUM_KI
MUM_KI=-I$(UCLINUX_KERNEL)/include
endif
ifndef KERNELSTYLE
KERNELSTYLE= \
        -fomit-frame-pointer \
        -fno-strict-aliasing \
        -fno-common \
	-pipe \
	-fno-builtin
PREPROCESSORFLAGS += -D__linux__
endif
else
ifndef MUM_KI
MUM_KI=-I/usr/src/linux-$(shell uname -r)/include -I/usr/src/linux-2.4/include
endif
ifndef KERNELSTYLE
KERNELSTYLE= \
        -fomit-frame-pointer \
        -fno-strict-aliasing \
        -fno-common \
	-mpreferred-stack-boundary=2 \
	-pipe 
endif
endif

ifndef DISABLE_WARNINGS
CWARNINGS+= \
	-Wno-import \
        -Wunused \
        -Wimplicit \
        -Wmain \
        -Wreturn-type \
        -Wswitch \
        -Wtrigraphs \
        -Wchar-subscripts \
        -Wparentheses \
        -Wpointer-arith \
        -Wcast-align \
#	-Wuninitialized \
	-Wno-uninitialized
	-O3
else
CWARNINGS+= -O3
endif

CFLAGS += $(KERNELSTYLE) $(MUM_KI)

else

ifeq ($(findstring release, $(COMPILKIND)), release)

	PREPROCESSORFLAGS += -U_DEBUG -DNDEBUG
	CFLAGS += -O3
	CXXFLAGS += -O3

else

	PREPROCESSORFLAGS += -D_DEBUG=1 

ifeq "$(CCOMPILER)" "gcc"
ifeq "$(CROSS)" ""
	CFLAGS += -gdwarf-2 -g3
	CXXFLAGS += -gdwarf-2 -g3
endif
else
	CFLAGS += -g
	CXXFLAGS += -g
endif
endif 


ifndef DISABLE_WARNINGS
COMMONWARNINGS+= \
	-Wundef \
	-Wall \
	-Wchar-subscripts \
	-Wsign-compare \
	-Wno-uninitialized \
	-O3
#	-Wmissing-prototypes \
#	-Wuninitialized \
#	-Werror 
CWARNINGS:=$(COMMONWARNINGS) -Wnested-externs -Wmissing-declarations 
CXXWARNINGS:=$(COMMONWARNINGS) -fcheck-new 
endif

ifndef USE_STD_LIB
	PREPROCESSORFLAGS += -nostdinc
endif

endif

CFLAGS += $(PREPROCESSORFLAGS) $(CWARNINGS)
CXXFLAGS += $(PREPROCESSORFLAGS) $(SPCXXFLAGS) $(CXXWARNINGS) 

TARGET_TYPE := $(strip $(TARGET_TYPE))

ifeq "$(TARGET_TYPE)" "LIBRARY"
ifeq ($(findstring static, $(COMPILKIND)), static)
TARGET_TYPE :=  OBJECT_LIBRARY
else
TARGET_TYPE :=  SHARED_LIBRARY
endif
endif

ifeq ($(findstring static, $(COMPILKIND)), static)
STATIC_LINKS_WITH += $(LINKS_WITH)
LDFLAGS += -static
else
ifeq ($(findstring implicit, $(COMPILKIND)), implicit)
IMPLICIT_LINKS_WITH := $(LINKS_WITH)
else
DEPENDS_ON += $(LINKS_WITH)
endif
endif

ifdef OBJECT_LIBRARY_USE_AR
OBJECT_LIBRARY = $(AR) rc $@ $^
else
OBJECT_LIBRARY = $(LD) -r $^ -o $@
endif
SHARED_LIBRARY = $(LINKER) -shared $^ $(LDFLAGS) -o $@
EXECUTABLE = $(LINKER) $^ $(LDFLAGS) $(EXEFLAGS) -o $@
DEPEND_COMMAND = $(CC) $(MUM_KI) $(PREPROCESSORFLAGS) -MM -E $(SRC) $(MAIN_SRC)
BINARY_FILE = $(OBJCOPY) -O binary $< $@

define MICROCODE
	$(SPASM) $^ -o $@
        $(SPBIN) -h $@
endef

COPY_FILE_LIST = sed 's/^.*://' | sed 's/\\$$//' | tr ' ' '\n' | sed '/^$$/d' | \
		sed '/^\//d' | \
		sed 's/^\([^/]\)/$(subst /,\/,$(LOCALDIR))\/\1/' | \
		sed '{ : rm_dotdot s/[^/]*\/\.\.\/// ; t rm_dotdot }' | sort -u | \
		xargs cp -f --parents --target-directory=$(1)

COPY_FILE_LIST2 = sed 's/^.*://' | sed 's/\\$$//' | tr ' ' '\n' | sed '/^$$/d' | \
        sed '/^\//d' | sort -u | \
        xargs -l1 cp -f --parents --target-directory=$(1)

REDUCE_DIR_TREE = cd $(1) ; x=1 ; while [ $$x -eq 1 ] ; \
		do x=0 ; [ `ls -1 | wc -l` -eq 1 -a -d `ls -1 | head -n 1` ] && \
		y=`ls -1` && mv $$y/* . && rmdir $$y && x=1 ; \
		done ; exit 0

SRC := $(strip $(SRC))
MAIN_SRC := $(strip $(MAIN_SRC))
OBJ := $(addsuffix .o, $(basename $(SRC)))
MAIN_OBJ := $(addsuffix .o, $(basename $(MAIN_SRC)))
EXE := $(basename $(MAIN_SRC))
SUBDIR := $(dir $(STATIC_LINKS_WITH) $(IMPLICIT_LINKS_WITH) $(DEPENDS_ON))

ifneq ($(findstring clean, $(MAKECMDGOALS)) ,clean)
	ifneq "$(SRC) $(MAIN_SRC)" " "
		DEPEND := $(shell cat $(DEPEND_FILE) 2>/dev/null)
		DEPEND := $(subst \, ,$(DEPEND))  
		DEPEND := $(subst :, , $(DEPEND)) 
		DEPEND := $(filter-out %.o, $(DEPEND))
		DEPEND := $(SRC) $(MAIN_SRC) $(DEPEND)
		DEPEND := $(sort $(DEPEND))  
	endif
endif

ifeq "$(TARGET_TYPE)" "OBJFILE"
TARGET := $(OBJ)
endif

ifeq "$(TARGET_TYPE)" "EXECUTABLE"
TARGET := $(EXE)
endif

all: prebuild $(SUBDIR) $(TARGET) postbuild

local: prebuild $(TARGET) postbuild

ifeq "$(TARGET_TYPE)" "SHARED_LIBRARY"
copy_shared: $(TARGET) $(SUBDIR)
	[ -d $(LIBDIR) ] && cp -f $(TARGET) $(LIBDIR)
else
copy_shared: $(SUBDIR)
endif

ifeq "$(TARGET_TYPE)" "OBJECT_LIBRARY"
$(TARGET): $(OBJ) $(STATIC_LINKS_WITH) $(EXTERNAL_STATIC_LINKS_WITH)
	$(OBJECT_LIBRARY)
endif

ifeq "$(TARGET_TYPE)" "SHARED_LIBRARY"
$(TARGET): $(OBJ) $(STATIC_LINKS_WITH) $(EXTERNAL_STATIC_LINKS_WITH) $(IMPLICIT_LINKS_WITH)
	$(SHARED_LIBRARY)
endif

ifeq "$(TARGET_TYPE)" "EXECUTABLE"
$(TARGET): %: %.o $(OBJ) $(STATIC_LINKS_WITH) $(EXTERNAL_STATIC_LINKS_WITH) $(IMPLICIT_LINKS_WITH)
	$(EXECUTABLE)
endif

ifeq "$(TARGET_TYPE)" "MICROCODE"
$(TARGET): $(SRC)
	$(MICROCODE)
endif

ifeq "$(TARGET_TYPE)" "BINARY_FILE"
$(TARGET): $(EXE)
	$(BINARY_FILE)

$(EXE): %: %.o $(OBJ) $(STATIC_LINKS_WITH) $(EXTERNAL_STATIC_LINKS_WITH) $(IMPLICIT_LINKS_WITH)
	$(EXECUTABLE)
endif

$(STATIC_LINKS_WITH): $(@D)

$(IMPLICIT_LINKS_WITH): $(@D)

$(DEPENDS_ON): $(@D)

$(SUBDIR):
	@ echo ---- Subdir $(SRCDIR)$@
	$(MAKE) $(MAKEARGS) -C $@ $(MAKECMDGOALS) SRCDIR=$(SRCDIR)$@

$(SRC):

$(MAIN_SRC):

prebuild:
ifneq "$(PREBUILD)" ""
	@echo "*** Start prebuild step ***"
	$(PREBUILD)
	@echo "*** Finish prebuild step ***"
endif

postbuild:
ifneq "$(POSTBUILD)" ""
	@echo "*** Start postbuild step ***"
	$(POSTBUILD)
	@echo "*** Finish postbuild step ***"
endif

$(DEPEND_FILE): $(DEPEND) $(LOCAL_MAKEFILE)
ifneq "$(TARGET_TYPE)" "MICROCODE"
ifneq "$(SRC) $(MAIN_SRC)" " " 
	$(DEPEND_COMMAND) | sed 's/:/: $(LOCAL_MAKEFILE)/' > $@
endif
endif

ifneq ($(findstring clean, $(MAKECMDGOALS)) ,clean)
-include $(DEPEND_FILE)
endif

totalclean: 
	$(MAKE_CLEAN)
	$(RM) $(OBJ) $(MAIN_OBJ) $(TARGET) $(DEPEND_FILE) $(OTHER_CLEAN) *~ 

clean: 
	$(RM) $(OBJ) $(MAIN_OBJ) $(TARGET) $(DEPEND_FILE) *~ 

%.o: %.S
	$(CC) -x assembler-with-cpp -c -D__ASSEMBLY__ -o $@ $<
