#include "common.h"
#include "libmd9781.h"

int md9781_download_file( usb_dev_handle* dh,
                          const char* filename,
                          int nr,
                          char location,
                          md9781_entry* playlist,
			  void (*callback)(int percent_done) ) {
    unsigned char send_buffer[256];
    int retval;
    FILE* file = fopen( filename, "w");
    long filesize;
    int chunks,i,last_value;

    if( location != 'M' && location != 'S' )
        return 0;

    /* get list of files */
    playlist = md9781_file_list(dh, location);

    for( i = 0; i < nr; i++ ) {
        playlist = playlist->next;
    }
    filesize = playlist->size - 16;
    chunks = filesize / 512 + 1;

    #ifdef DEBUG
    printf("Exspecting a %ld byte (%d packets) long file.\n", filesize, chunks);
    #endif

    /* prepare the send_buffer */
    memset(send_buffer, 0, 256);

    /* the command */
    send_buffer[0] = '#';
    send_buffer[1] = 0x00;
    send_buffer[2] = 0x18;
    send_buffer[3] = ':';
    send_buffer[4] = location;
    send_buffer[5] = 'R';
    send_buffer[6] = nr;
    send_buffer[7] = 0x2e;

    #ifdef DEBUG
    printf("start sending\n");
    #endif
    md9781_bulk_write(dh, send_buffer, 256);

    #ifdef DEBUG
    printf("command sent \n");
    #endif

    dummy_write(dh);
    dummy_read(dh);

    /** needs some work - you must know the filesize and hence the number of
    packages you can read */

    dummy_read(dh);
    dummy_read(dh);
    dummy_read(dh);


    /* read the file */
    last_value = 0;
    for(  i = 0; i < chunks; i++ ) {
        int length = 512;
        unsigned char buffer[512];

        memset( buffer, 0, 512);
        retval = md9781_bulk_read(dh, buffer, 512);
	
        if( (i+1) * 512 > filesize )
            length = filesize % 512;
	    
        fwrite( buffer, 1, length, file );

	if( callback != NULL ) {
	    int percent_done = (i / (double)chunks) * 100;
	    if( percent_done != last_value ) {
       	       callback( percent_done );
	    }
	    last_value = percent_done;
	}
    }
    fclose(file);
    return 1;
}

