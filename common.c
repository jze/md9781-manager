#include <stdio.h>
#include <malloc.h>
#include "common.h"

void error_message(unsigned char *hdr, unsigned char *txt) {
    fprintf(stderr, "\nERROR: %-15.15s %s\n", hdr, txt);
}

void debug_message(unsigned char *hdr, unsigned char *txt) {
    #ifdef DEBUG
    fprintf(stderr, "\nDEBUG: %-15.15s %s\n", hdr, txt);
    #endif
}

usb_dev_handle*  md9781_open() {
    int retval;
    struct usb_device *md9781_dev = NULL;
    struct usb_bus *bus;
    struct usb_device *dev;
    usb_dev_handle* md9781_handle;

    usb_find_busses();
    usb_find_devices();

    /* initialize the usb stuff */
    if (md9781_dev == NULL) {
        usb_init();
        usb_find_busses();
        usb_find_devices();
    }

    md9781_dev = NULL;
    for (bus = usb_busses; bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if ((dev->descriptor.idVendor == 0x4e8 )
                    && (dev->descriptor.idProduct == 0x1003 )) {
                md9781_dev = dev;
                debug_message("md9781_initialize", "md9781 device found");
            }
        }
    }

    if (md9781_dev == NULL) {
        error_message("md9781_initialize", "device not found");
        return NULL;
    }

    debug_message("md9781_open", "begin");
    md9781_handle = usb_open(md9781_dev);
    if (md9781_handle == NULL) {
        error_message("md9781_open", "usb_open failed");
        return NULL;
    }

    retval = usb_claim_interface(md9781_handle, 0);
    if (retval < 0) {
        error_message("md9781_open", "usb_claim_interface failed");
        return NULL;
    }

    return md9781_handle;
}

int md9781_close( usb_dev_handle* md9781_handle ) {
    debug_message("md9781_close", "begin");

    if (md9781_handle == NULL) {
        error_message("md9781_close", "need to initialize first");
        return -1;
    }

    usb_close(md9781_handle);
    return 0;
}

/** reads 256 dummy bytes from the bus */
int dummy_read( usb_dev_handle* dh ) {
    int retval;
    static unsigned char buffer[256];

    retval = usb_bulk_read(dh, 3, buffer, USB_BLOCK_SIZE, USB_SHORT_TIMEOUT);
    if (retval < 0) {
        error_message("dummy_read", "md9781_read failed");
        exit( EXIT_FAILURE );
    }
    dump_buffer( buffer, 256 );

    return 0;
}

/** writes 256 zero bytes from the bus */
int dummy_write( usb_dev_handle* dh ) {
    int retval;
    static unsigned char buffer[256];
    memset( buffer, 0, 256);

    retval = usb_bulk_write(dh, 2, buffer, USB_BLOCK_SIZE, USB_SHORT_TIMEOUT);

    if (retval < 0) {
        error_message("dummy_write", "md9781_write failed");
        exit( EXIT_FAILURE );
    }
    dump_buffer( buffer, 256 );

    return 0;

}

void dump_buffer( const unsigned char* buffer, int size ) {
    #ifdef DEBUG
    int pos,i,temp, is_not_zero;

    for( pos = 0; pos < size; pos++ ) {
        if( pos % 16 == 0 && pos + 16 < size ) {
            is_not_zero = 0;
            for( i = pos; i < pos + 16; i++ )
                is_not_zero += buffer[i];
            if( is_not_zero )
                printf("%08x  ", pos );
        }

        if( is_not_zero ) {
            printf("%02x ", buffer[pos] );

            if( (pos % 8) == 7  )
                printf( " " );
        }

        if( pos % 0x10 == 0x0f ) {
            if( is_not_zero ) {
                printf("|");
                for( i = pos - 0x0f; i <= pos; i++ ) {
                    if( buffer[i] >= ' ' )
                        printf("%c", buffer[i] );
                    else
                        printf(".");
                }
                printf("|\n");

            }
        }

    }

    if( is_not_zero ) {
        temp = pos % 16;
        if( temp == 0 ) temp = 16;

        for( i = temp; i < 16; i++ )
            printf("   ");
        printf(" |");
        for( i = pos - temp; i <= pos; i++ ) {
            if( buffer[i] >= ' ' )
                printf("%c", buffer[i] );
            else
                printf(".");
        }
        printf("|\n");
    }
    #endif
}

int md9781_bulk_write( usb_dev_handle* dh, char* buffer, int size ) {
    int retval;
    dump_buffer(buffer, size);
    retval = usb_bulk_write(dh, 2, buffer, size, USB_SHORT_TIMEOUT);
    if (retval < 0) {
        error_message("md9781_bulk_write", "md9781_send failed");
        exit( EXIT_FAILURE );
    }
    return retval;
}

int md9781_bulk_read( usb_dev_handle* dh, char* buffer, int size ) {
    int retval;
    retval = usb_bulk_read(dh, 3, buffer, size, USB_SHORT_TIMEOUT);
    if (retval < 0) {
        error_message("md9781_bulk_read", "md9781_read failed");
        exit( EXIT_FAILURE );
    }
    dump_buffer(buffer, size);
    return retval;
}
