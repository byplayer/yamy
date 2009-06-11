############################################################## -*- Makefile -*-
#
# Makefile for mayu
#
###############################################################################


!if "$(VERSION)" == ""
#VERSION		= 3.31
VERSION		= snapshot20050612
!endif

!if "$(TARGETOS)" == "WINNT"
OS_SPECIFIC_DEFINES	=  -DUNICODE -D_UNICODE
DISTRIB_OS	= xp
!endif

!if "$(TARGETOS)" == "WIN95"
OS_SPECIFIC_DEFINES	=  -D_MBCS
DISTRIB_OS	= 9x
!endif

!if "$(TARGETOS)" == "BOTH"
!error You must specify TARGETOS=WINNT
!endif


COMMON_DEFINES	= -DSTRICT -D_WIN32_IE=0x0500 $(OS_SPECIFIC_DEFINES)
BOOST_DIR	= ../boost_$(BOOST_VER)_0


# mayu.exe	###############################################################

TARGET_1	= $(OUT_DIR)\mayu.exe
OBJS_1		=					\
		$(OUT_DIR)\compiler_specific_func.obj	\
		$(OUT_DIR)\dlgeditsetting.obj		\
		$(OUT_DIR)\dlginvestigate.obj		\
		$(OUT_DIR)\dlglog.obj			\
		$(OUT_DIR)\dlgsetting.obj		\
		$(OUT_DIR)\dlgversion.obj		\
		$(OUT_DIR)\engine.obj			\
		$(OUT_DIR)\focus.obj			\
		$(OUT_DIR)\function.obj			\
		$(OUT_DIR)\keyboard.obj			\
		$(OUT_DIR)\keymap.obj			\
		$(OUT_DIR)\layoutmanager.obj		\
		$(OUT_DIR)\mayu.obj			\
		$(OUT_DIR)\parser.obj			\
		$(OUT_DIR)\registry.obj			\
		$(OUT_DIR)\setting.obj			\
		$(OUT_DIR)\stringtool.obj		\
		$(OUT_DIR)\target.obj			\
		$(OUT_DIR)\vkeytable.obj		\
		$(OUT_DIR)\windowstool.obj		\

SRCS_1		=				\
		compiler_specific_func.cpp	\
		dlgeditsetting.cpp		\
		dlginvestigate.cpp		\
		dlglog.cpp			\
		dlgsetting.cpp			\
		dlgversion.cpp			\
		engine.cpp			\
		focus.cpp			\
		function.cpp			\
		keyboard.cpp			\
		keymap.cpp			\
		layoutmanager.cpp		\
		mayu.cpp			\
		parser.cpp			\
		registry.cpp			\
		setting.cpp			\
		stringtool.cpp			\
		target.cpp			\
		vkeytable.cpp			\
		windowstool.cpp			\

RES_1		= $(OUT_DIR)\mayu.res

LIBS_1		=			\
		$(guixlibsmt)		\
		shell32.lib		\
		comctl32.lib		\
		wtsapi32.lib		\
		$(OUT_DIR)\mayu.lib	\

EXTRADEP_1	= $(OUT_DIR)\mayu.lib

# mayu.dll	###############################################################

TARGET_2	= $(OUT_DIR)\mayu.dll
OBJS_2		= $(OUT_DIR)\hook.obj $(OUT_DIR)\stringtool.obj
SRCS_2		= hook.cpp stringtool.cpp
LIBS_2		= $(guixlibsmt) imm32.lib


# mayu.lib	###############################################################

TARGET_3	= $(OUT_DIR)\mayu.lib
DLL_3		= $(OUT_DIR)\mayu.dll


# distribution	###############################################################

DISTRIB_SETTINGS =			\
		104.mayu		\
		104on109.mayu		\
		109.mayu		\
		109on104.mayu		\
		default.mayu		\
		emacsedit.mayu		\
		dot.mayu		\

DISTRIB_MANUAL	=				\
		doc\banner-ja.gif		\
		doc\CONTENTS-ja.html		\
		doc\CUSTOMIZE-ja.html		\
		doc\edit-setting-ja.png		\
		doc\investigate-ja.png		\
		doc\log-ja.png			\
		doc\MANUAL-ja.html		\
		doc\menu-ja.png			\
		doc\pause-ja.png		\
		doc\README-ja.html		\
		doc\README.css			\
		doc\setting-ja.png		\
		doc\syntax.txt			\
		doc\target.png			\
		doc\version-ja.png		\
		mayu-mode.el			\

DISTRIB_CONTRIBS =				\
		contrib\109onAX.mayu		\
		contrib\mayu-settings.txt	\
		contrib\dvorak.mayu		\
		contrib\dvorak109.mayu		\
		contrib\keitai.mayu		\
		contrib\ax.mayu			\
		contrib\98x1.mayu		\
		contrib\DVORAKon109.mayu	\

!if "$(TARGETOS)" == "WINNT"
DISTRIB_DRIVER	=				\
		d\i386\mayud.sys		\
		d\rescue\i386\mayudrsc.sys	\
		d\nt4\i386\mayudnt4.sys
!endif
!if "$(TARGETOS)" == "WIN95"
DISTRIB_DRIVER	= d_win9x\mayud.vxd
!endif

DISTRIB		=			\
		$(TARGET_1)		\
		$(TARGET_2)		\
		s\$(OUT_DIR)\setup.exe	\
		$(DISTRIB_SETTINGS)	\
		$(DISTRIB_MANUAL)	\
		$(DISTRIB_CONTRIBS)	\
		$(DISTRIB_DRIVER)	\


# tools		###############################################################

IEXPRESS	= iexpress
DOCXX		= doc++.exe
MAKEDEPEND	= perl tools/makedepend -o.obj
DOS2UNIX	= perl tools/dos2unix
UNIX2DOS	= perl tools/unix2dos
MAKEFUNC	= perl tools/makefunc
GETCVSFILES	= perl tools/getcvsfiles
GENIEXPRESS	= perl tools/geniexpress


# rules		###############################################################

all:		boost $(OUT_DIR) $(TARGET_1) $(TARGET_2) $(TARGET_3)

$(OUT_DIR):
		if not exist "$(OUT_DIR)\\" $(MKDIR) $(OUT_DIR)

functions.h:	engine.h tools/makefunc
		$(MAKEFUNC) < engine.h > functions.h

clean::
		-$(RM) $(TARGET_1) $(TARGET_2) $(TARGET_3)
		-$(RM) $(OUT_DIR)\*.obj
		-$(RM) $(OUT_DIR)\*.res $(OUT_DIR)\*.exp
		-$(RM) mayu.aps mayu.opt $(OUT_DIR)\*.pdb
		-$(RM) *~ $(CLEAN)
		-$(RMDIR) $(OUT_DIR)

depend::
		$(MAKEDEPEND) -fmayu-common.mak \
		-- $(DEPENDFLAGS) -- $(SRCS_1) $(SRCS_2)

distrib:
		-@echo "we need cygwin tool"
		-rm -f mayu-$(VERSION) 
		-ln -s . mayu-$(VERSION)
		-bash -c "tar cvjf mayu-$(VERSION)-src.tar.bz2 `$(GETCVSFILES) | sed 's/^./mayu-$(VERSION)/'`"
		-rm -f mayu-$(VERSION) 
		-$(GENIEXPRESS) \
			mayu-$(VERSION)-$(DISTRIB_OS).exe \
			"MADO TSUKAI NO YUUTSU $(VERSION) $(TARGETOS)" \
			setup.exe $(DISTRIB) > __mayu__.sed
		-$(UNIX2DOS) $(DISTRIB_SETTINGS) $(DISTRIB_CONTRIBS)
		-$(IEXPRESS) /N __mayu__.sed
		-$(DOS2UNIX) $(DISTRIB_SETTINGS) $(DISTRIB_CONTRIBS)
		-$(RM) __mayu__.sed

srcdesc::
		@$(ECHO) USE DOC++ 3.4.4 OR HIGHER
		$(DOCXX) *.h

# DO NOT DELETE

$(OUT_DIR)\compiler_specific_func.obj: compiler_specific.h \
 compiler_specific_func.h misc.h stringtool.h
$(OUT_DIR)\dlgeditsetting.obj: compiler_specific.h dlgeditsetting.h \
 layoutmanager.h mayurc.h misc.h stringtool.h windowstool.h
$(OUT_DIR)\dlginvestigate.obj: compiler_specific.h d\ioctl.h \
 dlginvestigate.h driver.h engine.h focus.h function.h functions.h hook.h \
 keyboard.h keymap.h mayurc.h misc.h msgstream.h multithread.h parser.h \
 setting.h stringtool.h target.h vkeytable.h windowstool.h
$(OUT_DIR)\dlglog.obj: compiler_specific.h dlglog.h layoutmanager.h mayu.h \
 mayurc.h misc.h msgstream.h multithread.h registry.h stringtool.h \
 windowstool.h
$(OUT_DIR)\dlgsetting.obj: compiler_specific.h d\ioctl.h dlgeditsetting.h \
 driver.h function.h functions.h keyboard.h keymap.h layoutmanager.h mayu.h \
 mayurc.h misc.h multithread.h parser.h registry.h setting.h stringtool.h \
 windowstool.h
$(OUT_DIR)\dlgversion.obj: compiler_specific.h compiler_specific_func.h \
 layoutmanager.h mayu.h mayurc.h misc.h stringtool.h windowstool.h
$(OUT_DIR)\engine.obj: compiler_specific.h d\ioctl.h driver.h engine.h \
 errormessage.h function.h functions.h hook.h keyboard.h keymap.h mayurc.h \
 misc.h msgstream.h multithread.h parser.h setting.h stringtool.h \
 windowstool.h
$(OUT_DIR)\focus.obj: compiler_specific.h focus.h misc.h stringtool.h \
 windowstool.h
$(OUT_DIR)\function.obj: compiler_specific.h d\ioctl.h driver.h engine.h \
 function.h functions.h hook.h keyboard.h keymap.h mayu.h mayurc.h misc.h \
 msgstream.h multithread.h parser.h registry.h setting.h stringtool.h \
 vkeytable.h windowstool.h
$(OUT_DIR)\keyboard.obj: compiler_specific.h d\ioctl.h driver.h keyboard.h \
 misc.h stringtool.h
$(OUT_DIR)\keymap.obj: compiler_specific.h d\ioctl.h driver.h \
 errormessage.h function.h functions.h keyboard.h keymap.h misc.h \
 multithread.h parser.h setting.h stringtool.h
$(OUT_DIR)\layoutmanager.obj: compiler_specific.h layoutmanager.h misc.h \
 stringtool.h windowstool.h
$(OUT_DIR)\mayu.obj: compiler_specific.h compiler_specific_func.h d\ioctl.h \
 dlginvestigate.h dlglog.h dlgsetting.h dlgversion.h driver.h engine.h \
 errormessage.h focus.h function.h functions.h hook.h keyboard.h keymap.h \
 mayu.h mayuipc.h mayurc.h misc.h msgstream.h multithread.h parser.h \
 registry.h setting.h stringtool.h target.h windowstool.h
$(OUT_DIR)\parser.obj: compiler_specific.h errormessage.h misc.h parser.h \
 stringtool.h
$(OUT_DIR)\registry.obj: array.h compiler_specific.h misc.h registry.h \
 stringtool.h
$(OUT_DIR)\setting.obj: array.h compiler_specific.h d\ioctl.h dlgsetting.h \
 driver.h errormessage.h function.h functions.h keyboard.h keymap.h mayu.h \
 mayurc.h misc.h multithread.h parser.h registry.h setting.h stringtool.h \
 vkeytable.h windowstool.h
$(OUT_DIR)\stringtool.obj: array.h compiler_specific.h misc.h stringtool.h
$(OUT_DIR)\target.obj: compiler_specific.h mayurc.h misc.h stringtool.h \
 target.h windowstool.h
$(OUT_DIR)\vkeytable.obj: compiler_specific.h misc.h vkeytable.h
$(OUT_DIR)\windowstool.obj: array.h compiler_specific.h misc.h stringtool.h \
 windowstool.h
$(OUT_DIR)\hook.obj: compiler_specific.h hook.h misc.h stringtool.h
$(OUT_DIR)\stringtool.obj: array.h compiler_specific.h misc.h stringtool.h
