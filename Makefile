prefix= /usr/local
exec_prefix=${prefix}

.PHONY:   clean all

all: 
	cd lib && make all 
	cd md9781-manager && make all

clean:
	cd lib && make clean
	cd md9781-manager && make clean

tidy:	clean
	cd lib && make tidy
	cd md9781-manager && make tidy
	rm -rf autom4te.cache
	rm -fr debian/tmp
	rm -f config.h config.log config.status Makefile 

install:
	mkdir -p ${exec_prefix}/lib
	mkdir -p ${exec_prefix}/bin
	mkdir -p ${prefix}/include
	mkdir -p ${prefix}/man
	mkdir -p ${prefix}/man/man1
	mkdir -p /usr/local/etc/hotplug/usb/
	cd lib && make install
	cd md9781-manager && make install
	cp usb-hotplug/* /usr/local/etc/hotplug/usb/
