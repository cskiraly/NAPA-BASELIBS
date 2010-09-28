AUTOMAKE_OPTIONS = foreign no-dist no-installinfo

#SUBDIRS = . tests

INCLUDES = -I$(top_srcdir)/include/  #-I$(top_srcdir)/dclog
noinst_LIBRARIES = libml.a

libml_a_SOURCES = BUGS.txt ml.c ml_log.c util/stun.c \
	util/udpSocket.c util/rateLimiter.c util/queueManagement.c
# transmissionHandler.c

# testMessaginglayer: echoServer.c ml.h ml.c transmissionHandler.h transmissionHandler.c util/stun.h util/stun.c util/udpSocket.h util/udpSocket.c
#        ${COMPILER} -o echoServer.o echoServer.c ml.h ml.c transmissionHandler.h transmissionHandler.c util/stun.h util/stun.c util/udpSocket.h util/udpSocket.c ${EVENT} ${MATH} ${GDB}

#libml_a_LIBADD = $(top_builddir)/dclog/libdclog.a
