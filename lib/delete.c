#include "common.h"
#include "libmd9781.h"

// for debugging
/*
void show_list(md9781_entry *entry) {
  printf("show_list: start\n");
  while(entry) {
    printf("show_list: %s = %s\n", entry->short_name, entry->long_name);
    entry = entry->next;
  }
  printf("show_list: end\n");
}
*/

int md9781_delete_file( usb_dev_handle* dh, int file_number, char location,
                        md9781_entry* playlist ) {
    unsigned char send_buffer[256];
    md9781_entry* entry;
    md9781_entry* playlist_mem = NULL;

    #ifdef DEBUG

    printf("delete file #%d\n", file_number );
    #endif

    if( location != 'M' && location != 'S' )
        return 0;

    if( playlist == NULL ) {
        playlist = playlist_mem = md9781_file_list(dh, location );
	if( ! playlist )
	  return 0;
    }

    if( file_number >= md9781_number_of_files( playlist ) ) {
        if(playlist_mem)
            md9781_freemem_filelist(playlist_mem);
        return 0;
    }

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
    md9781_bulk_read_with_timeout(dh, send_buffer, 256, USB_LONG_TIMEOUT);
    dummy_read( dh );

    /* unless it is the info file that has been deleted, a new info file
       will be written */
    if( file_number > MD9781_INFO_FILE) {
        entry = md9781_entry_number( playlist, file_number );
	
	// remove item from list
        if( entry->prev != NULL ) {
            entry->prev->next = entry->next;
	    if(entry->next != NULL)
	      entry->next->prev = entry->prev;
	} else
	  printf("this shouldn't have happend\n");

	// delete item, using delete_list function
	// it's sure that file_number is > 0, so the first item will not
	// be deleted (that would make things more complicated)
        entry->next = NULL;
        md9781_freemem_filelist(entry);

        if( use_info_file )
            md9781_upload_playlist( dh, location, playlist );

    }

    if(playlist_mem)
        md9781_freemem_filelist(playlist_mem);

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
    Passed_args passdown_args = {dh, location, playlist, 0 };
    int ret;

    /* temporarly ignore the info_file, because updating it only once,
     * at the end, is faster
     */
    int old_uinf = use_info_file;
    ignore_info_file();
    ret = exec_on_range(1, md9781_number_of_files(playlist) - 1,
                        range, fp_delete, &passdown_args);
    use_info_file = old_uinf;
    md9781_upload_playlist( dh, location, playlist );

    return ret;
}

