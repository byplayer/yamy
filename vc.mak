############################################################## -*- Makefile -*-
#
# Makefile (Visual C++)
#
#	make release version: nmake nodebug=1
#	make debug version: nmake
#
###############################################################################


# VC++ rules	###############################################################

!if "$(TARGETOS)" == ""
TARGETOS	= WINNT
!endif	# TARGETOS

!if "$(TARGETOS)" == "WINNT"
APPVER		= 5.0
!ifdef nodebug
OUT_DIR		= out$(MAYU_VC)_winnt
!else	# nodebug
OUT_DIR		= out$(MAYU_VC)_winnt_debug
!endif	# nodebug
!endif	# TARGETOS

!if "$(TARGETOS)" == "WIN95"
APPVER		= 4.0
!ifdef nodebug
OUT_DIR		= out$(MAYU_VC)_win9x
!else	# nodebug
OUT_DIR		= out$(MAYU_VC)_win9x_debug
!endif	# nodebug
!endif	# TARGETOS

!if "$(TARGETOS)" == "BOTH"
!error Must specify TARGETOS=WIN95 or TARGETOS=WINNT
!endif	# TARGETOS

#_WIN32_IE	= 0x0500
!include <win32.mak>
#NMAKE_WINVER	= 0x0500	# trick for WS_EX_LAYERED

!ifdef nodebug
DEBUG_FLAG	= $(cdebug)
!else	# nodebug
DEBUG_FLAG	=
!endif	# nodebug

{}.cpp{$(OUT_DIR)}.obj:
	$(cc) -GX $(cflags) $(cvarsmt) $(DEFINES) $(INCLUDES) \
		$(DEBUG_FLAG) -Fo$@ $(*B).cpp
{}.rc{$(OUT_DIR)}.res:
	$(rc) $(rcflags) $(rcvars) /fo$@ $(*B).rc

conxlibsmt	= $(conlibsmt) libcpmt.lib libcmt.lib
guixlibsmt	= $(guilibsmt) libcpmt.lib libcmt.lib

DEPENDFLAGS	= --cpp=vc --ignore='$(INCLUDE)' -p"$$(OUT_DIR)\\"	\
		--path-delimiter=dos --newline=unix			\
		$(DEPENDIGNORE) -GX $(cflags) $(cvarsmt)	\
		$(DEFINES) $(INCLUDES) $(DEBUG_FLAG)

CLEAN		= $(OUT_DIR)\*.pdb


# tools		###############################################################

RM		= del
COPY		= copy
ECHO		= echo
MKDIR		= mkdir
RMDIR		= rmdir
