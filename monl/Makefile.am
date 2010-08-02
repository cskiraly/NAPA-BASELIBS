#TODO: change this:
ML = $(top_srcdir)/ml

INCLUDES = -I$(top_srcdir)/include/ -I$(top_srcdir)/dclog -I$(ML)/include/
noinst_LIBRARIES = libmon.a
#libmon_a_SOURCES = measure_dispatcher.cpp  measure_manager.cpp  monl.cpp
#noinst_LTLIBRARIES = libmon.la
#libmon_la_SOURCES = measure_dispatcher.cpp  measure_manager.cpp  monl.cpp
#noinst_PROGRAMS = libmon.so
libmon_a_SOURCES = plugins/bulktransfer_measure.cpp measure_dispatcher.cpp \
	measure_manager.cpp mon_event.cpp mon_measure.cpp monl.cpp \
	plugins/byte_measure.cpp plugins/capprobe_measure.cpp plugins/clockdrift_measure.cpp \
	plugins/corrected_delay_measure.cpp plugins/packet_measure.cpp plugins/forecaster_measure.cpp \
	plugins/generic_measure.cpp plugins/hopcount_measure.cpp plugins/loss_burst_measure.cpp \
	plugins/loss_measure.cpp plugins/rtt_measure.cpp plugins/seqwin_measure.cpp result_buffer.cpp
#libmon_so_LDFLAGS = -shared
LDADD = $(top_builddir)/dclog/libdclog.a $(top_builddir)/common/libcommon.a $(top_builddir)/ml/libml.a $(top_builddir)/rep/librep.a
noinst_HEADERS = ctrl_msg.h result_buffer.h stat_types.h
