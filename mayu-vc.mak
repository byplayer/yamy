############################################################## -*- Makefile -*-
#
# Makefile for mayu (Visual C++)
#
#	make release version: nmake nodebug=1
#	make debug version: nmake
#
###############################################################################

!if "$(BOOST_VER)" == ""
BOOST_VER	= 1_38
!endif
INCLUDES	= -I$(BOOST_DIR)	# why here ?
DEPENDIGNORE	= --ignore=$(BOOST_DIR)

!if "$(MAYU_VC)" == ""
MAYU_VC	= vc9
!endif

!if ( "$(MAYU_VC)" == "vct" )
MAYU_REGEX_VC	= vc71
!else
MAYU_REGEX_VC	= $(MAYU_VC)
!endif

!include <vc.mak>
!include <mayu-common.mak>

DEFINES		= $(COMMON_DEFINES) -DVERSION=""""$(VERSION)"""" \
		-DLOGNAME=""""$(USERNAME)"""" \
		-DCOMPUTERNAME=""""$(COMPUTERNAME)"""" -D_CRT_SECURE_NO_WARNINGS -DMAYU64 -DNO_DRIVER -DUSE_MAILSLOT -DUSE_INI
# INCLUDES	= -I$(BOOST_DIR)	# make -f mayu-vc.mak depend fails ...

LDFLAGS_1	=						\
		$(guilflags)					\
		/PDB:$(TARGET_1).pdb				\
		/LIBPATH:$(BOOST_DIR)/libs/regex/build/$(MAYU_REGEX_VC)0	\

LDFLAGS_2	=						\
		$(dlllflags)					\
		/PDB:$(TARGET_2).pdb				\
		/LIBPATH:$(BOOST_DIR)/libs/regex/build/$(MAYU_REGEX_VC)0	\

LDFLAGS_4	=						\
		$(guilflags)					\
		/PDB:$(TARGET_4).pdb				\

LDFLAGS_5	=						\
		$(guilflags)					\
		/PDB:$(TARGET_5).pdb				\

$(TARGET_1):	$(OBJS_1) $(RES_1) $(EXTRADEP_1)
	$(link) -out:$@ $(ldebug) $(LDFLAGS_1) $(OBJS_1) $(LIBS_1) $(RES_1)

$(TARGET_2):	$(OBJS_2) $(RES_2) $(EXTRADEP_2)
	$(link) -out:$@ $(ldebug) $(LDFLAGS_2) $(OBJS_2) $(LIBS_2) $(RES_2)

$(TARGET_3):	$(DLL_3)

!if "$(MAYU_ARCH)" == "32"
$(TARGET_4):	$(OBJS_4) $(EXTRADEP_4)
	$(link) -out:$@ $(ldebug) $(LDFLAGS_4) $(OBJS_4) $(LIBS_4)

$(TARGET_5):	$(OBJS_5) $(EXTRADEP_5)
	$(link) -out:$@ $(ldebug) $(LDFLAGS_5) $(OBJS_5) $(LIBS_5) $(RES_5)
!endif

REGEXPP_XCFLAGS	= $(REGEXPP_XCFLAGS) XCFLAGS=-D_WCTYPE_INLINE_DEFINED

clean::
		-$(RM) mayu.aps mayu.opt *.pdb

boost:
		cd $(BOOST_DIR)/libs/regex/build/
		$(MAKE) -f $(MAYU_REGEX_VC).mak $(REGEXPP_XCFLAGS) main_dir libboost_regex-$(MAYU_REGEX_VC)0-mt-s-$(BOOST_VER)_dir ./$(MAYU_REGEX_VC)0/libboost_regex-$(MAYU_REGEX_VC)0-mt-s-$(BOOST_VER).lib libboost_regex-$(MAYU_REGEX_VC)0-mt-sgd-$(BOOST_VER)_dir ./$(MAYU_REGEX_VC)0/libboost_regex-$(MAYU_REGEX_VC)0-mt-sgd-$(BOOST_VER).lib
		cd ../../../../yamy

distclean::	clean
		cd $(BOOST_DIR)/libs/regex/build/
		-$(MAKE) -k -f $(MAYU_REGEX_VC).mak clean
		cd ../../../../yamy

batch:
!if "$(MAYU_VC)" != "vct"
		-$(MAKE) -f mayu-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT
!endif
		-$(MAKE) -f mayu-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT nodebug=1
#		cd s
#		-$(MAKE) -f setup-vc.mak MAYU_VC=$(MAYU_VC) batch
#		cd ..

batch_clean:
		-$(MAKE) -k -f mayu-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT nodebug=1 clean
		-$(MAKE) -k -f mayu-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT clean
		cd s
		-$(MAKE) -k -f setup-vc.mak MAYU_VC=$(MAYU_VC) batch_clean
		cd ..

batch_distclean: batch_clean
		-$(MAKE) -k -f mayu-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT distclean

batch_distrib: batch
		-$(MAKE) -k -f mayu-vc.mak MAYU_VC=$(MAYU_VC) TARGETOS=WINNT nodebug=1 distrib
