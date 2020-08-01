CC          = /usr/bin/gcc
RM          = /bin/rm -f
CP          = /bin/cp -f

CPPFLAGS    = -DDEBUG
CPPFLAGS    = -g
CFLAGS      = -Wall
INCLUDES    = 
LDFLAGS     =  -lusb 
#LDFLAGS     = /usr/src/packages/BUILD/libusb-0.1.5/.libs/libusb.a
LIBRARIES   = 

TARGETS =   md9781-manager 
OBJECTS =  md9781-manager.o common.o listfiles.o delete.o download.o upload.o playlist.o

default: all

all: $(TARGETS)

md9781-manager: $(OBJECTS)
	$(CC) -o md9781-manager $(OBJECTS) $(LDFLAGS) $(LIBRARIES)
	
clean:
	rm -f $(OBJECTS)
	rm $(TARGETS)

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $<

install:  md9781-manager 
	cp md9781-manager /usr/local/bin
