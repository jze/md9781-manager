#ifndef libmd9781_h
#define libmd9781_h

#include <usb.h>

#define MD9781_INTERN   'M'
#define MD9781_EXTERN   'S'

#define MD9781_NO_FILE_ON_PLAYER	(md9781_entry*)1
#define MD9781_VERSION  "0.3.1"

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
    int fnumber;
}
md9781_entry;


usb_dev_handle*  md9781_open();
int md9781_close( usb_dev_handle* dh );

int md9781_dummy_read( usb_dev_handle *dh );

md9781_entry* md9781_file_list( usb_dev_handle* dh,
                                char location );
				
void md9781_freemem_filelist( md9781_entry* files );

int md9781_delete_file( usb_dev_handle* dh,
                        int file_number,
                        char location,
                        md9781_entry* playlist );

int md9781_download_file( usb_dev_handle* dh,
                          const char* filename,
                          int nr,
                          char location,
                          md9781_entry* playlist,
                          void (*callback)(int percent_done, float speed)  );

int md9781_upload_file( usb_dev_handle* dh,
                        const char* filename,
                        char location,
                        md9781_entry* playlist,
                        void (*callback)(int percent_done, float speed) );


int md9781_number_of_files( md9781_entry* playlist );

/* returns size of SmartMedia card or internal flash in KB
 * a flash card signed "64 MB" actually has only 64000 KB */
int md9781_flash_size( usb_dev_handle* dh, char location);

/* round size up to player filesystem block size */
int md9781_round_to_bs(int size);

/* calculate amount of free kb, respect overhead by fat and use of blocks
 * the player uses block size 16kb - a file of 1 byte will need 16kb */
int md9781_freesize_kb( int memsize_kb, md9781_entry* playlist);

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

int md9781_delete_range( usb_dev_handle* dh, char *range, char location,
                         md9781_entry* playlist );

int md9781_download_range(usb_dev_handle* dh,
                          char *range,
                          char location,
                          md9781_entry* playlist,
                          void (*callback)(int percent_done, float speed)  );
			  
int md9781_format( usb_dev_handle* dh, char location  );

char*  md9781_get_version();
			  
#endif
