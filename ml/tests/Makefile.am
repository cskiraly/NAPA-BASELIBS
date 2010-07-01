INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/dclog -I$(top_srcdir)

bin_PROGRAMS = echoServer

echoServer_SOURCES = echoServer.c
echoServer_LDADD = $(top_builddir)/common/libcommon.a $(top_builddir)/ml/libml.a $(top_builddir)/dclog/libdclog.a -lm
