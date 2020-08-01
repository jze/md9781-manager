#ifndef libmd9781_h
#define libmd9781_h

#include <usb.h>

#define MD9781_INTERN   'M'
#define MD9781_EXTERN   'S'


#define INFO_FILE_ENTRY_LENGTH   sizeof( struct info_file_entry)

extern struct usb_device *md9781_dev;

struct file_descriptor {
    unsigned char name[13];
    long size;
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
};

struct info_file_entry {
   char*   name;
   long    size;
};

usb_dev_handle*  md9781_open();
int md9781_close( usb_dev_handle* dh );
struct file_descriptor ** md9781_file_list( usb_dev_handle* dh, char location );
struct info_file_entry**  md9781_play_list( usb_dev_handle* dh, char location );
int md9781_delete_file( usb_dev_handle* dh, int file_number, char location  );
int md9781_download_file( usb_dev_handle* dh, const char* filename, int nr, char location  );
int md9781_upload_file( usb_dev_handle* dh, 
                        const char* filename,  
			char location  );
int md9781_rebuild_infofile( usb_dev_handle* dh, char location  );
int md9781_upload_playlist( usb_dev_handle* dh, 
                            char location,
  			    struct info_file_entry** playlist ); 

#endif
