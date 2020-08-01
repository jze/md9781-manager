#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libmd9781.h"

#define ACTION_DELETE	1
#define ACTION_UPLOAD	2
#define ACTION_DOWNLOAD	3
#define ACTION_LIST	4
#define ACTION_SHOW_PLAYLIST	5
#define ACTION_REGENERATE_PLAYLIST	6

void print_usage() {
    fprintf(stderr, "USAGE: md9781-manager <options> <command>\n" );
    fprintf(stderr, "\nOPTIONS:\n");
    fprintf(stderr, "-e            use smartmedia card\n");
    fprintf(stderr, "\nCOMMANDS:\n");
    fprintf(stderr, "-d <nr>      delete file number nr\n");
    fprintf(stderr, "-l           list files\n");
    fprintf(stderr, "-p <file> [targetname]\n");
    fprintf(stderr, "             put a file to the player\n");
    fprintf(stderr, "             (if targetname is given the file will get that name on the player)\n");
    fprintf(stderr, "-g <nr>      get file number nr and save it in the current directory\n");
}

int main( int argc, char **argv) {
    int i,action_count=0, nr = 0,action;
    struct file_descriptor ** file;
    char location = 'M';
    usb_dev_handle* md9781_handle;
    char* source;
    char* target;
    
    for( i = 1; i < argc; i++ ) {
        if( !strcmp( argv[i], "-e" ) ) {
            location = 'S';
        } else if( !strcmp( argv[i], "-d" ) ) {
		char* nr_string = argv[++i];
		action = ACTION_DELETE;
		action_count += 1;
        	nr = atoi( nr_string );
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
    		target = argv[++i];
        } else if( !strcmp( argv[i], "-lpl" ) ) {
		action = ACTION_SHOW_PLAYLIST;
		action_count++;
	} else if( !strcmp( argv[i], "-r" ) ) {
		action = ACTION_REGENERATE_PLAYLIST;
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
        fprintf(stderr, "md9781_open failed\n");
        return 1;
    }
    
#ifdef DEBUG
    printf("getting filelist\n");
#endif
    file = md9781_file_list(md9781_handle, location);

    if( action == ACTION_DELETE ) {
            if( nr > 0 ) {
                if( file[nr] == NULL )
                    fprintf(stderr, "File #%d not on player.\n", nr );
                else {
                    printf("Deleting file #%d (%s)...\n", nr, file[nr]->name);
                    if( md9781_delete_file( md9781_handle , nr, location ) )
                        printf("File #%d (%s) deleted.\n", nr, file[nr]->name);
                }
            }
        } else if( action == ACTION_LIST ) {
            int j = 0;
	    long sum_filesize = 0;
            /* list files on player */
            printf("+-----+--------------+----------+------------+----------+\n");
            printf("|   # | Filename     | Size     | Date       | Time     |\n");
            printf("+-----+--------------+----------+------------+----------+\n");

            while( file[j] != NULL ) {
                printf("| %3d ", j );
                printf("| %-12.12s ", file[j]->name );
                printf("| %8ld ", file[j]->size );
                printf("| %02d-%02d-%02d ", file[j]->year, file[j]->month,file[j]->day );
                printf("| %02d:%02d:%02d ", file[j]->hour, file[j]->minute, file[j]->second );
                printf("|\n");
		sum_filesize += file[j]->size;
                j++;
            }

            printf("+-----+--------------+----------+------------+----------+\n");
	    printf("| Used: %5.0f kB /", sum_filesize / 1024.0 );
	    printf(" Free: %5.0f kB (%2.0f %%)                |\n", 
	                  64.0 * 1024.0 - sum_filesize / 1024.0 ,
			  100.0 - sum_filesize / (64.0 * 1024.0 * 1024.0) * 100.0 );
            printf("+-------------------------------------------------------+\n");
        } else if( action == ACTION_DOWNLOAD) {
            /* get a file from the player (download) */
            if( nr > 0 ) {
                file = md9781_file_list(md9781_handle, location);
                if( file[nr] == NULL )
                    fprintf(stderr, "File #%d not on player.\n", nr );
                else {
                    printf("Getting file #%d (%s)...\n", nr, file[nr]->name);
                    if( md9781_download_file( md9781_handle , file[nr]->name, nr, location ) )
                        printf("File #%d (%s) saved.\n", nr, file[nr]->name);
                }
            }
        } else if( action == ACTION_UPLOAD ) {
            /* put a file on the player (upload) */

                file = md9781_file_list(md9781_handle, location);
                printf("Putting %s to the player\n", source);
                if( md9781_upload_file( md9781_handle, source, location) ) {
                    printf("File has been uploaded.\n");
                }
        } else if( action == ACTION_SHOW_PLAYLIST ) {
		int j;
		struct info_file_entry** playlist = md9781_play_list( md9781_handle, location);
		printf("The playlist:\n");
		for( j = 0; playlist[j] != NULL ; j++ ) 
		       printf("%d\t%s\t%ld\n", j,playlist[j]->name, playlist[j]->size );
	} else if( action == ACTION_REGENERATE_PLAYLIST ) {
	   md9781_regenerate_playlist( md9781_handle, location );
	}


    md9781_close(md9781_handle);
    return 0;
}
