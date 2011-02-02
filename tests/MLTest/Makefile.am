INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/ml -I$(top_srcdir)/ml/include -I$(top_srcdir)/dclog

bin_PROGRAMS = MLTest
MLTest_SOURCES = MLTest.c

MLTest_LDADD = $(top_builddir)/common/libcommon.a $(top_builddir)/ml/libml.a $(top_builddir)/dclog/libdclog.a -lm
