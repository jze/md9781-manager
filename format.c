#include "common.h"
#include "libmd9781.h"

int md9781_format( usb_dev_handle* dh, char location) {
    unsigned char send_buffer[256];
    memset(send_buffer, 0, 256);
    send_buffer[0] = '#';
    send_buffer[1] = 0x00;
    send_buffer[2] = 0x05;
    send_buffer[3] = ':';
    send_buffer[4] = location;
    send_buffer[5] = 'F';
    send_buffer[6] = 0x2e;

    md9781_bulk_write( dh, send_buffer, 256 );
    dummy_write( dh );
         
    sleep(10);
    
    md9781_bulk_read(dh, send_buffer, 256);
    dummy_read( dh );

    md9781_init_playlist( dh, location );
    return 1;
}
