############################################################## -*- Makefile -*-
#
# Makefile for setup (Visual C++)
#
#	make release version: nmake nodebug=1
#	make debug version: nmake
#
###############################################################################


!if "$(BOOST_VER)" == ""
BOOST_VER	= 1_32
!endif
INCLUDES	= -I$(BOOST_DIR)	# why here ?
DEPENDIGNORE	= --ignore=$(BOOST_DIR)

!if "$(MAYU_VC)" == ""
MAYU_VC	= vc6
!endif

!if ( "$(MAYU_VC)" == "vct" )
MAYU_REGEX_VC	= vc71
!else
MAYU_REGEX_VC	= $(MAYU_VC)
!endif

!include <..\vc.mak>
!include <setup-common.mak>

LDFLAGS_1	=						\
		$(guilflags)					\
		/PDB:$(TARGET_1).pdb				\
		/LIBPATH:$(BOOST_DIR)/libs/regex/build/$(MAYU_REGEX_VC)	\

$(TARGET_1):	$(OBJS_1) $(RES_1) $(EXTRADEP_1)
	$(link) -out:$@ $(ldebug) $(LDFLAGS_1) $(OBJS_1) $(LIBS_1) $(RES_1)

strres.h:	setup.rc
	grep IDS setup.rc | \
	sed "s/\(IDS[a-zA-Z0-9_]*\)[^""]*\("".*\)$$/\1, _T(\2),/" | \
	sed "s/""""/"") _T(""/g" > strres.h

batch:
!if "$(MAYU_VC)" != "vct"
		-$(MAKE) -k -f setup-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT
!endif
		-$(MAKE) -k -f setup-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT nodebug=1

batch_clean:
		-$(MAKE) -k -f setup-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT nodebug=1 clean
		-$(MAKE) -k -f setup-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT clean
