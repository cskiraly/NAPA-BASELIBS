INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/dclog

bin_PROGRAMS = neighborlist
repoclient SOURCES = neighborlist.c
LDADD = $(top_builddir)/rep/librep.a $(top_builddir)/dclog/libdclog.a $(top_builddir)/common/libcommon.a  $(top_builddir)/ml/libml.a -lm

