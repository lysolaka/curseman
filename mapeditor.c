#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include "cmanutils.h"

struct COORDS {
  int y; int x;
};

struct R_FLAGS {
  char sp; char gsp;
};

void editor_print(WINDOW*);
void legendbar_print(int);
void legend_print(int, char);
void info_print(int, const char*);
void init_load(char*, char*);
void map_print(WINDOW*, MAP);

int main() {
  setlocale(LC_ALL, "");
  struct COORDS cur;
  struct R_FLAGS flags; flags.sp = 0; flags.gsp = 0;
  int ctrl = (int) 'r';
  int id;
  WINDOW* editor;
  MAP map = {0};
  FILE* map_f;
  char path[16];
  char filename[9];

  initscr(); keypad(stdscr, TRUE); curs_set(0); start_color(); noecho();
  init_pair(1, COLOR_BLACK, COLOR_YELLOW);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_BLACK, COLOR_RED);
  init_pair(5, COLOR_BLACK, COLOR_WHITE);
  
  if(getmaxy(stdscr) < 40 || getmaxx(stdscr) < 80) {
    endwin();
    printf("Unsupported screen size.\n Minumum is 40x80 columns.");
    fflush(stdin);
    getchar();
    return -1;
  }

  mvprintw(0, 2, "CURSEMAN MAP EDITOR");
  mvchgat(0, 0, -1, A_NORMAL, 1, NULL);
  legend_print(MAP_HEIGHT + 5, 'n');
  legendbar_print(MAP_HEIGHT + 3);

  editor = newwin(MAP_HEIGHT+2, MAP_WIDTH+2, 1, getmaxx(stdscr)/2 - MAP_WIDTH/2);
  editor_print(editor);

  cur.y = 1; cur.x = 1;
  curs_set(2);
  while(ctrl != KEY_F(10)) {
    wmove(editor, cur.y, cur.x); // addch moves cursor to the right, need to move it back
    legend_print(MAP_HEIGHT + 5, 'n');
    wrefresh(stdscr); wrefresh(editor);
    ctrl = getch();
    legendbar_print(MAP_HEIGHT + 3);
    wmove(editor, cur.y, cur.x);
    wrefresh(editor);
    switch(ctrl) {
      //movement
      case KEY_UP:
        if(cur.y - 1 < 1) break; // protection from moving off screen (causes softlock)
        cur.y--;
        break;
      case KEY_DOWN:
        if(cur.y + 1 > MAP_HEIGHT) break;
        cur.y++;
        break;
      case KEY_LEFT:
        if(cur.x - 1 < 1) break;
        cur.x--;
        break;
      case KEY_RIGHT:
        if(cur.x + 1 > MAP_WIDTH) break;
        cur.x++;
        break;
      //placement
      case (int)'s':
        if(map[cur.y - 1][cur.x - 1]) {
          waddch(editor, ' '); map[cur.y - 1][cur.x - 1] = 0;//addch moves cursor to the right, moving it back
        } else {
          waddch(editor, '#'); map[cur.y - 1][cur.x - 1] = 1;
        }
        break;
      case (int)'g':
        if(map[cur.y - 1][cur.x - 1] || flags.gsp == 3) { //want to fail for every non-empty space
          wattron(stdscr, COLOR_PAIR(4));
          info_print(MAP_HEIGHT + 3, "Placement error!");
          wattroff(stdscr, COLOR_PAIR(4));
          break;
        } else {
          waddch(editor, '@'); flags.gsp++; map[cur.y - 1][cur.x - 1] = 2; break;
        }
      case (int)'c':
        if(map[cur.y - 1][cur.x - 1] || flags.sp == 1) {
          wattron(stdscr, COLOR_PAIR(4));
          info_print(MAP_HEIGHT + 3, "Placement error!");
          wattroff(stdscr, COLOR_PAIR(4));
          break;
        } else {
          waddch(editor, '$'); flags.sp++; map[cur.y - 1][cur.x - 1] = 3; break;
        }
      //interaction
      case KEY_F(1):
        //reset vars
        id = 0;
        init_load(filename, path);

        legend_print(MAP_HEIGHT + 5, 'i');
        wattron(stdscr, COLOR_PAIR(5));
        info_print(MAP_HEIGHT + 3, "Filename to load (8 char incl. '.bin'): ./maps/");
        echo();
        mvwgetnstr(stdscr, MAP_HEIGHT + 3, 49, filename, 8);
        noecho();
        wattroff(stdscr, COLOR_PAIR(5));
        //quick strcat no need for libraries
        while(path[id] != '\0') id++;
        for(int i = 0; i < 9; i++) { //no brain juice for size security, always assume 8 char fname
          path[id+i] = filename[i];
        }

        map_f = fopen("./maps/test.bin", "rb");
        if(map_f == NULL) {
          wattron(stdscr, COLOR_PAIR(4));
          info_print(MAP_HEIGHT + 3, "Specified file doesn't exist!");
          wattroff(stdscr, COLOR_PAIR(4));
        } else if(fread(&map, sizeof(map[0][0]), sizeof(map), map_f) < sizeof(map)) {
          wattron(stdscr, COLOR_PAIR(4));
          info_print(MAP_HEIGHT + 3, "Unknown loading error!"); 
          wattroff(stdscr, COLOR_PAIR(4));
        } else {
          wattron(stdscr, COLOR_PAIR(2));
          info_print(MAP_HEIGHT + 3, "File loaded successfully!");
          wattroff(stdscr, COLOR_PAIR(2));
          map_print(editor, map);          
        }
        break;
      default:
        break;
    }
  }
  fclose(map_f);
  endwin();
  return 0;
}

void editor_print(WINDOW* win) {
  keypad(win, TRUE);
  wattron(win, COLOR_PAIR(3)); 
  wborder(win, ACS_BOARD, ACS_BOARD, ACS_BOARD, ACS_BOARD, ACS_BOARD, ACS_BOARD, ACS_BOARD, ACS_BOARD);
  wattroff(win, COLOR_PAIR(3));
}

void legendbar_print(int win_y) {
  mvprintw(win_y,  0, "  # - WALL  $ - SPAWNPOINT  @ - GHOST SPAWNPOINTS (3 required)");
  mvchgat(win_y, 0, -1, A_NORMAL, 1, NULL);
}

void legend_print(int win_y, char mode) {
  if(mode == 'n') {
    //line 1
    mvhline(win_y, 0, ' ', getmaxx(stdscr)); // Clear lines first
    mvhline(win_y + 1, 0, ' ', getmaxx(stdscr));
    mvprintw(win_y, 2, "F1 Load file");
    mvaddch(win_y, 16, ACS_LARROW); mvaddch(win_y, 17, ACS_RARROW); mvaddch(win_y, 18, ACS_UARROW); mvaddch(win_y, 19, ACS_DARROW);
    mvprintw(win_y, 21, "Move");
    mvprintw(win_y, 27, "G Place Ghost");
    mvprintw(win_y, 42, "F10 Exit");

    mvchgat(win_y, 2, 2, A_NORMAL, 1, NULL);
    mvchgat(win_y, 16, 4, A_ALTCHARSET, 1, NULL);
    mvchgat(win_y, 27, 1, A_NORMAL, 1, NULL);
    mvchgat(win_y, 42, 3, A_NORMAL, 1, NULL);
    //line 2
    mvprintw(win_y + 1, 2, "F2 Save buffer");
    mvprintw(win_y + 1, 18, "S Place/Remove");
    mvprintw(win_y + 1, 34, "C Place Spawnpoint");

    mvchgat(win_y + 1, 2, 2, A_NORMAL, 1, NULL);
    mvchgat(win_y + 1, 18, 1, A_NORMAL, 1, NULL);
    mvchgat(win_y + 1, 34, 1, A_NORMAL, 1, NULL);
  } else if(mode == 'i') {
    mvhline(win_y, 0, ' ', getmaxx(stdscr)); // Clear lines first
    mvhline(win_y + 1, 0, ' ', getmaxx(stdscr));
    mvprintw(win_y, 2, "^H Remove 1 character");
    mvprintw(win_y + 1, 2, "^U Clear all characters");

    mvchgat(win_y, 2, 2, A_NORMAL, 1, NULL);
    mvchgat(win_y + 1, 2, 2, A_NORMAL, 1, NULL);
  }
}

void info_print(int win_y, const char* message) {
  mvhline(win_y, 0, ' ', getmaxx(stdscr));
  mvprintw(win_y, 2, "%s", message);
}

void init_load(char* fname, char* path) {
  const char c_path[8] = "./maps/"; 
  *fname = '\0';
  //dirty?
  for(int i = 0; i < 8; i++) {
    path[i] = c_path[i];
  }
}

void map_print(WINDOW* win, MAP map) {
  for(int y = 0; y < MAP_HEIGHT; y++) {
    for(int x = 0; x < MAP_WIDTH; x++) {
      if(map[y][x] == 1) mvwaddch(win, y+1, x+1, '#');
      else if(map[y][x] == 2) mvwaddch(win, y+1, x+1, '@');
      else if(map[y][x] == 3) mvwaddch(win, y+1, y+1, '$');
    }
  }
}
