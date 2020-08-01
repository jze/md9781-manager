#include "common.h"
#include "libmd9781.h"
#include <stdio.h>
#include <string.h> 

struct file_descriptor * convert_file_entry( const unsigned char* buffer) {
    struct file_descriptor * file = (struct file_descriptor *)malloc(sizeof(struct file_descriptor));
    unsigned int date;
    unsigned int time;

    memcpy( file->name, buffer, 8 );
    file->name[8] = '.';
    memcpy( file->name + 9, buffer+8, 3 );
    file->name[12] = 0;

    file->size = (buffer[28] | buffer[29]<<8 |
                  buffer[30]<< 16 | buffer[31]<< 24 );

    date = buffer[25] |  buffer[24]<< 8;
    time = buffer[23] |  buffer[22]<< 8;
    file->year =  (date>>9)+1980;

    file->month =  (date>>5) & 15;
    file->day =date & 31;
    file->hour= time>>11;
    file->minute = (time>>5)&63;
    file->second = (time&31)<<1;
    return file;
}

struct file_descriptor ** md9781_file_list( usb_dev_handle* dh, char location ) {
    static unsigned char buffer[256];
    unsigned char send_buffer[256];
    struct file_descriptor ** files = malloc(256*sizeof(struct file_descriptor *));
    int i = 0;
    int file_nr = 0;
    
    #ifdef DEBUG
    printf("md9781_file_list\n");
    #endif    
    
    if( location != 'M' && location != 'S' )
        return 0;

    memset(send_buffer, 0, 256);
    send_buffer[0] = '#';
    send_buffer[1] = 0x00;
    send_buffer[2] = 0x04;
    send_buffer[3] = ':';
    send_buffer[4] = location;
    send_buffer[5] = 'G';
    send_buffer[6] = 0x2e;

    md9781_bulk_write(dh,  send_buffer, 256 );
    dummy_write( dh );

    for( i = 0; i < 34; i++ ) {
        memset(buffer, 0, USB_BLOCK_SIZE);
        md9781_bulk_read(dh,  buffer, 256);

        if( buffer[0] != 0 ) {
            int pos = 0;
            while( buffer[pos] != 0 ) {
                struct file_descriptor * file;
                file = convert_file_entry( buffer+pos );
                if( file->name[0] != 0xe5 ) {
                    files[file_nr] = file;
                    file_nr++;
                }
                pos = pos + 32;
            }
            files[file_nr] = NULL;
        }
    }

    return files;
}

void md9781_freemem_filelist( struct file_descriptor** files ) {
   int i = 0;
   if( files != NULL ) {
      while( files[i] != NULL ) {
         free(files[i]->name);
	 i++;
      }
      free( files );
   }
}
