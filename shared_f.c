#include <ncurses.h>

void info_print(int win_y, const char* message) {
  mvhline(win_y, 0, ' ', getmaxx(stdscr)); //clear legendbar
  mvprintw(win_y, 2, "%s", message);
}

void init_str(char* fname, char* path) {
  const char c_path[8] = "./maps/"; 
  *fname = '\0';
  //dirty?
  for(int i = 0; i < 8; i++) {
    path[i] = c_path[i];
  }
}

