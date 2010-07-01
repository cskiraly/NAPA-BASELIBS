XMLSRC ?= /usr/include/libxml2

CPPFLAGS += -I$(XMLSRC)
LDLIBS += -lxml2

LDLIBS += -lcurl

all: main lib

lib: ALTOclient.o
		@$(AR) r libALTO.a ALTOclient.o

main: ALTOclient.o

clean:
		rm -f *.o
		rm *.a
		rm main
