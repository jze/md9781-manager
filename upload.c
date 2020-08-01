#include "common.h"
#include "libmd9781.h"


int md9781_upload_file_from_buffer( usb_dev_handle* dh,char location,
                                    char* filename, unsigned char* file_buffer, int length ) {
    unsigned char send_buffer[256];
    int filetime, filedate;
    time_t t;
    struct tm*  tm;
    int sent;

    t = time( NULL );
    tm = localtime( &t );

    filedate = (tm->tm_year-80)<<9 | (tm->tm_mon+1)<<5 | tm->tm_mday;
    filetime = tm->tm_hour << 11 | tm->tm_min << 5 | (tm->tm_sec/2);

    /* prepare the send_buffer */
    memset(send_buffer, 0, 256);

    /* the command */
    send_buffer[0] = '#';
    send_buffer[1] = 0x00;
    send_buffer[2] = 0x18;
    send_buffer[3] = ':';
    send_buffer[4] = location;
    send_buffer[5] = 'W';

    /* filename */
    strncpy( send_buffer+6, filename, 12 );

    /* date */
    send_buffer[18] = filedate & 0xff;
    send_buffer[19] = filedate >> 8;
    send_buffer[20] = filetime & 0xff;
    send_buffer[21] = filetime >> 8;

    /* size */
    send_buffer[22] = length>>24;
    send_buffer[23] = (length>>16) & 0xff;
    send_buffer[24] = (length>>8) & 0xff;
    send_buffer[25] = length & 0xff;

    /* end */
    send_buffer[26] = 0x2e;

    md9781_bulk_write(dh, send_buffer, 256);
    dummy_write(dh);
    dummy_read(dh);
    dummy_read(dh);

    sent = 0;
    while( sent < length ) {
        md9781_bulk_write(dh, file_buffer + sent, 512);
        sent = sent + 512;
    }

    return 1;
}

int md9781_upload_file( usb_dev_handle* dh,
                        const char* filename,
                        char location,
                        md9781_entry* playlist,
                        void (*callback)(int percent_done) ) {
    unsigned char send_buffer[256];
    long filesize;
    int filetime, filedate, read, chunks, i, last_value;
    struct stat*  filestat = (struct stat *)malloc( sizeof( struct stat) );
    FILE* file = fopen( filename, "r");
    time_t t;
    struct tm*  tm;
    char* long_target_old = strdup( filename );
    char* long_target = long_target_old;
    char* short_target;
    md9781_entry* playlist_start;
    md9781_entry* playlist_mem = NULL;
    md9781_entry* entry = (md9781_entry*)malloc(sizeof(md9781_entry));
    unsigned char* extension_temp;
    int numbered_names = 0;

    #ifdef DEBUG
    printf("md9781_upload_file\n");
    #endif

    if( location != 'M' && location != 'S' )
        return 0;

    if( playlist == NULL )
        playlist = playlist_mem = md9781_file_list(dh, location);
    playlist_start = playlist;

    if( rindex( long_target, '/' ) != NULL )
        long_target = rindex(long_target, '/')+1;

    short_target = strdup(long_target);

    extension_temp = rindex( short_target, '.' );
    if( extension_temp != NULL ) {
        entry->extension[0] = extension_temp[1];
        entry->extension[1] = extension_temp[2];
        entry->extension[2] = extension_temp[3];
        entry->extension[3] = 0;
        extension_temp[0] = 0;
    } else {
        entry->extension[0] = entry->extension[1] = entry->extension[2] = entry->extension[3] =0;
    }

    entry->fnumber = 0;

    /*  set up regular non-numbered name */
    memset( entry->short_name, ' ', 8);
    memcpy( entry->short_name, short_target, 8 );
    entry->short_name[8] = 0;

    /* only use numbered names if name wouldn't be unique, because
     * without id3 tags, the hardware player will display the (ugly)
     * numbered name 
     */
    for( playlist = playlist_start; playlist != NULL;
            playlist = playlist->next) {
        if(strncmp(entry->short_name, playlist->short_name, 8) == 0)
            numbered_names = 1;
    }

    if( use_info_file && numbered_names) {
        char file_numbers[1000];
        int c;
        for(c = 0; c < 1000; c++)
            file_numbers[c] = 0;

        playlist = playlist_start;

        /* find first free file number */
        while(playlist != NULL) {
            file_numbers[playlist->fnumber] = 1;
            playlist = playlist->next;
        }
        for(c = 1; file_numbers[c] == 1; c++)
            ;
        entry->fnumber = c;

        sprintf(entry->short_name, "_name%03d", c);
        entry->short_name[8] = 0;
    }

    /* get some information about the file */
    stat( filename, filestat );
    filesize =  filestat->st_size;
    chunks = filesize / 512;

    t = time( NULL );
    tm = localtime( &t );

    filedate = (tm->tm_year-80)<<9 | (tm->tm_mon+1)<<5 | tm->tm_mday;
    filetime = tm->tm_hour << 11 | tm->tm_min << 5 | (tm->tm_sec/2);

    entry->long_name = strdup(long_target);
    entry->size = filesize;
    entry->prev = NULL;
    entry->next = NULL;

    /* prepare the send_buffer */
    memset(send_buffer, 0, 256);

    /* the command */
    send_buffer[0] = '#';
    send_buffer[1] = 0x00;
    send_buffer[2] = 0x18;
    send_buffer[3] = ':';
    send_buffer[4] = location;
    send_buffer[5] = 'W';

    /* filename */
    memset( send_buffer+6, ' ', 8 );
    memcpy( send_buffer+6, entry->short_name, 8);

    send_buffer[14] = '.';
    send_buffer[15] = 'M';
    send_buffer[16] = 'P';
    send_buffer[17] = '3';

    /* date */
    send_buffer[18] = filedate & 0xff;
    send_buffer[19] = filedate >> 8;
    send_buffer[20] = filetime & 0xff;
    send_buffer[21] = filetime >> 8;

    /* size */
    send_buffer[22] = filesize>>24;
    send_buffer[23] = (filesize>>16) & 0xff;
    send_buffer[24] = (filesize>>8) & 0xff;
    send_buffer[25] = filesize & 0xff;

    /* end */
    send_buffer[26] = 0x2e;

    md9781_bulk_write( dh, send_buffer, 256);
    dummy_write(dh);
    dummy_read(dh);
    dummy_read(dh);

    /* send the file */
    i = 0;
    last_value = 0;
    read = 512;
    while( read == 512 ) {
        unsigned char buffer[512];

        memset(buffer, 0, 512);
        read = fread( buffer, 1, 512, file );
        md9781_bulk_write(dh, buffer, 512);
        if( callback != NULL ) {
            int percent_done = (i++ / (double)chunks) * 100;
            if( percent_done != last_value ) {
                callback( percent_done );
            }
            last_value = percent_done;
        }
    }
    fclose(file);

    if( use_info_file ) {
        playlist = playlist_start;
        while( playlist->next != NULL )
            playlist = playlist->next;

        /* add entry to list */
        playlist->next = entry;
        entry->prev = playlist;

        md9781_upload_playlist(dh,location, playlist_start);
    } else {
        free(entry->long_name);
        free(entry);
    }

    free(filestat);
    free(short_target);
    free(long_target_old);
    if(playlist_mem)
        md9781_freemem_filelist(playlist_mem);

    return 1;
}

