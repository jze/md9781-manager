#ifndef libmd9781_h
#define libmd9781_h

#include <usb.h>

#define MD9781_INTERN   'M'
#define MD9781_EXTERN   'S'
#define MD9781_NO_FILE_ON_PLAYER	(md9781_entry*)1
#define MD9781_VERSION  "0.2.1"

extern struct usb_device *md9781_dev;

typedef struct _md9781_entry {
    unsigned char short_name[9];
    unsigned char extension[4];
    unsigned char* long_name;
    long size;
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
    struct _md9781_entry* next;
    struct _md9781_entry* prev;
} md9781_entry;


usb_dev_handle*  md9781_open();
int md9781_close( usb_dev_handle* dh );

md9781_entry* md9781_file_list( usb_dev_handle* dh, 
                                 char location );

int md9781_delete_file( usb_dev_handle* dh, 
                        int file_number, 
			char location,
			md9781_entry* playlist );

int md9781_download_file( usb_dev_handle* dh, 
                          const char* filename, 
			  int nr, 
			  char location,
			  md9781_entry* playlist,
			  void (*callback)(int percent_done)  );

int md9781_upload_file( usb_dev_handle* dh, 
                        const char* filename,  
			char location,
			md9781_entry* playlist,
			void (*callback)(int percent_done) );

int md9781_number_of_files( md9781_entry* playlist );

md9781_entry*  md9781_entry_number( md9781_entry* playlist, int nr );

/* If something really went wrong with the player's playlist this
 * procedure will regenerate it. 
 * You will loose all long filenames in the playlist!
 */
int md9781_regenerate_playlist( usb_dev_handle* dh, char location  );

/*
 * If by some 'accident' the info file got lost you can create a new one
 * with the help of this procedure
 */
int md9781_init_playlist( usb_dev_handle* dh, char location  );

/* After a call of this procedure the player's playlist will not be used
 * (read and write)
 */ 
void ignore_info_file();

#endif
