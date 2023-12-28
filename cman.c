#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include "cmanutils.h"

int main() {
  MAP map;
  MAP_W game;
  FILE* map_f;
  WINDOW* cman;

  setlocale(LC_ALL, "");
  initscr(); keypad(stdscr, TRUE); curs_set(0); start_color(); noecho();
  init_pair(1, COLOR_BLACK, COLOR_YELLOW);
//screen size stuff
  if(getmaxy(stdscr) < 40 || getmaxx(stdscr) < 80) {
    endwin();
    printf("Unsupported screen size.\nMinumum is 40x80 columns.");
    fflush(stdin);
    getchar();
    return -1;
  }
//title
  mvprintw(0, 2, "CURSEMAN");
  mvchgat(0, 0, -1, A_NORMAL, 1, NULL);
  refresh();
//game area
  cman = newwin(MAP_HEIGHT + 2, MAP_WIDTH + 2, getmaxy(stdscr)/2 - 3 - MAP_HEIGHT/2, getmaxx(stdscr)/2 - MAP_WIDTH/2);

  map_f = fopen("./maps/mapp.bin", "rb");
  fread(&map, sizeof(map[0][0]), sizeof(map), map_f);

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
        if(game[y-1][x] == 0 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] == 0) mvwaddch(cman, y, x, ACS_BSSB);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] == 0 && game[y][x-1] == 0) mvwaddch(cman, y, x, ACS_SSBB);
        else if(game[y-1][x] == 0 && game[y][x+1] == 0 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_BBSS);
        else if(game[y-1][x] == 1 && game[y][x+1] == 0 && game[y+1][x] == 0 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SBBS);
        else if(game[y-1][x] == 1 && game[y][x+1] == 0 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SBSS);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] == 0) mvwaddch(cman, y, x, ACS_SSSB);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] == 0 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SSBS);
        else if(game[y-1][x] == 0 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_BSSS);
        else if(game[y-1][x] == 0 && game[y][x+1] == 1 && game[y+1][x] == 0 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_BSBS);
        else if(game[y-1][x] == 1 && game[y][x+1] == 0 && game[y+1][x] == 1 && game[y][x-1] == 0) mvwaddch(cman, y, x, ACS_SBSB);
        else if(game[y-1][x] == 1 && game[y][x+1] == 1 && game[y+1][x] == 1 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_SSSS);
        else if(game[y-1][x] == 1 && game[y][x+1] == 0 && game[y+1][x] == 0 && game[y][x-1] == 0) mvwaddch(cman, y, x, ACS_VLINE);
        else if(game[y-1][x] == 0 && game[y][x+1] == 1 && game[y+1][x] == 0 && game[y][x-1] == 0) mvwaddch(cman, y, x, ACS_HLINE);
        else if(game[y-1][x] == 0 && game[y][x+1] == 0 && game[y+1][x] == 1 && game[y][x-1] == 0) mvwaddch(cman, y, x, ACS_VLINE);
        else if(game[y-1][x] == 0 && game[y][x+1] == 0 && game[y+1][x] == 0 && game[y][x-1] == 1) mvwaddch(cman, y, x, ACS_HLINE);
        else mvwaddch(cman, y, x, ACS_DIAMOND);
     }
    }
  }


  wrefresh(cman);
  getchar();
  endwin();

  return 0;
}
