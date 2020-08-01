#include "common.h"
#include "libmd9781.h"
#include <stdio.h>
#include <string.h> 

struct info_file_entry** md9781_play_list( usb_dev_handle* dh, char location ) {
    unsigned char buffer[256];
    struct info_file_entry** playlist;
    struct file_descriptor ** files;
    long filesize;
    int chunks,i, retval, file_nr, number_of_files ;
    char*  info_file;

    if( location != 'M' && location != 'S' )
     return 0;

    /* get list of files */
    files = md9781_file_list(dh, location);
    
    number_of_files = 0;
    while( files[number_of_files] != NULL )  number_of_files++;
    playlist = malloc((number_of_files+1)*sizeof(void*));
    memset( playlist, 0, (number_of_files+1)*sizeof(void*) );
    
       #ifdef DEBUG
    printf("%d files\n", number_of_files );
#endif
    filesize = files[MD9781_INFO_FILE]->size;
    
    md9781_freemem_filelist( files );
    
    chunks = filesize / 512 + 1;
    info_file = (char*)malloc( filesize +1 );

       #ifdef DEBUG
    printf("Exspecting a %ld byte (%d packets) long file.\n", filesize, chunks);
#endif

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

    md9781_bulk_write(dh, buffer, 256);
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
        retval = md9781_bulk_read(dh, info_file + i *512 , 512);
        if( (i+1) * 512 > filesize )
            length = filesize % 512;
    }
    
    info_file[filesize+1] = 0;

       #ifdef DEBUG
    printf("%s\n", info_file);
       #endif

    file_nr = 0;
    while( index(info_file, 0xd ) != NULL ) {
       int name_length;
       char* size_string;
       
       playlist[file_nr] = (struct info_file_entry*)malloc( INFO_FILE_ENTRY_LENGTH );
       name_length = index(info_file, 0xd ) - info_file;
       info_file[name_length+1] = 0;
       playlist[file_nr]->name = strdup(info_file);
       size_string =  index( playlist[file_nr]->name, '#' ) +1 ;
       size_string[-1] = 0;
       playlist[file_nr]->size = atoi( size_string );
       
       #ifdef DEBUG
       printf("%s  - %ld\n", playlist[file_nr]->name, playlist[file_nr]->size);
       #endif
       
       info_file = info_file + name_length +2 ;
       file_nr++;
    }

    return playlist;
}

int md9781_upload_playlist( usb_dev_handle* dh,
                            char location,
                            struct info_file_entry** playlist ) {
    char* info_file = (char*)malloc(8192);
    int i, len;

    len = 0;
    for( i = 0; playlist[i] != NULL ; i++ ) 
        len += sprintf( info_file + len, "%s#%ld\r\n", playlist[i]->name, playlist[i]->size -16 );

    printf("%s\n", info_file);
    
    /* delete the old info file */
    md9781_delete_file( dh, MD9781_INFO_FILE, location );

    md9781_upload_file_from_buffer(dh, location, "tmpFname.txt", info_file, strlen(info_file) );

    return 1;
}

int md9781_regenerate_playlist( usb_dev_handle* dh,char location ) {
    char* info_file = (char*)malloc(8192);
    struct file_descriptor ** files;
    int len, i;

printf("regenerating playlist\n");
    files = md9781_file_list(dh, location);
    memset( info_file, 0, 8192 );
    len = 0;
    i = 0;
    while( files[i] != NULL ) {
        len += sprintf( info_file + len, "%s#%ld\r\n", files[i]->name, files[i]->size -16 );
        i++;
    }
printf("%s\n", info_file );
    md9781_delete_file( dh, MD9781_INFO_FILE, location );
    md9781_upload_file_from_buffer(dh, location, "tmpFname.txt", info_file, strlen(info_file) );
    
    free(info_file);
    return 1;
}

void md9781_freemem_playlist( struct info_file_entry** playlist ) {
   int i = 0;
   if( playlist != NULL ) {
      while( playlist[i] != NULL ) {
         free(playlist[i]->name);
	 free(playlist[i]);
	 i++;
      }
   }
}
