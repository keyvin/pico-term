#ifndef KEYCODE_STRINGS
#define KEYCODE_STRINGS 1

#define UP_S 0
#define DOWN_S 1
#define RIGHT_S 2
#define LEFT_S 3
#define DEL_S 4
#define PGUP_S 5
#define PGDOWN_S 6
#define HOME_S 7
#define END_S 8
#define FUNCTION_S 9


char **scan_to_escape = {
  "\x1b[A",
  "\x1b[B",
  "\x1b[C",
  "\x1b[D",
  "\x1b[[3~"
};



#endif
