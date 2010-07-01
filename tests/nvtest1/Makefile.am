#TODO: change this:
ML = $(top_srcdir)/ml

INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/dclog -I$(ML)/include/

bin_PROGRAMS = nvtest1
nvtest1_SOURCES = nvtest1.c chunk.c

LDADD = $(top_builddir)/common/libcommon.a $(top_builddir)/rep/librep.a $(top_builddir)/ml/libml.a $(top_builddir)/monl/libmon.a $(top_builddir)/dclog/libdclog.a -lm -lstdc++
