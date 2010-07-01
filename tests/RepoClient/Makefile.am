#TODO: change this:
ML = $(top_srcdir)/ml

INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/dclog -I$(ML)/include/

bin_PROGRAMS = RepoClient
RepoClient SOURCES = RepoClient.c
LDADD = $(top_builddir)/rep/librep.a $(top_builddir)/dclog/libdclog.a $(top_builddir)/common/libcommon.a  $(top_builddir)/ml/libml.a -lm

