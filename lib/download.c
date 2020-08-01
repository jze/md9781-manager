#include "common.h"
#include "libmd9781.h"

int md9781_download_file( usb_dev_handle* dh,
                          const char* filename,
                          int nr,
                          char location,
                          md9781_entry* playlist,
                          void (*callback)(int percent_done, float speed) ) {
    unsigned char send_buffer[256];
    int retval;
    FILE* file = fopen( filename, "w");
    md9781_entry* playlist_mem = NULL;
    long filesize = 0;
    int chunks,i,last_value;
    const int chunksize = 2048;
    double starttime;
    unsigned char buffer[chunksize];
    int length = filesize % chunksize;

    if(file == NULL) {
      perror(filename);
      return 0;
    }
    if( location != 'M' && location != 'S' )
        return 0;

    /* get list of files */
    if(playlist == NULL) {
      playlist = playlist_mem = md9781_file_list(dh, location);
      if( ! playlist)
	return 0;
    }

    for( i = 0; i < nr; i++ ) {
        playlist = playlist->next;
    }
    filesize = playlist->size - 16;
    // filesize /= 3;              // §
    chunks = filesize / chunksize + 1;

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
    starttime = getsec();
    last_value = 0;
    /* read all complete chunks - stop before last incomplete chunk
       before end of file, to prevent read errors */
    for( i = 0; i < chunks-1; i++ ) {
      memset( buffer, 0, chunksize);
      
      retval = md9781_bulk_read(dh, buffer, chunksize);
      fwrite( buffer, 1, chunksize, file );
      
      if( callback != NULL ) {
	int percent_done = ( (i+1) / (double)chunks) * 100;
	if( percent_done != last_value ) {
	  callback( percent_done, (i+1)*chunksize/(getsec() - starttime) );
	}
	last_value = percent_done;
      }
    }
    /* now read the last (incomplete) chunk, using a smaller block size */
    if(length > 0) {
      const int small_block = 512;
      int write_length = small_block;
      /* find number of complete + incomplete small blocks */
      int n_blocks = length / small_block;
      if(length % small_block)
	n_blocks++;
	
      for( i = 0; i < n_blocks; i++) {
	if( (i+1) * small_block > length )
	  write_length = length % small_block;

	retval = md9781_bulk_read(dh, buffer, small_block);
	fwrite( buffer, 1, write_length, file);
      }
    }

    fclose(file);

    if(playlist_mem)
      md9781_freemem_filelist(playlist_mem);
    
    return 1;
}

static int fp_download(int nr, void *args ) {
    Passed_args *arg = args;

    /* get a file from the player (download) */
    if( nr > 0 ) {
        char *filename;
        md9781_entry* tentry = md9781_entry_number( arg->playlist, nr );

        filename = tentry->long_name;

        printf("Getting file #%d will call it '%s'...\n", nr, filename);
        if( md9781_download_file( arg->dh , filename, nr, arg->location,
                                  arg->playlist, arg->callback ) ) {
            printf("File #%d saved.\n", nr);
            return 1;
        }
    }
    return 0;
}


int md9781_download_range(usb_dev_handle* dh, char *range, char location,
                          md9781_entry* playlist, void (*callback)(int percent_done, float speed)  ) {
    Passed_args passdown_args = {
        dh, location, playlist, callback
    };

    return exec_on_range(1, md9781_number_of_files(playlist) - 1,
                         range, fp_download, &passdown_args);
}
