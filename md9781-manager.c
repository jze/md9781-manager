#include "config.h"
#include "libmd9781.h"

#define ACTION_DELETE	1
#define ACTION_UPLOAD	2
#define ACTION_DOWNLOAD	3
#define ACTION_LIST	4
#define ACTION_SHOW_PLAYLIST	5
#define ACTION_REGENERATE_PLAYLIST	6
#define ACTION_INITIALIZE_PLAYLIST	7
#define ACTION_FORMAT	8

void print_usage() {
    fprintf(stderr, "md9781-manager version %s\n", MD9781_VERSION );
    fprintf(stderr, "USAGE: md9781-manager <options> <command>\n" );
    fprintf(stderr, "\nOPTIONS:\n");
    fprintf(stderr, "-e           use smartmedia card\n");
    fprintf(stderr, "-i           ignore playlist (if you know what you do)\n");
    fprintf(stderr, "\nCOMMANDS:\n");
    fprintf(stderr, "-d <range-l> delete files numbered in <range-list>, e.g. 3-6,7,1 \n");
    fprintf(stderr, "-l           list files\n");
    fprintf(stderr, "--format     format memory - erases all data (of course)\n");
    fprintf(stderr, "-p <file..>  put files to the player\n");
    fprintf(stderr, "-g <range-l> get the files named in <range-l> and save them in the current directory\n");
}

void print_percent_done(int value ) {
    int i= 0;
    for( i=0; i<9; i++)
        printf("%c", 8 );
    printf("%3.0d%% done", value );
    fflush( stdout );
}

int main( int argc, char **argv) {
    int i,action_count=0, nr = 0,action = 0;
    char *nr_string = NULL;
    md9781_entry* playlist = NULL;
    char location = 'M';
    usb_dev_handle* md9781_handle;
    int max_retries = 5, retries = 0;
    int source_count = 0;
    char* sources[argc];	// vector of remaining args (e.g. files)

    for( i = 1; i < argc; i++ ) {
        if( !strcmp( argv[i], "-e" ) ) {
            location = 'S';
        } else if( !strcmp( argv[i], "-d" ) ) {
            nr_string = argv[++i];
            action = ACTION_DELETE;
            action_count += 1;
        } else if( !strcmp( argv[i], "-i" ) ) {
            ignore_info_file();
        } else if( !strcmp( argv[i], "-l" ) ) {
            action = ACTION_LIST;
            action_count++;
        } else if( !strcmp( argv[i], "-g" ) ) {
            nr_string = argv[++i];
            action = ACTION_DOWNLOAD;
            action_count++;
        } else if( !strcmp( argv[i], "-p" ) ) {
            action = ACTION_UPLOAD;
            action_count++;
        } else if( !strcmp( argv[i], "-r" ) ) {
            /* ignoring the info_file is no more needed, because mixed up
             * (=wrong) short/long-name combinations are made impossible now
             */
            action = ACTION_REGENERATE_PLAYLIST;
            action_count++;
        } else if( !strcmp( argv[i], "--init" ) ) {
            ignore_info_file();
            action = ACTION_INITIALIZE_PLAYLIST;
            action_count++;
        } else if( !strcmp( argv[i], "--format" ) ) {
            ignore_info_file();
            action = ACTION_FORMAT;
            action_count++;
        } else {		// more args = files
            sources[source_count++] = argv[i];
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
            /*md9781_close(md9781_handle); */
            printf("communication failed - waiting %d seconds\n", 5 * retries);
            sleep( 5* retries );
            /* md9781_handle = md9781_open();
            if( md9781_handle == NULL )
                abort(); */
            while(!dummy_read(md9781_handle) );
        }
    }

    if( playlist == NULL ) {
        abort();
    }

    if( action == ACTION_DELETE ) {
        if( (nr = md9781_delete_range( md9781_handle, nr_string, location,playlist )) > 0 )
            printf("%d files deleted.\n", nr);
    } else if( action == ACTION_LIST ) {
        md9781_print_playlist(playlist);

    } else if( action == ACTION_DOWNLOAD) {
        if( (nr = md9781_download_range( md9781_handle, nr_string, location,
                                         playlist, print_percent_done )) > 0)
            printf("%d files downloaded. \n", nr);
    } else if( action == ACTION_UPLOAD ) {
        /* put all files on the player (upload) */
        for( i = 0; i < source_count ; i++) {
            printf("Putting %s to the player\n", sources[i]);
            if( md9781_upload_file( md9781_handle, sources[i], location,playlist, print_percent_done) ) {
                printf("File has been uploaded.\n");
            }
        }
    } else if( action == ACTION_REGENERATE_PLAYLIST ) {
        md9781_regenerate_playlist( md9781_handle, location );
    } else if( action == ACTION_INITIALIZE_PLAYLIST ) {
        md9781_init_playlist( md9781_handle, location );
    } else if( action == ACTION_FORMAT ) {
        printf("Formating... (takes some time - do not interrupt)\n");
        md9781_format( md9781_handle, location );
    }

    md9781_freemem_filelist(playlist);
    md9781_close(md9781_handle);
    return 0;
}
