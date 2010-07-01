This is GRAPES, the Generic Resource-Aware P2P Environment for Streaming

Quick-start Documentation for Developers (aka README.txt)

1. This version is intended for developers only.

PREREQUISITES
-------------

Developments versions of 
- libevent2 (http://monkey.org/~provos/libevent/ - note we use version 2.0, generally referred to as libevent2)
- libconfuse (http://www.nongnu.org/confuse/) 
need to be installed. These libraries should be available for any reasonable Linux distribution (e.g. as RPM packages).

DEVELOPMENT
-----------

The code structure follows the usual autoconf/automake hierarchy: 
 - main autoconf file is configure.ac (should be modified with a strong reason
   only e.g. adding a new directory to the structure).
 - Makefile templates are Makefile.am and */Makefile.am. See existing examples if you need to create a new one.
   Good examples are rep/Makefile.am (contains both an intermediate library,
   librep.a, and an executable repoclient (compiled from repoclient.c)
   Makefile.am and configure.ac needs to be modified only if a new directory is added to the code base.

For adding files and dependencies, edit Makefile.am(s), there is one in each source directory.
After changing any Makefile.am or configure.am, re-run:
./autogen.sh 
then
./configure
and 
make

For deleting all auto-generated files use:
./autoclean.sh

COMPILATION & DOCUMENT GENERATION
---------------------------------

./configure
make

If libevent or libconfuse are installed in a non-standard location (i.e. not in /usr or /usr/local) then use
./configure --with-lib[confuse|event2]=_dir_location

To generate HTML doc by doxygen into directory doxygen/html, use:
make doxygen


