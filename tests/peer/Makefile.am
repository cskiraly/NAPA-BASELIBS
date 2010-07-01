INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/dclog

bin_PROGRAMS = peer
peer_SOURCES = peer.c init.c ul/source.c 

#peer_LDFLAGS="-static"

LDADD = $(top_builddir)/common/libcommon.a $(top_builddir)/rep/librep.a $(top_builddir)/ml/libml.a $(top_builddir)/monl/libmon.a $(top_builddir)/dclog/libdclog.a -lm -lstdc++

