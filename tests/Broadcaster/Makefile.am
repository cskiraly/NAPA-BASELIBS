INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/dclog

bin_PROGRAMS = peer
peer_SOURCES = peer.c ml.c mon.c rep.c som.c source.c playout.c version.c crc.c

#peer_LDFLAGS="-static"

LDADD = $(top_builddir)/common/libcommon.a $(SOM)/libgrapes.a $(top_builddir)/rep/librep.a  $(top_builddir)/ml/libml.a $(top_builddir)/monl/libmon.a $(top_builddir)/dclog/libdclog.a $(top_builddir)/ul/libul.a -lm -lstdc++

