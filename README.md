# Using the MD9781 MP3-player with Linux

In December 2002 a large German trade chain sold this portable MP3-player for
a very good price. Since I use Linux I looked for a driver to upload and
download files to the player. But the device uses a vendor specific protocol
so none of the existing drivers worked. So I decided to write my own...

Medion sells the player with 64MB internal memory by the name "MD-9781
Tragbarer MP3-Player-/Recorder". PoGo Products sells the device by the name
"RipFlash PLUS" http://www.pogoproducts.com/ripflash_plus.html Their player
has more memory (128MB) but the Medion player costs only half. :-)

The USB vendor ID of the device is `0x4e8` that means the chip is from Samsung.
The product ID is `0x1003`.

I have analysed the protocol used for communication with the player and wrote
my own program "md9781-manager" that implements the most important parts of
the protocol:

- list the files on the player
- delete a file from the player
- upload a file to the player
- download a file from the player - btw: the manual says that's impossible... ;-)
  
## Installation

To use the md9781-manager you need a kernel with USB support configured. A
simple check (with the device plugged in):

```bash
grep Vendor /proc/bus/usb/devices
```

You should see a line that begins with `Vendor=04e8.`

You also need the libusb shared library. Most distributions already include
it. If you do not have it it can be found at 
http://libusb.sourceforge.net/download.html 

If everything works installing the program should be as easy as:

```bash
git clone https://github.com/jze/md9781-manager.git
cd md9781-manager/
./configure
make
make install   # (optional - this copies the md9781-manager binary to /usr/local/bin/ )
```

The access rights for the usb device file (`/prc/bus/usb/001/xxx`) are a little
bit tricky. Usually only root is allowed to write to the file. There are
several ways to solve this problem:

- run md9781-manager as root - bad
- have the line `usbdevfs /proc/bus/usb usbdevfs devmode=0666 0 0` in `/etc/fstab` - also bad (now every user on the system can write to any file in the proc usb tree
- try this little skript and let the hotplug daemon do the work

```bash
#!/bin/sh
#
# /etc/hotplug/usb/md9781
#
chgrp users $DEVICE
chmod g+w $DEVICE
```

You will have to add the line

```
md9781 0x0003 0x04e8 0x1003 0x0000 0x0000 0x00 0x00 0x00 0x00 0x00 0x00 0x00000000
```

to `/etc/hotplub/usb.usermap` to tell the hotplug daemon to call the skript (don't forget to make it executeable).
  
## Usage

Now give it a shot: `md9781-manager -l` You should see a list of all files in  the players internal memory.
 
Some examples how to use md9781-manager:  
 
The "-e" option tells the program to use the external smartmedia card of the
player. List of all files on the card: `md9781-manager -e -d`

To delete the second file from the player: `md9781-manager -d 2`

Delete the third track from the smartmedia card: `md9781-manager -e -d 3`

Put a song to the player: `md9781-manager -p my_song.mp3`. The player uses a
FAT file system so the filename will be truncated after the first 8
characters. If you want the file on the player of have a different name (i.e.
to avoid ambiguous names) use `md9781-manager -p my_song.mp3 other`.

To get the first file from the player: md9781-manager -g 1 . This will save
the file in the current directory. It will be called like it was on the
player.
  
Dietrich Rothe (dietrich@d-rothe.de) send me a patch that make the usage of the program easier:
  
1. with `-p` you can upload multiple files 
2. `-d` and `-g` accept a range-list with multiple numbers: for example
   
```
-d -3,4,20-40,49- deletes files 1,2,3, file #4, 20 to 40, and all files having a number greater than (including) 49 
```

The newest version of this text can be found at:  https://github.com/jze/md9781-manager
