AUTOMAKE_OPTIONS = foreign no-dist no-installinfo
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = . common dclog rep monl  

#include aminclude.am

EXTRA_DIST = autogen.sh autoclean.sh Doxyfile

doxygen: FORCE
	mkdir -p $(srcdir)/doxygen
	doxygen $(srcdir)/Doxyfile

FORCE:

