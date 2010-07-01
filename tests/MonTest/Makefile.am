#TODO: change this:
ML = $(top_srcdir)/ml

INCLUDES = -I$(top_srcdir)/dclog -I$(top_srcdir)/include/ -I$(ML)/include/
METASOURCES = AUTO
bin_PROGRAMS = MonTest
MonTest_SOURCES = MonTest.c
LDADD = $(top_builddir)/dclog/libdclog.a $(top_builddir)/common/libcommon.a $(top_builddir)/ml/libml.a $(top_builddir)/rep/librep.a $(top_builddir)/monl/libmon.a -lm -lrt -lstdc++ 
