#include "common.h"
#include "libmd9781.h"

int md9781_delete_file( usb_dev_handle* dh, int file_number, char location,
                        md9781_entry* playlist ) {
    unsigned char send_buffer[256];
    md9781_entry* entry;

    #ifdef DEBUG

    printf("delete file #%d\n", file_number );
    #endif

    if( location != 'M' && location != 'S' )
        return 0;

    if( playlist == NULL )
        playlist = md9781_file_list(dh, location );

    if( file_number >= md9781_number_of_files( playlist ) )
        return 0;

    memset(send_buffer, 0, 256);
    send_buffer[0] = '#';
    send_buffer[1] = 0x00;
    send_buffer[2] = 0x05;
    send_buffer[3] = ':';
    send_buffer[4] = location;
    send_buffer[5] = 'E';
    send_buffer[6] = file_number;
    send_buffer[7] = 0x2e;

    md9781_bulk_write( dh, send_buffer, 256 );
    dummy_write( dh );
    dummy_read( dh );
    dummy_read( dh );

    /* unless it is the info file that has been deleted, a new info file
       will be written */
    if( use_info_file && (file_number > MD9781_INFO_FILE) ) {

        entry = md9781_entry_number( playlist, file_number );

        if( entry->prev != NULL )
            entry->prev->next = entry->next;

        md9781_upload_playlist( dh, location, playlist );
    }

    return 1;
}


static int fp_delete(int nr, void *x) {
    Passed_args *arg = x;
    if( nr > 0 ) {
        printf("Deleting file #%d...\n", nr);
        if( md9781_delete_file( arg->dh, nr, arg->location,
                                arg->playlist) ) {
            printf("File #%d  deleted.\n", nr);
            return 1;
        }
    }
    return 0;
}

int md9781_delete_range( usb_dev_handle* dh, char *range, char location,
                         md9781_entry* playlist ) {
    Passed_args passdown_args = {
        dh, location, playlist
    };

    return exec_on_range(1, md9781_number_of_files(playlist) - 1,
                         range, fp_delete, &passdown_args);
}

