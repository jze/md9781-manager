#include "common.h"
#include "libmd9781.h"
#include <stdio.h>
#include <string.h> 

#define INFO_FILE_ENTRY_LENGTH   sizeof( struct info_file_entry)

int main() {
   struct info_file_entry*   playlist;
   
   playlist = (struct info_file_entry* )calloc( 3, INFO_FILE_ENTRY_LENGTH);
   printf("playlist: %p\n", playlist);
   printf("playlist[0]: %p\n", playlist);
   printf("playlist[1]: %p\n", playlist+ INFO_FILE_ENTRY_LENGTH);
   playlist->size = 1234;
   (playlist+INFO_FILE_ENTRY_LENGTH)->size = 2345;
   
   printf("%d\n", playlist->size );
   printf("%d\n", (playlist+INFO_FILE_ENTRY_LENGTH)->size );
   
   return 0;
}
