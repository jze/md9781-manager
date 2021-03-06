#ifndef md9781_common_h
#define md9781_common_h
#include "config.h"

#include <usb.h>
#include "libmd9781.h"

#define MD9781_SUCCESS                0
#define MD9781_ERROR                 -1
#define MD9781_DEVICE_NOT_FOUND      -1000
#define MD9781_NOT_ENOUGH_SPACE      -1010

#define USB_BLOCK_SIZE          256
#define USB_SHORT_TIMEOUT       5000
#define USB_LONG_TIMEOUT        60000

#define MD9781_INFO_FILE 	0
#define MAX_INFO_FILE_SIZE	70000

extern struct usb_device *md9781_dev;

typedef struct _md9781_playlist_entry {
    char* name;
    long  size;
    int   used;
    struct _md9781_playlist_entry*  next;
}
md9781_playlist_entry;

extern int use_info_file;
/* sizes in KB */
extern int internal_size;
extern int smc_size;

void debug_message(unsigned char *hdr, unsigned char *txt);
void error_message(unsigned char *hdr, unsigned char *txt);

usb_dev_handle*  md9781_open();

int dummy_write( usb_dev_handle *dh );
int dummy_read( usb_dev_handle *dh );

void dump_buffer(  const unsigned char* buffer, int size, const char* );

int md9781_bulk_write( usb_dev_handle* dh, char* buffer, int size );

int md9781_bulk_read( usb_dev_handle* dh, char* buffer, int size );
int md9781_bulk_read_with_timeout( usb_dev_handle* dh, char* buffer, int size, long timeout );

int md9781_upload_file_from_buffer( usb_dev_handle* dh,char location,
                                    char* filename, unsigned char* file_buffer, int length );

int md9781_upload_playlist(usb_dev_handle* dh,
                           char location,
                           md9781_entry* playlist );


md9781_playlist_entry* md9781_play_list( usb_dev_handle* dh, char location, const md9781_entry* playlist_file );

void md9781_freemem_playlist( md9781_playlist_entry* playlist );

typedef struct {
    usb_dev_handle* dh;
    char location;
    md9781_entry* playlist;
    void (*callback) (int percent_done, float speed);
}
Passed_args;

int exec_on_range(int low, int high, char *range, int (*fp)(int n, void *arg), void *arg);

double getsec();

#endif
