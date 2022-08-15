#ifndef ANSI_TERMINAL_H
#define ANSI_TERMINAL_H 0
#include "main.h"
#include "text_mode.h"
#include "pico/stdlib.h"

bool start_escape;
bool in_escape;
char escape_buffer[30];
uint8_t escape_buffer_position;

extern void scroll_screen();
extern uint cursor;
extern char abuffer[2401];
extern char sbuffer[2401];

void process_recieve(char);
// #define RGB_ACTIVE 639 // change to this if 1 pixel/byte
//Terminal color codes


#endif
