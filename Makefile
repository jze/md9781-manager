CC=gcc
CFLAGS=-g -O2 -Wall
LIBS=-lusb

OBJECTS =  common.o listfiles.o delete.o download.o upload.o playlist.o format.o

.PHONY:   clean all

all: md9781-manager 

md9781-manager: md9781-manager.o libmd9781.so
	$(CC) -o md9781-manager md9781-manager.o -L. -lmd9781
	
libmd9781.so:  $(OBJECTS)
	$(CC) -o libmd9781.so -shared $(LIBS) $(OBJECTS)
	
clean:
	rm -f $(OBJECTS)
	rm -f *~
	rm -f *.orig
	rm -f md9781-manager 

tidy:	clean
	rm config.h config.log config.status Makefile

%.o: %.c
	$(CC) -c $(CFLAGS) $<

install:  md9781-manager 
	cp md9781-manager /usr/local/bin
