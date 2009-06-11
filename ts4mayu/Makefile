#
# Makefile for VC
#

!include <win32.mak>

!if "$(TS4MAYU)" == "STS4MAYU"
TARGET_NAME = sts4mayu
!elseif "$(TS4MAYU)" == "CTS4MAYU"
TARGET_NAME = cts4mayu
!else
TARGET_NAME = dummy
!endif

SRCS = ts4mayu.cpp
HEADERS = 
OBJS = $(TARGET_NAME)/ts4mayu.obj
DEFS = ts4mayu.def
TARGET = $(TARGET_NAME)/$(TARGET_NAME).dll

CFLAGS = -DUNICODE -D_UNICODE -D_MT -MT
INCLUDES = -I../../SynCOMAPIv1_0/Include -I"$(PROGRAMFILES)"/Touchpad
LDFLAGS =  $(dlllflags) /libpath:../../SynCOMAPIv1_0/Lib /libpath:"$(PROGRAMFILES)"/Touchpad
LDLIBS = user32.lib

RM = rd
MKDIR = md

all:
	$(MAKE) /$(MAKEFLAGS) TS4MAYU=STS4MAYU sts4mayu/sts4mayu.dll
	$(MAKE) /$(MAKEFLAGS) TS4MAYU=CTS4MAYU cts4mayu/cts4mayu.dll

$(TARGET_NAME):
	if not exist "$(TARGET_NAME)" $(MKDIR) $(TARGET_NAME)

$(TARGET): $(TARGET_NAME) $(OBJS) $(DEFS)
	$(link) $(LDFLAGS) $(OBJS) -def:$(DEFS) $(LDLIBS) -out:$@

{}.cpp.obj:
	$(cc) -D$(TS4MAYU) -GX $(cdebug) $(cflags) $(cvarsmt) $(DEFINES) $(INCLUDES) \
		$(DEBUG_FLAG) -Fo$@ $(*B).cpp

clean:
	-$(RM) /Q /S sts4mayu cts4mayu
