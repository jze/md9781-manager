#include "common.h"

int use_info_file = 1;

/* sizes in KB,  -1 = not yet known */
int smc_size;
int internal_size;

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

    internal_size = -1;
	 smc_size = -1;
	 
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
            } else if ((dev->descriptor.idVendor == 0xaa9 )
	 	    && (dev->descriptor.idProduct == 0xf01b )) {
		md9781_dev = dev;
		debug_message("md9781_initialize", "md6242 device found");
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
        return MD9781_ERROR;
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
        return MD9781_ERROR;
    }
    dump_buffer( buffer, 256, "dummy_read" );

    return 0;
}

int md9781_dummy_read( usb_dev_handle* dh ) {
   return dummy_read( dh );
}

/** writes 256 zero bytes from the bus */
int dummy_write( usb_dev_handle* dh ) {
    int retval;
    static unsigned char buffer[256];
    memset( buffer, 0, 256);

    retval = usb_bulk_write(dh, 2, buffer, USB_BLOCK_SIZE, USB_SHORT_TIMEOUT);

    if (retval < 0) {
        error_message("dummy_write", "md9781_write failed");
        return MD9781_ERROR;
    }
    dump_buffer( buffer, 256, "dummy_write" );

    return 0;

}

void dump_buffer( const unsigned char* buffer, int size, const char* descr ) {
    #ifdef DEBUG
    int pos,i,temp, is_not_zero;

    printf("dump called from: %s\n", descr);
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
    printf("end dump (from %s)\n", descr);
    #endif
}

int md9781_bulk_write( usb_dev_handle* dh, char* buffer, int size ) {
    int retval;
    dump_buffer(buffer, size, "md9781_bulk_write");
    retval = usb_bulk_write(dh, 2, buffer, size, USB_SHORT_TIMEOUT);
    if (retval < 0) {
        error_message("md9781_bulk_write", "md9781_send failed");
        return MD9781_ERROR;
    }
    return MD9781_SUCCESS;
}

int md9781_bulk_read_with_timeout( usb_dev_handle* dh, char* buffer, int size,
                                   long timeout ) {
    int retval;
    retval = usb_bulk_read(dh, 3, buffer, size, timeout);
    if (retval < 0) {
        error_message("md9781_bulk_read", "md9781_read failed");
        return MD9781_ERROR;
    }
    dump_buffer(buffer, size, "md9781_bulk_read");
    return MD9781_SUCCESS;
}


int md9781_bulk_read( usb_dev_handle* dh, char* buffer, int size ) {
    return md9781_bulk_read_with_timeout(dh, buffer, size, USB_SHORT_TIMEOUT);
}

void ignore_info_file() {
    use_info_file = 0;
}


double getsec() {
  struct timeval t;
  gettimeofday(&t, 0);
  return (double)t.tv_sec + t.tv_usec / 1000000.0;
}

/* return -1 on error, >= 0 (number of deleted items) on success
 * fp point to a function that returns 1 on success and takes the
 * (file) number to be deleted/downloaded/etc and an additional argument arg
 */
int exec_on_range(int low, int high, char *range, int (*fp)(int n, void *arg), void *arg) {
    char accept[] = "0123456789-,";
    int n_exec = 0;
    int nmax = high - low +1;	/* max number of entries in field */
    int field[nmax];
    int i, j, last_field;
    char *trange = range;
    char *tp;

    for(i = 0; i < nmax; i++)
        field[i] = 0;

    /* check for invalid chars */
    if(strspn(range, accept) != strlen(range)) {
        fprintf(stderr, "range string error: invalid char(s) used\n");
        return -1;
    }

    /* process the comma seperated ranges list - one field at a time */
    for(last_field = 0; !last_field; trange = tp + 1) {
        char invalid_range_err[] = "range error: %d not in valid range\n";
        char *first, *second;
        int k;
        if( (tp = index(trange, ',')) != NULL) {
            *tp = 0;
        } else
            last_field = 1;
        if(strlen(trange) == 0)
            continue;

        /* process the string [^,]<string>[,$]
         * it may be an integer, a range #-#, or an incomplete range -#, #- 
         */
        second = index(trange, '-');
        if(second == NULL) {	/* no dash - one number */
            i = atoi(trange);
            if(i >= low && i <= high)
                field[i - low] = 1;
            else {
                fprintf(stderr, invalid_range_err, i);
                return -1;
            }
        } else {			/* a range x-y */
            first = trange;
            *second = 0;
            second++;

            if(strlen(first) != 0) {
                i = atoi(first);
                if(i < low || i > high) {
                    fprintf(stderr, invalid_range_err, i);
                    return -1;
                }
            } else			/* first is empty */
                i = low;

            if(strlen(second) != 0) {
                j = atoi(second);
                if(j < low || j > high) {
                    fprintf(stderr, invalid_range_err, j);
                    return -1;
                }
            } else			/* second is empty */
                j = high;

            if(j < i ) {
                fprintf(stderr, "range error: wrong order in <x>-<y>\n");
                return -1;
            }

            for(k = i; k <= j; k++)
                field[k - low] = 1;
        }

    }

    for(i = high; i >= low; i--) {
        if(field[i - low]) {

            if(fp(i, arg) == 1)
                n_exec++;
        }
    }

    return n_exec;
}

char* md9781_get_version() {
    return MD9781_VERSION;
}
