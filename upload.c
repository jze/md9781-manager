#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

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
                        char location ) {
    unsigned char send_buffer[256];
    long filesize;
    int filetime, filedate, read, chunks, i;
    struct stat*  filestat = (struct stat *)malloc( sizeof( struct stat) );
    FILE* file = fopen( filename, "r");
    time_t t;
    struct tm*  tm;
    char* long_target = strdup( filename );
    char* short_target;
struct info_file_entry** playlist;

    if( location != 'M' && location != 'S' )
        return 0;
	
    if( rindex( long_target, '/' ) != NULL )
                 long_target = rindex(long_target, '/')+1;	
	
    if( strlen(long_target) > 8 ) {
             short_target = strdup(long_target);
             if( rindex( short_target, '.' ) != NULL )
                 rindex(short_target, '.')[0] = 0;
             short_target[8] = 0;
         }

    /* get some information about the file */
    stat( filename, filestat );
    filesize =  filestat->st_size + 16;
    chunks = filesize / 512;

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
    memset( send_buffer+6, ' ', 8 );
    memcpy( send_buffer+6, short_target, strlen(short_target) );

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
    read = 512;
    while( read == 512 ) {
        unsigned char buffer[512];

        memset(buffer, 0, 512);
        read = fread( buffer, 1, 512, file );
        md9781_bulk_write(dh, buffer, 512);
        /*  printf("%2.2f %% done\n", (i++ / (double)chunks) * 100 ); */
    }

    fclose(file);

printf("reading playlist\n");
playlist = md9781_play_list( dh, location);
i = 0;
while ( playlist[i] != NULL ) {
printf("%s\n", playlist[i]->name);
 i++ ;
}
printf("files on player: %d\n", i );
playlist[i] = (struct info_file_entry*)malloc( sizeof( struct info_file_entry) );

playlist[i]->name = long_target;
playlist[i]->size = filesize - 16;

printf("entry added\n");
md9781_upload_playlist( dh, location, playlist );
printf("playlist written\n");
    return 1;
}

