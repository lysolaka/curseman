#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cmanutils.h"

typedef struct A {
  char display;
  int y;
  int x;
  char nextmove;
  char hitflags[4]; //unused for player | 0 means no collision on specified side |
                    //hitflag index: 0 - top, 1 - right, 2 - bottom, 3 - left
} ACTOR;

extern void info_print(int, const char*);
extern void init_str(char*, char*);

void process_ghosts(ACTOR*);
void move_actor(ACTOR*, MAP_W, WINDOW*);
void update_hitflags(ACTOR*, MAP_W);

int main() {
  MAP map;
  MAP_W game;
  FILE* map_f;
  WINDOW* cman;
  int ctrl;

  ACTOR player; player.display = 'C';
  ACTOR ghost[C_GHOSTS];
  for(int i = 0; i < C_GHOSTS; i++) ghost[i].display = '@';
  int ghost_count = 0;

  char filename[MAX_FNAME];
  char path[MAX_PATH];
  int str_i;

  srand(time(0));
  setlocale(LC_ALL, "");
  initscr(); keypad(stdscr, TRUE); curs_set(0); start_color(); noecho();
  init_pair(1, COLOR_BLACK, COLOR_YELLOW);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
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
  // fill in hitflags for ghosts, they will be used to avoid moving ghosts into walls
  for(int i = 0; i < C_GHOSTS; i++) {
    ghost[i].hitflags[0] = 0;
    if(game[ghost[i].y - 1][ghost[i].x] == 1) ghost[i].hitflags[0] = 1;
    ghost[i].hitflags[1] = 0;
    if(game[ghost[i].y][ghost[i].x + 1] == 1) ghost[i].hitflags[1] = 1;
    ghost[i].hitflags[2] = 0;
    if(game[ghost[i].y + 1][ghost[i].x] == 1) ghost[i].hitflags[2] = 1;
    ghost[i].hitflags[3] = 0;
    if(game[ghost[i].y][ghost[i].x - 1] == 1) ghost[i].hitflags[3] = 1;
  }

  halfdelay(5);

  while(ctrl != KEY_F(10)) {
    for(int i = 0; i < C_GHOSTS; i++) update_hitflags(&ghost[i], game);
    wrefresh(cman);
    ctrl = getch();
    switch(ctrl) {
      case KEY_UP:
        player.nextmove = C_UP;
        break;
      case KEY_RIGHT:
        player.nextmove = C_RIGHT;
        break;
      case KEY_DOWN:
        player.nextmove = C_DOWN;
        break;
      case KEY_LEFT:
        player.nextmove = C_LEFT;
        break;
      default:
        player.nextmove = C_STATIONARY;
        break;
    }
    process_ghosts(ghost);
    for(int i = 0; i < C_GHOSTS; i++) {
      move_actor(&ghost[i], game, cman);
    }
    move_actor(&player, game, cman);
  }

  wrefresh(cman);
  getchar();
  endwin();

  return 0;
}

void process_ghosts(ACTOR* ghost) {
  for(int i = 0; i < C_GHOSTS; i++) {
    if(ghost[i].hitflags[0]) { // hitflag - up
      if(rand() % 2) {
        ghost[i].nextmove = C_DOWN;
      } else {
        if(rand() % 2) ghost[i].nextmove = C_LEFT;
        else ghost[i].nextmove = C_RIGHT;
      }
    } else if(ghost[i].hitflags[2]) { // hitflag - down
      // up and down are more probable, because MAP_HEIGHT < MAP_WIDTH
      // makes code spaghetti but we need this order
      if(rand() % 2) {
        ghost[i].nextmove = C_UP;
      } else {
        if(rand() % 2) ghost[i].nextmove = C_LEFT;
        else ghost[i].nextmove = C_RIGHT;
      }
    } else if(ghost[i].hitflags[1]) { // hitflag - right
      if(rand() % 2) {
        ghost[i].nextmove = C_LEFT;
      } else {
        if(rand() % 2) ghost[i].nextmove = C_UP;
        else ghost[i].nextmove = C_DOWN;
      }
    } else if(ghost[i].hitflags[3]) { // hitflag - left
      if(rand() % 2) {
        ghost[i].nextmove = C_RIGHT;
      } else {
        if(rand() % 2) ghost[i].nextmove = C_UP;
        else ghost[i].nextmove = C_DOWN;
      }
    } else {
      switch(rand() % 4) {
        case 0:
          ghost[i].nextmove = C_UP; break;
        case 1:
          ghost[i].nextmove = C_RIGHT; break;
        case 2:
          ghost[i].nextmove = C_DOWN; break;
        case 3:
          ghost[i].nextmove = C_LEFT; break;
      }
    }
  }
}

void move_actor(ACTOR* actor, MAP_W game, WINDOW* win) {
  short color = 3;
  char id = 2; // color - color pair (short), id: 2 - ghost, 3 - player
  if(actor->display == 'C') {
    color = 2; id = 3; // switching those values is just a coincidence
  }
  switch(actor->nextmove) {
    case C_UP:
      if(game[actor->y - 1][actor->x] == 1) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, 0, NULL);
      (actor->y)--;
      game[actor->y][actor->x] = id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, color, NULL);
      break;
    case C_RIGHT:
      if(game[actor->y][actor->x + 1] == 1) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, 0, NULL);
      (actor->x)++;
      game[actor->y][actor->x] = id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, color, NULL);
      break;
    case C_DOWN:
      if(game[actor->y + 1][actor->x] == 1) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, 0, NULL);
      (actor->y)++;
      game[actor->y][actor->x] = id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, color, NULL);
      break;
    case C_LEFT:
      if(game[actor->y][actor->x - 1] == 1) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, 0, NULL);
      (actor->x)--;
      game[actor->y][actor->x] = id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_NORMAL, color, NULL);
      break;
    default:
      break;
  }
}

void update_hitflags(ACTOR* ghost, MAP_W game) {
    ghost->hitflags[0] = 0;
    if(game[ghost->y - 1][ghost->x] == 1) ghost->hitflags[0] = 1;
    ghost->hitflags[1] = 0;
    if(game[ghost->y][ghost->x + 1] == 1) ghost->hitflags[1] = 1;
    ghost->hitflags[2] = 0;
    if(game[ghost->y + 1][ghost->x] == 1) ghost->hitflags[2] = 1;
    ghost->hitflags[3] = 0;
    if(game[ghost->y][ghost->x - 1] == 1) ghost->hitflags[3] = 1;
}
