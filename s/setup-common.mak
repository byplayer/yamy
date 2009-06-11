############################################################## -*- Makefile -*-
#
# Makefile for setup
#
###############################################################################


!if "$(TARGETOS)" == "WINNT"
OS_SPECIFIC_DEFINES	=  -DUNICODE -D_UNICODE
DISTRIB_OS	= nt
!endif

!if "$(TARGETOS)" == "WIN95"
OS_SPECIFIC_DEFINES	=  -D_MBCS
DISTRIB_OS	= 9x
!endif

!if "$(TARGETOS)" == "BOTH"
!error Must specify TARGETOS=WIN95 or TARGETOS=WINNT
!endif


DEFINES		= -DSTRICT -D_WIN32_IE=0x0400 $(OS_SPECIFIC_DEFINES) \
		  $(DEBUGDEFINES)
BOOST_DIR	= ../../boost_$(BOOST_VER)_0


# setup.exe	###############################################################

TARGET_1	= $(OUT_DIR)\setup.exe
OBJS_1		=				\
		$(OUT_DIR)\setup.obj		\
		$(OUT_DIR)\installer.obj	\
		..\$(OUT_DIR)\registry.obj	\
		..\$(OUT_DIR)\stringtool.obj	\
		..\$(OUT_DIR)\windowstool.obj	\

SRCS_1		=			\
		setup.cpp		\
		installer.cpp		\
		..\registry.cpp		\
		..\stringtool.cpp	\
		..\windowstool.cpp	\

RES_1		= $(OUT_DIR)\setup.res

LIBS_1		= $(guixlibsmt) shell32.lib ole32.lib uuid.lib


# tools		###############################################################

MAKEDEPEND	= perl ../tools/makedepend -o.obj


# rules		###############################################################

all:		$(OUT_DIR) $(TARGET_1)

$(OUT_DIR):
		if not exist "$(OUT_DIR)\\" $(MKDIR) $(OUT_DIR)

setup.cpp:	strres.h

clean:
		-$(RM) $(TARGET_1) strres.h
		-$(RM) $(OUT_DIR)\*.obj $(OUT_DIR)\*.res $(OUT_DIR)\*.pdb *.pdb
		-$(RM) *~ $(CLEAN)
		-$(RMDIR) $(OUT_DIR)

depend::
		$(MAKEDEPEND) -fsetup-common.mak \
		-- $(DEPENDFLAGS) -- $(SRCS_1)

# DO NOT DELETE

$(OUT_DIR)\setup.obj: ../compiler_specific.h ../mayu.h ../misc.h \
 ../registry.h ../stringtool.h ../windowstool.h installer.h setuprc.h \
 strres.h
$(OUT_DIR)\installer.obj: ../compiler_specific.h ../misc.h ../registry.h \
 ../stringtool.h ../windowstool.h installer.h
$(OUT_DIR)\..\registry.obj: ../compiler_specific.h ../misc.h \
 ../registry.cpp ../registry.h ../stringtool.h
$(OUT_DIR)\..\stringtool.obj: ../compiler_specific.h ../misc.h \
 ../stringtool.cpp ../stringtool.h
$(OUT_DIR)\..\windowstool.obj: ../compiler_specific.h ../misc.h \
 ../stringtool.h ../windowstool.cpp ../windowstool.h
