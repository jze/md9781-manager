#include "common.h"
#include "libmd9781.h"

md9781_entry* convert_file_entry( const unsigned char* buffer) {
    md9781_entry* file = (md9781_entry*)malloc(sizeof(md9781_entry));
    unsigned int date;
    unsigned int time;

    file->long_name = 0;
    memcpy( file->short_name, buffer, 8 );
    file->short_name[8] = 0;
    memcpy( file->extension, buffer+8, 3 );
    file->extension[3] = 0;

    file->size = (buffer[28] | buffer[29]<<8 |
                  buffer[30]<< 16 | buffer[31]<< 24 );

    date = buffer[25] |  buffer[24]<< 8;
    time = buffer[23] |  buffer[22]<< 8;
    file->year =  (date>>9)+1980;

    file->month =  (date>>5) & 15;
    file->day =date & 31;
    file->hour= time>>11;
    file->minute = (time>>5)&63;
    file->second = (time&31)<<1;

    file->next = NULL;
    file->prev = NULL;

    file->fnumber = 0;
    return file;
}

static void long_by_shortname(md9781_entry *entry) {
  int t;
  entry->long_name = (char*) malloc(13);
  t = sprintf(entry->long_name, "%s.%s", entry->short_name, entry->extension);
  entry->long_name[t] = 0;
}
  
md9781_entry* md9781_file_list( usb_dev_handle* dh, char location ) {
    unsigned char buffer[256];
    md9781_entry* start = NULL;
    md9781_entry* entry = NULL;
    md9781_playlist_entry* playlist = NULL;
    int i = 0, number_of_files;
    int t_size;
	 
    #ifdef DEBUG

    printf("md9781_file_list\n");
    #endif

    if( location != 'M' && location != 'S' )
        return 0;

    memset(buffer, 0, 256);
    buffer[0] = '#';
    buffer[1] = 0x00;
    buffer[2] = 0x04;
    buffer[3] = ':';
    buffer[4] = location;
    buffer[5] = 'G';
    buffer[6] = 0x2e;

    if( md9781_bulk_write(dh,  buffer, 256 ) )
        return NULL;
    dummy_write( dh );

    /* this is the block with information about the number of files on the
     * player and maybe also contains information about the free memory
     * - but I have not been able to find out about that
     */
    memset(buffer, 0, USB_BLOCK_SIZE);
    if( md9781_bulk_read(dh,  buffer, 256) )
        return NULL;

    number_of_files = buffer[7];
    dummy_read(dh);

    t_size = (buffer[10] << 8 | buffer[11]) * 16;
    if(location == 'M') {
      internal_size = t_size;

      #ifdef DEBUG
      printf("internal flash size probably: %d KB\n", internal_size);
      #endif
    } else {
      smc_size = t_size;
      if(smc_size == 0) {
	fprintf(stderr, "error: there is no external flash card - cannot access it\n");
	return NULL;
      }
      
      #ifdef DEBUG
      printf("smc size probably: %d KB\n", smc_size);
      #endif
    }
    
    #ifdef DEBUG

    printf("number of files: %d\n", number_of_files );
    #endif

    if( number_of_files > 0 ) {
        for( i = 0; i < 32; i++ ) {
            memset(buffer, 0, USB_BLOCK_SIZE);
            if( md9781_bulk_read(dh,  buffer, 256) )
                return NULL;

            if( buffer[0] != 0 ) {
                int pos = 0;
                while( buffer[pos] != 0 && pos < 256) {
                    md9781_entry* file = convert_file_entry( buffer+pos );
                    if( file->short_name[0] != 0xe5 ) {
                        if( entry != NULL ) {
                            file->prev = entry;
                            entry->next = file;
                        }
                        entry = file;
                        if( start == NULL )
                            start = entry;
                    } else {
                        free(file);
                    }
                    pos = pos + 32;
                }
            }
        }

        if( !strcmp(start->short_name,"tmpFname") && use_info_file ) {
            md9781_playlist_entry *tpl, *old_tpl, *tpl_prev;
	    playlist = md9781_play_list( dh, location, start );

            /* merge list of files from the filesystem of the player
             * with the playlist 
            * each short_name (if uploaded by this program) should be
            * _name###, were ### is an unique number.
            * long_name is found by searching the infofile entries
            * for an appropriate beginning, _name### will be cut off
             */

            entry = start;
            while( entry != NULL ) {
                int found = 0;
                tpl = playlist;
		tpl_prev = NULL;
		
		while(tpl != NULL && !found) {
		  // we have to compare the info name without extension,
		  // like it's stored as short_name
		  char *ext_p;
		  char *tmp_short_name = strdup(tpl->name);
		  if( (ext_p = rindex(tmp_short_name, '.')) != NULL)
		    *ext_p = 0;
		  if( (strncmp(entry->short_name, tmp_short_name, 8) == 0)
		      && !tpl->used ) {
		    
                        entry->long_name = tpl->name;
                        sscanf(entry->short_name, "_name%3d", &entry->fnumber);

                        if(entry->fnumber && (strlen(entry->long_name) > 8 ))
                            entry->long_name += 8;
                        entry->long_name = strdup(entry->long_name);

                        tpl->used = 1;
                        found = 1;
			old_tpl = tpl;
			tpl = tpl->next;

			// algorithm speedup (not neccessary): delete list item
			if(tpl_prev != NULL) {
			  tpl_prev->next = old_tpl->next;
			  old_tpl->next = NULL;
			  md9781_freemem_playlist(old_tpl);
			} else
			  tpl_prev = old_tpl;
			
		  } else {
		    tpl_prev = tpl;
		    tpl = tpl->next;
		  }
		  free(tmp_short_name);
		}
                if(!found) {
		  long_by_shortname(entry);
                }

                entry = entry->next;
            }
	    md9781_freemem_playlist(playlist);
        } else {
            entry = start;
            while( entry != NULL ) {
                long_by_shortname(entry);
                entry = entry->next;
            }
        }
    } else {
        start = MD9781_NO_FILE_ON_PLAYER;
    }

    return start;
}

void md9781_freemem_filelist( md9781_entry* files ) {
    md9781_entry* entry = files;
    md9781_entry* old;

    while( entry != NULL ) {
        if(entry->long_name != entry->short_name)
            free(entry->long_name);
        old = entry;
        entry = old->next;
        free( old );
    }
}

/* calls md9781_list_files, if size not yet known */
int md9781_flash_size( usb_dev_handle* dh, char location ) {
  md9781_entry* t = NULL;
  
  if( location != 'M' && location != 'S' )
    return 0;
  if( location == 'M' ) {
    if(internal_size == -1) {
      t = md9781_file_list(dh, location);
      if(t)
	md9781_freemem_filelist(t);
    }
    return internal_size;
  } else {
    if(smc_size == -1) {
      t = md9781_file_list(dh, location);
      if(t)
	md9781_freemem_filelist(t);
    }
    return smc_size;
  }
}

int md9781_round_to_bs(int size) {
  const int blocksize = 16*1024;
  return (int)ceil( (double)size / blocksize) * blocksize;
}
  
/* this respects overhead caused by fat and use of blocks */
int md9781_freesize_kb( int memsize_kb, md9781_entry* playlist) {
  int size = memsize_kb;
  /* reserve extra size for info file - it will grow on uploads */
  int info_ext = 1024;
  while(playlist != NULL) {
    size -= md9781_round_to_bs( playlist->size - 16 + info_ext) / 1024;
    info_ext = 0;
    playlist = playlist->next;
  }
  /* fat causes about 64 kb overhead */
  size -= 64;
  return size;
}
  
/* gets the nr'th entry from the playlist */
md9781_entry*  md9781_entry_number( md9781_entry* playlist, int nr ) {
    md9781_entry* my_playlist = playlist;
    int i;
    for( i = 0; i < nr; i++ ) {
        if(my_playlist->next)
            my_playlist = my_playlist->next;
        else
            printf("entry_number: error: arg nr too large: %d\n", nr);
    }
    return my_playlist;
}

int md9781_number_of_files( md9781_entry* playlist ) {
    md9781_entry* my_playlist = playlist;
    int nr = 0;
    while( my_playlist != NULL ) {
        my_playlist = my_playlist->next;
        nr++;
    }
    return nr;
}



