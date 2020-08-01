#include "common.h"
#include "libmd9781.h"

md9781_entry* convert_file_entry( const unsigned char* buffer) {
    md9781_entry* file = (md9781_entry*)malloc(sizeof(md9781_entry));
    unsigned int date;
    unsigned int time;

    file->long_name = 0;
    memcpy( file->short_name, buffer, 8 );
    file->short_name[8] = 0;
    memcpy( file->extension, buffer+8, 3 );
    file->extension[3] = 0;

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

    file->next = NULL;
    file->prev = NULL;
    return file;
}

md9781_entry* md9781_file_list( usb_dev_handle* dh, char location ) {
    unsigned char buffer[256];
    md9781_entry* start = NULL;
    md9781_entry* entry = NULL;
    md9781_playlist_entry* playlist = NULL;
    int i = 0, number_of_files;

    #ifdef DEBUG

    printf("md9781_file_list\n");
    #endif

    if( location != 'M' && location != 'S' )
        return 0;

    memset(buffer, 0, 256);
    buffer[0] = '#';
    buffer[1] = 0x00;
    buffer[2] = 0x04;
    buffer[3] = ':';
    buffer[4] = location;
    buffer[5] = 'G';
    buffer[6] = 0x2e;

    if( md9781_bulk_write(dh,  buffer, 256 ) )
        return NULL;
    dummy_write( dh );

    /* this is the block with information about the number of files on the
     * player and maybe also contains information about the free memory
     * - but I have not been able to find out about that
     */
    memset(buffer, 0, USB_BLOCK_SIZE);
    if( md9781_bulk_read(dh,  buffer, 256) )
        return NULL;

    number_of_files = buffer[7];
    dummy_read(dh);
    #ifdef DEBUG

    printf("number of files: %d\n", number_of_files );
    #endif

    if( number_of_files > 0 ) {
        for( i = 0; i < 32; i++ ) {
            memset(buffer, 0, USB_BLOCK_SIZE);
            if( md9781_bulk_read(dh,  buffer, 256) )
                return NULL;

            if( buffer[0] != 0 ) {
                int pos = 0;
                while( buffer[pos] != 0 && pos < 256) {
                    md9781_entry* file = convert_file_entry( buffer+pos );
                    if( file->short_name[0] != 0xe5 ) {
                        if( entry != NULL ) {
                            file->prev = entry;
                            entry->next = file;
                        }
                        entry = file;
                        if( start == NULL )
                            start = entry;
                    }
                    pos = pos + 32;
                }
            }
        }

        if( !strcmp(start->short_name,"tmpFname") && use_info_file ) {
            playlist = md9781_play_list( dh, location, start );
            // if( playlist == NULL )
	    // return NULL;

            /* merge list of files from the filesystem of the player
             * with the playlist 
             */
            entry = start;
            while( entry != NULL ) {
                if( playlist != NULL ) {
                    entry->long_name = playlist->name;
                    playlist = playlist->next;
		  
                } else {
                    entry->long_name = entry->short_name;
                }
                entry = entry->next;
            }
        } else {
            entry = start;
            while( entry != NULL ) {
                entry->long_name = entry->short_name;
                entry = entry->next;
            }
        }
    } else {
        start = MD9781_NO_FILE_ON_PLAYER;
    }

    return start;
}

void md9781_freemem_filelist( md9781_entry* files ) {
    md9781_entry* entry = files;
    md9781_entry* old;

    while( entry != NULL ) {
        free(entry->long_name);
        old = entry;
        entry = old->next;
        free( old );
    }
}

/* gets the nr'th entry from the playlist */
md9781_entry*  md9781_entry_number( md9781_entry* playlist, int nr ) {
    md9781_entry* my_playlist = playlist;
    int i;
    for( i = 0; i < nr; i++ ) {
      if(my_playlist->next)
        my_playlist = my_playlist->next;
      else
	printf("entry_number: error: arg nr too large: %d\n", nr);
    }
    return my_playlist;
}

int md9781_number_of_files( md9781_entry* playlist ) {
    md9781_entry* my_playlist = playlist;
    int nr = 0;
    while( my_playlist != NULL ) {
        my_playlist = my_playlist->next;
        nr++;
    }
    return nr;
}


void md9781_print_playlist( md9781_entry* playlist ) {
    md9781_entry* my_playlist = playlist->next;
    int i = 1;
    long sum_filesize = 0;
    printf("+-----+-----------------------------------------------+----------+----------+------------+----------+\n");
    printf("|   # | Filename (long)                               | (short)  | Size     | Date       | Time     |\n");
    printf("+-----+-----------------------------------------------+----------+----------+------------+----------+\n");
    while( my_playlist != NULL ) {
        printf("| %3d ", i++ );
        printf("| %-45.45s ", my_playlist->long_name );
	printf("| %-8.8s ", my_playlist->short_name );
        printf("| %8ld ", my_playlist->size );
        printf("| %02d-%02d-%02d ", my_playlist->year, my_playlist->month,my_playlist->day );
        printf("| %02d:%02d:%02d ", my_playlist->hour, my_playlist->minute, my_playlist->second );
        printf("|\n");
        sum_filesize += my_playlist->size;
        my_playlist = my_playlist->next;
    }

    printf("+-----+-----------------------------------------------+----------+----------+------------+----------+\n");
    printf("| Used: %5.0f kB /", sum_filesize / 1024.0 );
    printf(" Free: %5.0f kB (%2.0f %%)                                                            |\n",
           64.0 * 1024.0 - sum_filesize / 1024.0 ,
           100.0 - sum_filesize / (64.0 * 1024.0 * 1024.0) * 100.0 );
    printf("+---------------------------------------------------------------------------------------------------+\n");

}

