#ifndef md9781_common_h
#define md9781_common_h

#include <usb.h>

#define MD9781_SUCCESS                0
#define MD9781_ERROR                 -1
#define MD9781_DEVICE_NOT_FOUND      -1000
#define MD9781_NOT_ENOUGH_SPACE      -1010

#define USB_BLOCK_SIZE          256
#define USB_SHORT_TIMEOUT       5000
#define USB_LONG_TIMEOUT        60000

#define MD9781_INFO_FILE 	0

extern struct usb_device *md9781_dev;

void debug_message(unsigned char *hdr, unsigned char *txt);
void error_message(unsigned char *hdr, unsigned char *txt);
usb_dev_handle*  md9781_open();
int dummy_write( usb_dev_handle *dh );
int dummy_read( usb_dev_handle *dh );
void dump_buffer(  const unsigned char* buffer, int size );
int md9781_bulk_write( usb_dev_handle* dh, char* buffer, int size );
int md9781_bulk_read( usb_dev_handle* dh, char* buffer, int size );
int md9781_upload_file_from_buffer( usb_dev_handle* dh,char location,
  char* filename, unsigned char* file_buffer, int length );

#endif
