CC=gcc
CFLAGS=-g -O2 -Wall 
OBJECTS =  listfiles.o common.o  delete.o download.o upload.o playlist.o format.o
prefix=/usr/local
exec_prefix=${prefix}

.PHONY:   clean all

all: libmd9781.so

libmd9781.so:  $(OBJECTS)
	$(CC) -o libmd9781.so -shared $(LIBS) $(OBJECTS)
	
clean:
	rm -f $(OBJECTS)
	rm -f *~
	rm -f *.orig
	rm -f libmd9781.so

tidy:	clean
	rm -f Makefile 

%.o: %.c
	$(CC) -I.. -c $(CFLAGS) $<

install: libmd9781.so
	cp libmd9781.so ${exec_prefix}/lib
	cp libmd9781.h ${prefix}/include
