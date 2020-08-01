extern "C" {
#include "libmd9781.h"
}

#include <stdio.h>
#include <string.h>

class Md9781 {
public:
  Md9781() {
    dh = md9781_open();		// §Fehler?
    location = 'M';
    playlist = NULL;
    callback = NULL;
  }
  ~Md9781() {
    if(playlist)
      md9781_freemem_filelist(playlist);
    md9781_close(dh);
  }
  void set_location(bool extern_card) {
    char old_loc = location;
    if(extern_card)
      location = 'S';
    else
      location = 'M';
    
    if(playlist && (old_loc != location)) {
      md9781_freemem_filelist(playlist);
      playlist = md9781_file_list(dh, location);
    }
  }
  void set_callback(void (*fp)(int percent_done)) {
    callback = fp;
  }
  void printlist() {
    get_playlist();
    md9781_print_playlist( playlist );
  }
  int delete_file(int nr) {
    get_playlist();
    return md9781_delete_file(dh, nr, location, playlist);
  }
  int delete_range(const char *range) {
    int ret;
    char *tr = strdup(range);
    get_playlist();
    ret = md9781_delete_range(dh, tr, location, playlist);
    free(tr);
    return ret;
  }
  int download_file(int nr, char *filename) {
    get_playlist();
    if(filename == 0)
      filename = (char*) md9781_entry_number( playlist, nr)->long_name;
    
    return md9781_download_file(dh, filename, nr, location,
				playlist, callback);
  }
  int download_range(const char *range) {
    int ret;
    char *tr = strdup(range);
    get_playlist();
    ret = md9781_download_range(dh, tr, location, playlist, callback);
    free(tr);
    return ret;
  }
  int upload_file(char *filename) {
    get_playlist();
    return md9781_upload_file(dh, filename, location, playlist, callback);
  }
  // number of real (not internal info) files
  int number_of_files() {
    get_playlist();
    return md9781_number_of_files(playlist) - 1;
  }
  int format() {
    return md9781_format(dh, location);
  }
  // § regen, init, ignore info ?
  
protected:
  md9781_entry* get_playlist() {
    if(!playlist)
      playlist = md9781_file_list(dh, location);
    return playlist;
  }
  
  usb_dev_handle *dh;
  md9781_entry *playlist;
  char location;
  void (*callback)(int );
};




int main() {
  Md9781 n;

  n.set_callback(0);
  n.set_location(0);
  n.printlist();
  n.set_location(1);
  n.printlist();
  

  /*
  n.set_callback(0);
  n.number_of_files();
  n.printlist();
  n.upload_file("test.mp3");
  n.delete_file(1);
  n.printlist();
  n.set_location(1);
  n.delete_range("45-");
  n.download_file(1, 0);
  n.download_range("-");
  */
  
  return 0;
}
