#!/bin/sh
[ -r Makefile ] && make distclean
rm -rf config config.log config.status aclocal.m4 configure m4 Makefile.in autom4te.cache
rm -f */Makefile.in tests/Makefile.in
rm -rf doxygen

