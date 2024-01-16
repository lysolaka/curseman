#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cmanutils.h"

typedef struct A {
  char display;
  unsigned char id; // 2 - ghost, 3 - player
  short color; // default color of the actor
  int y;
  int x;
  char nextmove;
  char hitflags[4]; //now used for player too! | 0 means no collision on specified side |
                    //hitflag index: 0 - top, 1 - right, 2 - bottom, 3 - left
  char enabled; //determines if actor is alive, 0 - dead, 1 - alive
} ACTOR;

extern void info_print(int, const char*);
extern void init_str(char*, char*);
extern void append_str(char*, char*); // filename, path
void legend_print(int, char);

void print_map(WINDOW*, ACTOR*, ACTOR*, MAP_W, int*);
char process_ghost(ACTOR*);
void move_actor(ACTOR*, MAP_W, WINDOW*);
void update_hitflags(ACTOR*, MAP_W);

int main() {
  MAP map;
  MAP_W game;
  FILE* map_f;
  WINDOW* cman;
  WINDOW* text;
  int ctrl;
  int ctrl_g;
  char map_loaded; // could really use a bool but ok
  extern const char* INSTRUCTIONS;
  extern const char* CREDITS;

  ACTOR player;
  ACTOR ghost[C_GHOSTS];
  int ghost_count;

  short c_gmode;
  int c_mcounter;
  char c_quitmsg;
  int c_scorepad;
  int c_counterpad;
  long c_score;

  char filename[MAX_FNAME];
  char path[MAX_PATH];

  srand(time(0));
  setlocale(LC_ALL, "");

  initscr(); keypad(stdscr, TRUE); curs_set(0); start_color(); noecho();
  init_pair(1, COLOR_BLACK, COLOR_YELLOW);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  init_pair(5, COLOR_BLACK, COLOR_RED);
  init_pair(6, COLOR_BLACK, COLOR_GREEN);
  init_pair(7, COLOR_BLACK, COLOR_WHITE);
  init_pair(8, COLOR_GREEN, COLOR_BLACK);
//screen size stuff
  if(getmaxy(stdscr) < 40 || getmaxx(stdscr) < 80) {
    endwin();
    printf("Unsupported screen size.\nMinumum is 40x80 columns.");
    fflush(stdin);
    getchar();
    return -1;
  }
  // init variables, this is here because there was a goto here before to restart the game, but...

  cman = newwin(MAP_HEIGHT + 2, MAP_WIDTH + 2, getmaxy(stdscr)/2 - 3 - MAP_HEIGHT/2, getmaxx(stdscr)/2 - MAP_WIDTH/2); // some clever alignment
  text = newwin(MAP_HEIGHT + 2, MAP_WIDTH + 2, getmaxy(stdscr)/2 - 3 - MAP_HEIGHT/2, getmaxx(stdscr)/2 - MAP_WIDTH/2); // new window to not overwrite possible loaded map
  map_f = NULL;
  ctrl = 0;
  ctrl_g = 0;
  map_loaded = 0;

  player.display = 'C'; player.color = 2; player.id = 3; // player color could also be C_NORMAL defined as 2
  for(int i = 0; i < C_GHOSTS; i++) {
    ghost[i].display = '@';
    ghost[i].color = 3;
    ghost[i].id = 2; // this switch from the player is just a coincidence
  }
  ghost_count = 0;

  c_gmode = C_NORMAL;
  c_mcounter = 0;
  c_quitmsg = 0;
  c_scorepad = getmaxy(stdscr) - 3; // this is a late addition to the code, so that we won't need to call the getmaxy function while the game is running, this being a late addition means I won't update all the code to use this since there are no performance gains there
  c_counterpad = getmaxy(stdscr) - 2;
  c_score = 0;

//"title screen"
  mvprintw(0, 2, "CURSEMAN");
  mvchgat(0, 0, -1, A_NORMAL, 1, NULL);
  attron(COLOR_PAIR(1));
  info_print(getmaxy(stdscr) - 5, "Welcome to CURSEMAN");
  attroff(COLOR_PAIR(1));

  while(ctrl != KEY_F(10)) {
    legend_print(getmaxy(stdscr) - 3, 'm');
    refresh();
    ctrl = getch();

    attron(COLOR_PAIR(1));
    info_print(getmaxy(stdscr) - 5, "Welcome to CURSEMAN");
    attroff(COLOR_PAIR(1));
    refresh();

    switch(ctrl) {
      case KEY_F(1): // Load map
        // get the input
        legend_print(getmaxy(stdscr) - 3, 'i');
        attron(COLOR_PAIR(7));
        info_print(getmaxy(stdscr) - 5, "Filename to load (8 char incl. '.map'): ./maps/");
        refresh();
        curs_set(1); echo();
        init_str(filename, path);
        mvgetnstr(getmaxy(stdscr) - 5, 49, filename, MAX_FNAME - 1);
        noecho(); curs_set(0);
        attroff(COLOR_PAIR(7));
        // append filename to path
        append_str(filename, path);
        map_f = fopen(path, "rb");
        if(map_f == NULL) {
          attron(COLOR_PAIR(5));
          info_print(getmaxy(stdscr) - 5, "Specified file doesn't exist!");
          attroff(COLOR_PAIR(5));
        } else if(fread(&map, sizeof(map[0][0]), sizeof(map), map_f) < sizeof(map)) {
          attron(COLOR_PAIR(5));
          info_print(getmaxy(stdscr) - 5, "Unknown loading error!");
          attroff(COLOR_PAIR(5));
        } else {
          map_loaded = 1;
          //copy to game with a border around
          for(int y = 0; y < MAP_HEIGHT + 2; y++) {
            for(int x = 0; x < MAP_WIDTH + 2; x++) {
              if(y == 0 || y == MAP_HEIGHT + 1) game[y][x] = 1;
              else if(x == 0 || x == MAP_WIDTH + 1) game[y][x] = 1;
              else game[y][x] = map[y-1][x-1];
            }
          }
          print_map(cman, &player, ghost, game, &ghost_count);
          // add player and ghosts to the screen
          wattron(cman, COLOR_PAIR(player.color) | A_BOLD);
          mvwaddch(cman, player.y, player.x, player.display);
          wattroff(cman, COLOR_PAIR(player.color)); // let the bold carry over
          wattron(cman, COLOR_PAIR(ghost[0].color)); // all ghost have the same colour
          for(int i = 0; i < C_GHOSTS; i++) mvwaddch(cman, ghost[i].y, ghost[i].x, ghost[i].display);
          wattroff(cman, COLOR_PAIR(ghost[0].color) | A_BOLD);
          wrefresh(cman);
          attron(COLOR_PAIR(6));
          info_print(getmaxy(stdscr) - 5, "Map loaded successfully");
          attroff(COLOR_PAIR(6));
        }
        if(map_f != NULL) fclose(map_f);
        break;
      case KEY_F(2): // Display instructions
        wclear(text);
        wprintw(text, "%s", INSTRUCTIONS);
        wrefresh(text);
        break;
      case KEY_F(3): // Display credits
        wclear(text);
        wprintw(text, "%s", CREDITS);
        wrefresh(text);
        break;
      case KEY_F(5): // Show cman window
        wclear(text);
        wrefresh(text);
        touchwin(cman); // NCURSES (TM) magic finger
        wnoutrefresh(cman);
        wrefresh(cman);
        break;
      case KEY_F(4): // GAME TIME!
        if(map_loaded != 1) {
          attron(COLOR_PAIR(5));
          info_print(getmaxy(stdscr) - 5, "Load a map first before playing!");
          attroff(COLOR_PAIR(5));
          break;
        }
        // restore cman window in case someone didn't do F5
        wclear(text);
        wrefresh(text);
        touchwin(cman);
        wnoutrefresh(cman);
        wrefresh(cman);
        // fill in C_STATIONARY into ghosts to initialize their movement rng
        // fill in enabled flag
        for(int i = 0; i < C_GHOSTS; i++) {
          ghost[i].nextmove = C_STATIONARY;
          ghost[i].enabled = 1;
        }
        player.enabled = 1;
        // some printouts
        attron(COLOR_PAIR(1));
        info_print(getmaxy(stdscr) - 5, "Playing...");
        attroff(COLOR_PAIR(1));
        legend_print(getmaxy(stdscr) - 3, 'g');
        mvprintw(c_scorepad, 44, "Score:"); // print this after the legend to avoid getting erased
        mvprintw(c_counterpad, 44, "Dreamer meter:");
        // let's play already ahh
        halfdelay(3);

        while(ctrl_g != KEY_F(10)) {
          refresh();
          if(c_gmode == C_DREAMER) {
            for(int i = 0; i < C_GHOSTS; i++) { // because of my 'smart' decision to implement ghosts as ACTOR[3] we have this, because we can't check around the player all the time since ghost are not distinguishable
              if((game[ghost[i].y - 1][ghost[i].x] == 3 || game[ghost[i].y][ghost[i].x + 1] == 3 || game[ghost[i].y + 1][ghost[i].x] == 3 || game[ghost[i].y][ghost[i].x - 1] == 3) && ghost[i].enabled) {
                ghost[i].enabled = 0;
                mvwaddch(cman, ghost[i].y, ghost[i].x, ' ');
                game[ghost[i].y][ghost[i].x] = 0;
                ghost_count--; // will be used for win/loss condition
                c_score = c_score - 100;
              }
            }
          } else { // I think that we won't get any values other than the two defined
            if(game[player.y - 1][player.x] == 2 || game[player.y][player.x + 1] == 2 || game[player.y + 1][player.x] == 2 || game[player.y][player.x - 1] == 2) {
              player.enabled = 0;
            }
          }

          for(int i = 0; i < C_GHOSTS; i++) update_hitflags(&ghost[i], game);
          update_hitflags(&player, game);
          wrefresh(cman);

          ctrl_g = getch(); // really didn't want to separete it from the stuff
          // win/loss check needs to be here for easy interruption with KEY_F(10)
          if(player.enabled == 0) {
            ctrl_g = KEY_F(10);
            c_quitmsg = 1;
          } else if(ghost_count == 0) {
            ctrl_g = KEY_F(10);
            c_quitmsg = 2;
          }

          switch(ctrl_g) {
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
          // separate loops to ensure proper sequence of events
          for(int i = 0; i < C_GHOSTS; i++) {
            if(ghost[i].enabled) ghost[i].nextmove = process_ghost(&ghost[i]);
          }
          // check for mode changes to make them display in time
          if(c_mcounter > 150 && c_gmode == C_NORMAL) {
            c_mcounter = 0;
            c_gmode = C_DREAMER;
            player.color = C_DREAMER;
          } else if(c_mcounter > 100 && c_gmode == C_DREAMER) {
            c_mcounter = 0;
            c_gmode = C_NORMAL;
            player.color = C_NORMAL;
            // reset the meter display
            mvaddch(c_counterpad, 59, ' '); // no need to reset colour, because the C_NORMAL meter starts green
          }
          // mode meter display (yes I've just called it mode, but I can't think of a better name
          if(c_gmode == C_NORMAL) {
            // make this look like part of the legend
            if(c_mcounter > 45) {
              mvaddch(c_counterpad, 59, ACS_BLOCK);
              mvchgat(c_counterpad, 59, 1, A_ALTCHARSET, 8, NULL); // 8 - green
            }
            if(c_mcounter > 90) {
              mvaddch(c_counterpad, 60, ACS_BLOCK);
              mvchgat(c_counterpad, 59, 2, A_ALTCHARSET, 2, NULL); // 2 - yellow
            }
            if(c_mcounter > 135) {
              mvaddch(c_counterpad, 61, ACS_BLOCK);
              mvchgat(c_counterpad, 59, 3, A_ALTCHARSET, 4, NULL); // 4 - red, ik those colours are all over the place
            }
          }
          if(c_gmode == C_DREAMER) {
            if(c_mcounter > 30) {
              mvaddch(c_counterpad, 61, ' ');
              mvchgat(c_counterpad, 59, 2, A_ALTCHARSET, 2, NULL); // remove last block, change colour to yellow
            }
            if(c_mcounter > 60) {
              mvaddch(c_counterpad, 60, ' ');
              mvchgat(c_counterpad, 59, 1, A_ALTCHARSET, 8, NULL);
            }
          }
          // score display
          mvprintw(c_scorepad, 51, "%ld", c_score);
          // finally moving the actors
          for(int i = 0; i < C_GHOSTS; i++) {
            if(ghost[i].enabled) move_actor(&ghost[i], game, cman);
          }
          move_actor(&player, game, cman);
          c_mcounter++;
          c_score++;
        }
        // post game stuff
        cbreak(); // put back cbreak input mode
        switch(c_quitmsg) {
          case 1:
            attron(COLOR_PAIR(5));
            info_print(getmaxy(stdscr) - 5, "Game Over! You lost, good luck next time! Press any key to exit...");
            attroff(COLOR_PAIR(5));
            break;
          case 2:
            attron(COLOR_PAIR(6));
            info_print(getmaxy(stdscr) - 5, "Game Over! You won, congratulations! Press any key to exit...");
            attroff(COLOR_PAIR(6));
            break;
          default:
            attron(COLOR_PAIR(7));
            info_print(getmaxy(stdscr) - 5, "Press any key to exit to menu...");
            attroff(COLOR_PAIR(7));
        }
        refresh();
        getch();
        //reset game related variables
        ctrl_g = 0;
        ghost_count = 0;
        c_gmode = C_NORMAL;
        c_mcounter = 0;
        c_quitmsg = 0;
        map_loaded = 0;
        wclear(cman); wrefresh(cman);
        attron(COLOR_PAIR(1));
        info_print(getmaxy(stdscr) - 5, "Welcome to CURSEMAN");
        attroff(COLOR_PAIR(1));
        break;
      default:
        break;
    }
  }
  endwin();

  return 0;
}
// there was bullshit previously here
char process_ghost(ACTOR* ghost) { // there should be a switch for multiple conditions like switch(a && b && c && d) case a==1; b==0; c==1; d==1:
  if(ghost->hitflags[0] && !ghost->hitflags[1] && ghost->hitflags[2] && !ghost->hitflags[3]) {
    // we want clean corridor movement
    switch(ghost->nextmove) {
      case C_LEFT:
        return C_LEFT; // moving in a valid direction? good.
      case C_RIGHT:
        return C_RIGHT;
      default:
        switch(rand() % 2) {
          case 0:
            return C_RIGHT;
          case 1:
            return C_LEFT;
        }
        break;
    }
  } else if(!ghost->hitflags[0] && ghost->hitflags[1] && !ghost->hitflags[2] && ghost->hitflags[3]) {
    // need for clean again, if ghosts are placed properly no wall grinding on the sides should happen
    switch(ghost->nextmove) {
      case C_UP:
        return C_UP;
      case C_DOWN:
        return C_DOWN;
      default:
        switch(rand() % 2) {
          case 0:
            return C_DOWN;
          case 1:
            return C_UP;
        }
        break;
    }
  } else if(!ghost->hitflags[0] && ghost->hitflags[1] && ghost->hitflags[2] && ghost->hitflags[3]) {
    // U shaped spot (open only on top), dead end
    switch(rand() % 10) {
      case 0:
        return C_STATIONARY; // small chance to remain in there for a bit
      default:
        return C_UP;
    }
  } else if(ghost->hitflags[0] && !ghost->hitflags[1] && ghost->hitflags[2] && ghost->hitflags[3]) {
    // dead end open right only
    switch(rand() % 10) {
      case 3: // case is diffrent for more spicyness
        return C_STATIONARY;
      default:
        return C_RIGHT;
    }
  } else if(ghost->hitflags[0] && ghost->hitflags[1] && !ghost->hitflags[2] && ghost->hitflags[3]) {
    // dead end open bottom
    switch(rand() % 10) {
      case 7: 
        return C_STATIONARY;
      default:
        return C_DOWN;
    }
  } else if(ghost->hitflags[0] && ghost->hitflags[1] && ghost->hitflags[2] && !ghost->hitflags[3]) {
    // open left
    switch(rand() % 10) {
      case 1: 
        return C_STATIONARY;
      default:
        return C_LEFT;
    }
  } else if(!ghost->hitflags[0] && !ghost->hitflags[1] && ghost->hitflags[2] && ghost->hitflags[3]) {
    // corner (open top, open right)
    // also want clean movement
    if(ghost->nextmove == C_DOWN) return C_RIGHT;
    else if(ghost->nextmove == C_LEFT) return C_UP; // this might make sense with more hitflag hacks
                                                    // todo: make hitflags better, no they will stay as an array, no new struct, because too much changes would be needed
                                                    // done!
    else {
      switch(rand() % 2) {
        case 0:
          return C_RIGHT;
        case 1:
          return C_UP;
      }
    }
  } else if(ghost->hitflags[0] && !ghost->hitflags[1] && !ghost->hitflags[2] && ghost->hitflags[3]) {
    // open right, open bottom
    if(ghost->nextmove == C_LEFT) return C_DOWN;
    else if(ghost->nextmove == C_DOWN) return C_RIGHT;
    else {
      switch(rand() % 2) {
        case 0:
          return C_DOWN;
        case 1:
          return C_RIGHT;
      }
    }
  } else if(ghost->hitflags[0] && ghost->hitflags[1] && !ghost->hitflags[2] && !ghost->hitflags[3]) {
    // open bottom, open left
    if(ghost->nextmove == C_UP) return C_LEFT;
    else if(ghost->nextmove == C_RIGHT) return C_DOWN;
    else {
      switch(rand() % 2) {
        case 0:
          return C_LEFT;
        case 1:
          return C_DOWN;
      }
    }
  } else if(!ghost->hitflags[0] && ghost->hitflags[1] && ghost->hitflags[2] && !ghost->hitflags[3]) {
    // open left, open top
    if(ghost->nextmove == C_RIGHT) return C_UP;
    else if(ghost->nextmove == C_DOWN) return C_LEFT;
    else {
      switch(rand() % 2) {
        case 0:
          return C_UP;
        case 1:
          return C_LEFT;
      }
    }
  } else if(ghost->hitflags[0] && !ghost->hitflags[1] && !ghost->hitflags[2] && !ghost->hitflags[3]) {
    // T junction time, unfortunately becasue of some bad map design this code might run for some cross junctions, explained more in hitflag detection
    // fixed!
    // todo: make the ghost choose a direction diffrent from the one he's coming from
    // done!
    // this one is closed top
    if(ghost->nextmove == C_LEFT) {
      switch(rand() % 2) {
        case 0:
          return C_RIGHT;
        case 1:
          return C_DOWN;
      }
    } else if(ghost->nextmove == C_UP) {
      switch(rand() % 2) {
        case 0:
          return C_RIGHT;
        case 1:
          return C_LEFT;
      }
    } else if(ghost->nextmove == C_RIGHT) {
      switch(rand() % 2) {
        case 0:
          return C_LEFT;
        case 1:
          return C_DOWN;
      }
    } else { // if the ghost's hitflags get effed
      switch(rand() % 3) {
        case 0:
          return C_RIGHT;
        case 1:
          return C_DOWN;
        case 2:
          return C_LEFT;
      }
    }
  } else if(!ghost->hitflags[0] && ghost->hitflags[1] && !ghost->hitflags[2] && !ghost->hitflags[3]) {
    // closed right
    if(ghost->nextmove == C_RIGHT) {
      switch(rand() % 2) {
        case 0:
          return C_UP;
        case 1:
          return C_DOWN;
      }
    } else if(ghost->nextmove == C_UP) {
      switch(rand() % 2) {
        case 0:
          return C_UP;
        case 1:
          return C_LEFT;
      }
    } else if(ghost->nextmove == C_DOWN) {
      switch(rand() % 2) {
        case 0:
          return C_LEFT;
        case 1:
          return C_DOWN;
      }
    } else {
      switch(rand() % 3) {
        case 0:
          return C_UP;
        case 1:
          return C_DOWN;
        case 2:
          return C_LEFT;
      }
    }
  } else if(!ghost->hitflags[0] && !ghost->hitflags[1] && ghost->hitflags[2] && !ghost->hitflags[3]) {
    // closed bottom
    if(ghost->nextmove == C_LEFT) {
      switch(rand() % 2) {
        case 0:
          return C_UP;
        case 1:
          return C_LEFT;
      }
    } else if(ghost->nextmove == C_RIGHT) {
      switch(rand() % 2) {
        case 0:
          return C_UP;
        case 1:
          return C_RIGHT;
      }
    } else if(ghost->nextmove == C_DOWN) {
      switch(rand() % 2) {
        case 0:
          return C_LEFT;
        case 1:
          return C_RIGHT;
      }
    } else {
      switch(rand() % 3) {
        case 0:
          return C_UP;
        case 1:
          return C_RIGHT;
        case 2:
          return C_LEFT;
      }
    }
  } else if(!ghost->hitflags[0] && !ghost->hitflags[1] && !ghost->hitflags[2] && ghost->hitflags[3]) {
    // closed left
    if(ghost->nextmove == C_LEFT) {
      switch(rand() % 2) {
        case 0:
          return C_UP;
        case 1:
          return C_DOWN;
      }
    } else if(ghost->nextmove == C_UP) {
      switch(rand() % 2) {
        case 0:
          return C_UP;
        case 1:
          return C_RIGHT;
      }
    } else if(ghost->nextmove == C_DOWN) {
      switch(rand() % 2) {
        case 0:
          return C_DOWN;
        case 1:
          return C_RIGHT;
      }
    } else {
      switch(rand() % 3) {
        case 0:
          return C_UP;
        case 1:
          return C_RIGHT;
        case 2:
          return C_DOWN;
      }
    }

  } else if(!ghost->hitflags[0] && !ghost->hitflags[1] && !ghost->hitflags[2] && !ghost->hitflags[3]) {
    // actual cross junction
    if(ghost->nextmove == C_DOWN) {
      switch(rand() % 3) {
        case 0:
          return C_RIGHT;
        case 1:
          return C_DOWN;
        case 2:
          return C_LEFT;
      }
    } else if(ghost->nextmove == C_LEFT) {
      switch(rand() % 3) {
        case 0:
          return C_UP;
        case 1:
          return C_DOWN;
        case 2:
          return C_LEFT;
      }
    } else if(ghost->nextmove == C_UP) {
      switch(rand() % 3) {
        case 0:
          return C_UP;
        case 1:
          return C_RIGHT;
        case 2:
          return C_LEFT;
      }
    } else if(ghost->nextmove == C_RIGHT) {
      switch(rand() % 3) {
        case 0:
          return C_UP;
        case 1:
          return C_RIGHT;
        case 2:
          return C_DOWN;
      }
    } else {
      switch(rand() % 4) {
        case 0:
          return C_UP;
        case 1:
          return C_RIGHT;
        case 2:
          return C_DOWN;
        case 3:
          return C_LEFT;
      }
    }
  }
  return C_STATIONARY; // in case something happens we will see | it was actually useful when debugging ;)
}

void move_actor(ACTOR* actor, MAP_W game, WINDOW* win) {
  switch(actor->nextmove) {
    case C_UP:
      if(actor->hitflags[0]) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, 0, NULL);
      (actor->y)--;
      game[actor->y][actor->x] = actor->id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, actor->color, NULL);
      break;
    case C_RIGHT:
      if(actor->hitflags[1]) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, 0, NULL);
      (actor->x)++;
      game[actor->y][actor->x] = actor->id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, actor->color, NULL);
      break;
    case C_DOWN:
      if(actor->hitflags[2]) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, 0, NULL);
      (actor->y)++;
      game[actor->y][actor->x] = actor->id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, actor->color, NULL);
      break;
    case C_LEFT:
      if(actor->hitflags[3]) break;
      game[actor->y][actor->x] = 0;
      mvwaddch(win, actor->y, actor->x, ' ');
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, 0, NULL);
      (actor->x)--;
      game[actor->y][actor->x] = actor->id;
      mvwaddch(win, actor->y, actor->x, actor->display);
      mvwchgat(win, actor->y, actor->x, 1, A_BOLD, actor->color, NULL);
      break;
    default:
      break;
  }
}
// because the text mode isn't square and the map is 64x32 (x,y), making vertical paths wider makes them prettier
// placing the ghosts properly wouldn't require the check 1 position away, but allowing user created content is a pain
// there are also corner checks to avoid premature cross junction behaviour for ghosts
// todo: check if those hitflags could be used to constrain player movement aswell
// done!
// potential issue: some incorect player placements (for eg. in a custom map) may lead to a softlock | solution: cope with your skill issue or implement player spawnpoint checks in the editor
void update_hitflags(ACTOR* actor, MAP_W game) {
  actor->hitflags[0] = 0;
  if(game[actor->y - 1][actor->x] == 1 || game[actor->y - 1][actor->x + 1] == 1 || game[actor->y - 1][actor->x - 1] == 1) actor->hitflags[0] = 1;
  actor->hitflags[1] = 0;
  if(game[actor->y][actor->x + 1] == 1 || game[actor->y][actor->x + 2] == 1) actor->hitflags[1] = 1;
  actor->hitflags[2] = 0;
  if(game[actor->y + 1][actor->x] == 1 || game[actor->y + 1][actor->x + 1] == 1 || game[actor->y + 1][actor->x - 1] == 1) actor->hitflags[2] = 1;
  actor->hitflags[3] = 0;
  if(game[actor->y][actor->x - 1] == 1 || game[actor->y][actor->x - 2]) actor->hitflags[3] = 1;
}

void legend_print(int win_y, char mode) {
  //mode: 'm' - menu
  //      'g' - gameplay
  //      'i' - user input
  mvhline(win_y, 0, ' ', getmaxx(stdscr));
  mvhline(win_y + 1, 0, ' ', getmaxx(stdscr));
  mvchgat(win_y, 0, -1, A_NORMAL, 0, NULL);
  mvchgat(win_y + 1, 0, -1, A_NORMAL, 0, NULL);
  switch(mode) {
    case 'm':
      mvprintw(win_y, 2, "F1 Load map");
      mvprintw(win_y, 15, "F4 Start game");
      mvprintw(win_y, 30, "F5 Show map");
      mvprintw(win_y + 1, 2, "F2 Display instructions");
      mvprintw(win_y + 1, 27, "F3 Display credits");
      mvprintw(win_y + 1, 47, "F10 Exit");

      mvchgat(win_y, 2, 2, A_NORMAL, 1, NULL);
      mvchgat(win_y, 15, 2, A_NORMAL, 1, NULL);
      mvchgat(win_y, 30, 2, A_NORMAL, 1, NULL);
      mvchgat(win_y + 1, 2, 2, A_NORMAL, 1, NULL);
      mvchgat(win_y + 1, 27, 2, A_NORMAL, 1, NULL);
      mvchgat(win_y + 1, 47, 3, A_NORMAL, 1, NULL);
      break;
    case 'g':
      mvaddch(win_y, 2, ACS_LARROW); mvaddch(win_y, 3, ACS_RARROW); mvaddch(win_y, 4, ACS_UARROW); mvaddch(win_y, 5, ACS_DARROW);
      mvprintw(win_y, 7, "Move");
      mvprintw(win_y, 13, "C Curseman (normal)");
      mvprintw(win_y, 34, "@ Ghost"); //44
      mvprintw(win_y + 1, 2, "F10 Exit game");
      mvprintw(win_y + 1, 17, "C Curseman (dreamer)");

      mvchgat(win_y, 2, 4, A_ALTCHARSET, 1, NULL);
      mvchgat(win_y, 13, 1, A_BOLD, C_NORMAL, NULL);
      mvchgat(win_y, 34, 1, A_BOLD, 3, NULL);
      mvchgat(win_y + 1, 2, 3, A_NORMAL, 1, NULL);
      mvchgat(win_y + 1, 17, 1, A_BOLD, C_DREAMER, NULL);
      break;
    case 'i':
      mvprintw(win_y, 2, "^H Remove 1 character");
      mvprintw(win_y, 25, "Enter Confirm");
      mvprintw(win_y + 1, 2, "^U Clear all characters");

      mvchgat(win_y, 2, 2, A_NORMAL, 1, NULL);
      mvchgat(win_y, 25, 5, A_NORMAL, 1, NULL);
      mvchgat(win_y + 1, 2, 2, A_NORMAL, 1, NULL);
      break;
  }
}

// variable names are like this, because of history (this code was previously part of the main function)
void print_map(WINDOW* cman, ACTOR* player, ACTOR* ghost, MAP_W game, int* ghost_count) {
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
        player->y = y; player->x = x;
      }
      if(game[y][x] == 2 && *ghost_count < C_GHOSTS) { //find ghosts spawn
        ghost[*ghost_count].y = y;
        ghost[*ghost_count].x = x;
        (*ghost_count)++;
      }
    }
  }
}
