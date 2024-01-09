#include <ncurses.h>
#include "cmanutils.h"

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

void append_str(char* fname, char* path) {
  int str_i = 0;
  while(path[str_i] != '\0') str_i++;
  for(int i = 0; i < MAX_FNAME; i++) path[str_i+i] = fname[i];
}
// put instructions and credits here to avoid messing code
const char* INSTRUCTIONS = "WELCOME TO CURSEMAN!\nYou have been trapped in some sort of a maze with deadly\nghosts. Use arrow keys to run from them.\nBut not all hope is lost! Every so often Curseman will get\nso mad at running away from ghosts, that he will curse them\nand dream of killing them.\nThat dream would last only for a while, but is real.\nThe game ends:\n1. When Curseman dies or,\n2. All ghosts leave Curseman alone and die.\nIn this game the lower the score the better.";
const char* CREDITS = "Inspired by PAC-MAN (arcade game by Capcom)\nProgrammed by ****** ****** (me)\nBEWARE! Unix CURSES technology is being used in this software!";

