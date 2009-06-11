############################################################## -*- Makefile -*-
#
# Makefile for setup
#
###############################################################################

F	=	setup-vc.mak

all:
	@echo "============================================================================="
	@echo "Visual C++ 6.0: nmake"
	@echo "Borland C++ 5.5: make F=setup-bcc.mak (NOT IMPREMENTED YET)"
	@echo "============================================================================="
	$(MAKE) -f $(F) $(MAKEFLAGS) batch

clean:
	$(MAKE) -f $(F) $(MAKEFLAGS) batch_clean

distrib:
	$(MAKE) -f $(F) $(MAKEFLAGS) batch_distrib

depend:
	$(MAKE) -f $(F) $(MAKEFLAGS) TARGETOS=WINNT depend
