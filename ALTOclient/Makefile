XMLSRC ?= $(LIBXML2_DIR)/include/libxml2

CPPFLAGS += -I$(XMLSRC)
LDLIBS += -lxml2 -lpthread

# libcurl is now obsolete
#LDLIBS += -lcurl

all: main lib

lib: ALTOclient.o
		@$(AR) r libALTO.a ALTOclient.o

main: ALTOclient.o

clean:
		rm -f *.o
		rm -f *.a
		rm -f main

distclean: clean
