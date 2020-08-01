#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libmd9781.h"
#include "libmd9781.h"

#define ACTION_DELETE	1
#define ACTION_UPLOAD	2
#define ACTION_DOWNLOAD	3
#define ACTION_LIST	4
#define ACTION_SHOW_PLAYLIST	5
#define ACTION_REGENERATE_PLAYLIST	6
#define ACTION_INITIALIZE_PLAYLIST	7

void print_usage() {
    fprintf(stderr, "USAGE: md9781-manager <options> <command>\n" );
    fprintf(stderr, "\nOPTIONS:\n");
    fprintf(stderr, "-e            use smartmedia card\n");
    fprintf(stderr, "\nCOMMANDS:\n");
    fprintf(stderr, "-d <nr>      delete file number nr\n");
    fprintf(stderr, "-l           list files\n");
    fprintf(stderr, "-i           ignore playlist (if you know what you do)\n");
    fprintf(stderr, "-p <file> [targetname]\n");
    fprintf(stderr, "             put a file to the player\n");
    fprintf(stderr, "             (if targetname is given the file will get that name on the player)\n");
    fprintf(stderr, "-g <nr>      get file number nr and save it in the current directory\n");
}

int main( int argc, char **argv) {
    int i,action_count=0, nr = 0,action;
    md9781_entry* playlist = NULL;
    char location = 'M';
    usb_dev_handle* md9781_handle;
    char* source;
    int max_retries = 5, retries = 0;

    for( i = 1; i < argc; i++ ) {
        if( !strcmp( argv[i], "-e" ) ) {
            location = 'S';
        } else if( !strcmp( argv[i], "-d" ) ) {
            char* nr_string = argv[++i];
            action = ACTION_DELETE;
            action_count += 1;
            nr = atoi( nr_string );
        } else if( !strcmp( argv[i], "-i" ) ) {
            ignore_info_file();
        } else if( !strcmp( argv[i], "-l" ) ) {
            action = ACTION_LIST;
            action_count++;
        } else if( !strcmp( argv[i], "-g" ) ) {
            char* nr_string = argv[++i];
            action = ACTION_DOWNLOAD;
            action_count++;
            nr = atoi( nr_string );
        } else if( !strcmp( argv[i], "-p" ) ) {
            action = ACTION_UPLOAD;
            action_count++;
            source = argv[++i];
        } else if( !strcmp( argv[i], "-lpl" ) ) {
            action = ACTION_SHOW_PLAYLIST;
            action_count++;
        } else if( !strcmp( argv[i], "-r" ) ) {
            ignore_info_file();
            action = ACTION_REGENERATE_PLAYLIST;
            action_count++;
        } else if( !strcmp( argv[i], "--init" ) ) {
            ignore_info_file();
            action = ACTION_INITIALIZE_PLAYLIST;
            action_count++;
        }
    }

    if( action_count != 1 ) {
        print_usage();
        exit( EXIT_FAILURE );
    }


    #ifdef DEBUG
    printf("opening device\n");
    #endif

    md9781_handle = md9781_open();
    if( md9781_handle == NULL ) {
        abort();
    }

    while( playlist == NULL && retries < max_retries ) {
        #ifdef DEBUG
        printf("getting filelist\n");
        #endif

        playlist = md9781_file_list(md9781_handle, location);
        if( playlist == NULL ) {
            retries++;
            md9781_close(md9781_handle);
            printf("communication failed - waiting %d seconds\n", 5 * retries);
            sleep( 5* retries );
            md9781_handle = md9781_open();
            if( md9781_handle == NULL )
                abort();
        }
    }

    if( playlist == NULL ) {
        abort();
    }

    if( action == ACTION_DELETE ) {
        if( nr > 0 ) {
            printf("Deleting file #%d...\n", nr);
            if( md9781_delete_file( md9781_handle , nr,  location,playlist ) )
                printf("File #%d  deleted.\n", nr);
        }
    } else if( action == ACTION_LIST ) {
        long sum_filesize = 0;

        i = 0;
        /* list files on player */
        printf("+-----+-------------------------------------+----------+------------+----------+\n");
        printf("|   # | Filename                            | Size     | Date       | Time     |\n");
        printf("+-----+-------------------------------------+----------+------------+----------+\n");

        if( playlist == MD9781_NO_FILE_ON_PLAYER )  {
            printf("| NO FILES ON PLAYER - please use '--init' to create a new info-file           |\n");
        printf("+------------------------------------------------------------------------------+\n");
	    exit(0);
        }

        while( playlist != NULL ) {
            printf("| %3d ", i++ );
            printf("| %-35.35s ", playlist->long_name );
            printf("| %8ld ", playlist->size );
            printf("| %02d-%02d-%02d ", playlist->year, playlist->month,playlist->day );
            printf("| %02d:%02d:%02d ", playlist->hour, playlist->minute, playlist->second );
            printf("|\n");
            sum_filesize += playlist->size;
            playlist = playlist->next;
        }

        printf("+-----+-------------------------------------+----------+------------+----------+\n");
        printf("| Used: %5.0f kB /", sum_filesize / 1024.0 );
        printf(" Free: %5.0f kB (%2.0f %%)                                       |\n",
               64.0 * 1024.0 - sum_filesize / 1024.0 ,
               100.0 - sum_filesize / (64.0 * 1024.0 * 1024.0) * 100.0 );
        printf("+------------------------------------------------------------------------------+\n");
    } else if( action == ACTION_DOWNLOAD) {
        /* get a file from the player (download) */
        if( nr > 0 ) {
	   char* filename = md9781_entry_number( playlist, nr )->long_name;
            printf("Getting file #%d will call it '%s'...\n", nr, filename);
            if( md9781_download_file( md9781_handle , filename, nr, location,playlist ) )
                printf("File #%d saved.\n", nr);
        }
    } else if( action == ACTION_UPLOAD ) {
        /* put a file on the player (upload) */
        printf("Putting %s to the player\n", source);
        if( md9781_upload_file( md9781_handle, source, location,playlist) ) {
            printf("File has been uploaded.\n");
        }
    } else if( action == ACTION_REGENERATE_PLAYLIST ) {
        md9781_regenerate_playlist( md9781_handle, location );
    } else if( action == ACTION_INITIALIZE_PLAYLIST ) {
        md9781_init_playlist( md9781_handle, location );
    }


    md9781_close(md9781_handle);
    return 0;
}
