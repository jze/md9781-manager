#include "common.h"
#include "libmd9781.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>

const int info_ssize = 8192;

md9781_playlist_entry* md9781_play_list( usb_dev_handle* dh, char location,
        const md9781_entry* playlist_file ) {
    unsigned char buffer[256];
    md9781_playlist_entry* playlist = NULL;
    md9781_playlist_entry* entry = NULL;
    long filesize;
    int chunks,i ;
    unsigned char*  info_file;
    char* orig_info_file;

    #ifdef DEBUG
    printf("Reading playlist.\n");
    #endif

    if( location != 'M' && location != 'S' )
        return NULL;

    filesize = playlist_file->size;
    chunks = filesize / 512 + 1;
    info_file = (char*)malloc( ((filesize+1)/512 +1 ) * 512 );
    orig_info_file = info_file;

    /* prepare the send_buffer */
    memset(buffer, 0, 256);

    /* the command */
    buffer[0] = '#';
    buffer[1] = 0x00;
    buffer[2] = 0x18;
    buffer[3] = ':';
    buffer[4] = location;
    buffer[5] = 'R';
    buffer[6] = MD9781_INFO_FILE;
    buffer[7] = 0x2e;

    if( md9781_bulk_write(dh, buffer, 256) ) return NULL;
    dummy_write(dh);
    dummy_read(dh);

    /** needs some work - you must know the filesize and hence the number of
    packages you can read */

    dummy_read(dh);
    dummy_read(dh);
    dummy_read(dh);

    /* read the file */
    for(  i = 0; i < chunks; i++ ) {
        int length = 512;
        if( md9781_bulk_read(dh, info_file + i *512 , 512) ) return NULL;
        if( (i+1) * 512 > filesize )
            length = filesize % 512;
    }

    info_file[filesize] = 0;

    #ifdef DEBUG
    printf("INFO-FILE:\n%s\n", info_file);
    #endif

    while( index(info_file, 0xd ) != NULL ) {
        int name_length;
        unsigned char* size_string = NULL;

        if( entry != NULL ) {
            entry->next = (md9781_playlist_entry*)malloc(sizeof(md9781_playlist_entry) );
            entry = entry->next;
        }
        else {
            entry = (md9781_playlist_entry*)malloc(sizeof(md9781_playlist_entry) );
        }

        name_length = index(info_file, 0xd ) - (char*)info_file;
        info_file[name_length] = 0;

        if(index(info_file, '#'))
            size_string =  index( info_file, '#' ) +1 ;
        else
            printf("format error in info file: no size parameter\n");

        size_string[-1] = 0;
        entry->next = NULL;
        entry->name = strdup(info_file);
        entry->size = atoi( size_string );
        if( playlist == NULL )
            playlist = entry;

        info_file = info_file + name_length +2 ;
    }

    free(orig_info_file);
    return playlist;
}

int md9781_upload_playlist( usb_dev_handle* dh,
                            char location,
                            md9781_entry* playlist ) {
    char* info_file = (char*)malloc(info_ssize);
    md9781_entry* entry = playlist;
    int len;
    len = 0;

    while( entry != NULL ) {
        len += sprintf( info_file + len, "%s#%ld\r\n", entry->long_name, entry->size -16 );
        entry = entry->next;
    }
    info_file[len] = 0;


    #ifdef DEBUG
    printf("Writing info file:\n");
    printf("%s\n", info_file);
    #endif

    /* delete the old info file */
    md9781_delete_file( dh, MD9781_INFO_FILE, location,playlist);

    md9781_upload_file_from_buffer(dh, location, "tmpFname.txt", info_file, strlen(info_file) );

    free(info_file);
    return 1;
}

int md9781_init_playlist( usb_dev_handle* dh,
                          char location ) {
    char* info_file = (char*)malloc(256);
    memset( info_file, 0, 256 );
    memcpy( info_file, "tmpFname.txt#17\r\n", 17 );
    md9781_upload_file_from_buffer(dh, location, "tmpFname.txt", info_file, strlen(info_file) );
    free(info_file);
    return 1;
}

int md9781_regenerate_playlist( usb_dev_handle* dh,char location ) {
    char* info_file = (char*)malloc(info_ssize);
    md9781_entry* playlist;
    md9781_entry* entry;
    int len, i;

    ignore_info_file();
    printf("regenerating playlist\n");
    playlist = md9781_file_list(dh, location);
    memset( info_file, 0, info_ssize );
    len = 0;
    i = 0;
    entry = playlist;
    while( entry != NULL ) {
        len += sprintf( info_file + len, "%s.%s#%ld\r\n", entry->short_name,entry->extension, entry->size -16 );
        entry = entry->next;
    }
    printf("%s\n", info_file );

    /* is the first file on the player the info file ? */

    printf("%d\n", (playlist->size < MAX_INFO_FILE_SIZE) );
    printf("%d\n", ( strcmp( playlist->short_name , "tmpFname" ) == 0 ) );
    printf("%d\n", ( strcmp( playlist->short_name , "temp0000" ) == 0 ) );

    if( (playlist->size < MAX_INFO_FILE_SIZE) &&
            (( strcmp( playlist->short_name , "tmpFname" ) == 0 ) ||
             ( strcmp(playlist->short_name, "TEMP 000") == 0 ) )
      ) {
        printf("deleting old info file\n");
        md9781_delete_file( dh, MD9781_INFO_FILE, location, playlist );
    }

    md9781_upload_file_from_buffer(dh, location, "tmpFname.txt", info_file, strlen(info_file) );

    free(info_file);
    return 1;
}

void md9781_freemem_playlist( md9781_playlist_entry* playlist ) {
    md9781_playlist_entry* entry = playlist;
    md9781_playlist_entry* old;

    if( playlist != NULL ) {
        while( playlist != NULL ) {
            free(playlist->name);
            old = entry;
            entry = old->next;
            free(old);
        }
    }
}
