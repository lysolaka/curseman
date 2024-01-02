#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include "cmanutils.h"

typedef struct A {
  char display;
  int y;
  int x;
  char nextmove;
} ACTOR;

extern void info_print(int, const char*);
extern void init_str(char*, char*);

int main() {
  MAP map;
  MAP_W game;
  FILE* map_f;
  WINDOW* cman;
  int ctrl;

  ACTOR player; player.display = 'C';
  ACTOR ghost[3]; ghost[0].display = '@'; ghost[1].display = '@'; ghost[2].display = '@';
  int ghost_count = 0;

  char filename[MAX_FNAME];
  char path[MAX_PATH];
  int str_i;

  setlocale(LC_ALL, "");
  initscr(); keypad(stdscr, TRUE); curs_set(0); start_color(); noecho();
  init_pair(1, COLOR_BLACK, COLOR_YELLOW);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
//screen size stuff
  if(getmaxy(stdscr) < 40 || getmaxx(stdscr) < 80) {
    endwin();
    printf("Unsupported screen size.\nMinumum is 40x80 columns.");
    fflush(stdin);
    getchar();
    return -1;
  }
//"title screen"
  mvprintw(0, 2, "CURSEMAN");
  mvchgat(0, 0, -1, A_NORMAL, 1, NULL);
  attron(COLOR_PAIR(1));
  info_print(getmaxy(stdscr) - 5, "Filename to load (8 char incl. '.bin'): ./maps/");
  refresh();
  curs_set(1); echo();
  init_str(filename, path);
  mvgetnstr(getmaxy(stdscr) - 5, 49, filename, MAX_FNAME - 1);
  noecho(); curs_set(0);
  attroff(COLOR_PAIR(1));

//append filename to path
  str_i = 0;
  while(path[str_i] != '\0') str_i++;
  for(int i = 0; i < MAX_FNAME; i++) { //no brain juice for size security, always assume 8 char fname
    path[str_i+i] = filename[i];
  }

//game area
  cman = newwin(MAP_HEIGHT + 2, MAP_WIDTH + 2, getmaxy(stdscr)/2 - 3 - MAP_HEIGHT/2, getmaxx(stdscr)/2 - MAP_WIDTH/2);

  map_f = fopen(path, "rb");
  fread(&map, sizeof(map[0][0]), sizeof(map), map_f);

  //copy to game with a border around
  for(int y = 0; y < MAP_HEIGHT + 2; y++) {
    for(int x = 0; x < MAP_WIDTH + 2; x++) {
      if(y == 0 || y == MAP_HEIGHT + 1) game[y][x] = 1;
      else if(x == 0 || x == MAP_WIDTH + 1) game[y][x] = 1;
      else game[y][x] = map[y-1][x-1];
    }
  }

  //box: ulcorner
  mvwaddch(cman, 0, 0, ACS_ULCORNER);
  //box: upper side
  for(int x = 1; x < MAP_WIDTH + 1; x++) {
    if(game[1][x] == 1) mvwaddch(cman, 0, x, ACS_BSSS);
    else mvwaddch(cman, 0, x, ACS_HLINE);
  }
  //box: urcorner
  mvwaddch(cman, 0, MAP_WIDTH + 1, ACS_URCORNER);
  //box: left side
  for(int y = 1; y < MAP_HEIGHT + 1; y++) {
    if(game[y][1] == 1) mvwaddch(cman, y, 0, ACS_SSSB);
    else mvwaddch(cman, y, 0, ACS_VLINE);
  }
  //box: right side
  for(int y = 1; y < MAP_HEIGHT + 1; y++) {
    if(game[y][MAP_WIDTH] == 1) mvwaddch(cman, y, MAP_WIDTH + 1, ACS_SBSS);
    else mvwaddch(cman, y, MAP_WIDTH + 1, ACS_VLINE);
  }
  //box: llcorner
  mvwaddch(cman, MAP_HEIGHT + 1, 0, ACS_LLCORNER);
  //box: lower side
  for(int x = 1; x < MAP_WIDTH + 1; x++) {
    if(game[MAP_HEIGHT][x] == 1) mvwaddch(cman, MAP_HEIGHT + 1, x, ACS_SSBS);
    else mvwaddch(cman, MAP_HEIGHT + 1, x, ACS_HLINE);
  }
  //box: lrcorner
  mvwaddch(cman, MAP_HEIGHT + 1, MAP_WIDTH + 1, ACS_LRCORNER);

  //fill the map in
  for(int y = 1; y < MAP_HEIGHT + 1; y++) {
    for(int x = 1; x < MAP_WIDTH + 1; x++) {
      if(game[y][x] == 1) { // top, right, bottom, left
        //IMPORTANT! : change == 0 to != 1
        if(game[y-1][x] != 1 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] != 1) mvwaddch(cman, y, x, ACS_BSSB);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] != 1 && game[y][x-1] != 1) mvwaddch(cman, y, x, ACS_SSBB);
        else if(game[y-1][x] != 1 && game[y][x+1] != 1 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_BBSS);
        else if(game[y-1][x] == 1 && game[y][x+1] != 1 && game[y+1][x] != 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SBBS);
        else if(game[y-1][x] == 1 && game[y][x+1] != 1 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SBSS);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] != 1) mvwaddch(cman, y, x, ACS_SSSB);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] != 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SSBS);
        else if(game[y-1][x] != 1 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_BSSS);
        else if(game[y-1][x] != 1 && game[y][x+1] == 1 && game[y+1][x] != 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_BSBS);
        else if(game[y-1][x] == 1 && game[y][x+1] != 1 && game[y+1][x] == 1 && game[y][x-1] != 1) mvwaddch(cman, y, x, ACS_SBSB);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SSSS);
        else if(game[y-1][x] == 1 && game[y][x+1] != 1 && game[y+1][x] != 1 && game[y][x-1] != 1) mvwaddch(cman, y, x, ACS_VLINE);
        else if(game[y-1][x] != 1 && game[y][x+1] == 1 && game[y+1][x] != 1 && game[y][x-1] != 1) mvwaddch(cman, y, x, ACS_HLINE);
        else if(game[y-1][x] != 1 && game[y][x+1] != 1 && game[y+1][x] == 1 && game[y][x-1] != 1) mvwaddch(cman, y, x, ACS_VLINE);
        else if(game[y-1][x] != 1 && game[y][x+1] != 1 && game[y+1][x] != 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_HLINE);
      }
      if(game[y][x] == 3) { //find player spawn
        player.y = y; player.x = x;
      }
      if(game[y][x] == 2 && ghost_count < 4) { //find ghosts spawn
        ghost[ghost_count].y = y;
        ghost[ghost_count].x = x;
        ghost_count++;
      }
    }
  }

  halfdelay(5);

  while(ctrl != KEY_F(10)) {
    wrefresh(cman);
    ctrl = getch();
    switch(ctrl) {
      case KEY_UP:
        mvwaddch(cman, player.y, player.x, player.display);
        player.y--;
        break;
      default:
        mvwaddch(cman, ghost[0].y, ghost[0].x, ghost[0].display);
        ghost[0].y--;
        break;
    }
  }

  wrefresh(cman);
  getchar();
  endwin();

  return 0;
}
