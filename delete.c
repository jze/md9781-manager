#include "common.h"

int md9781_delete_file( usb_dev_handle* dh, int file_number, char location ) {
    unsigned char send_buffer[256];
  
    if( location != 'M' && location != 'S' )
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

    if( md9781_bulk_write( dh, send_buffer, 256 ) ) {
        dummy_write( dh );
        dummy_read( dh );
        dummy_read( dh );
        
    } else
        return 0;
	
    
    return 1;
}

