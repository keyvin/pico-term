#ifndef ANSI_TERMINAL_H
#define ANSI_TERMINAL_H 0
#include "main.h"
#include "text_mode.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"

#define MAX_ESCAPE_ARGUMENTS 20
#define MAX_ARG_LENGTH 10
bool start_escape;
bool in_escape;
char escape_buffer[MAX_ARG_LENGTH+1];
uint8_t escape_buffer_position;
unsigned int escape_arguments[MAX_ESCAPE_ARGUMENTS];
extern uint8_t num_arguments;
extern uint8_t cursor_attributes;

#define BOLD 0x01
#define DIM 0x01
#define UNDERSCORE 0x02
#define BLINK 0x04
#define REVERSE 0x08
//bits bright, underscore, blink, reverse
// bit 1 bright, bit 2 underscore, bit 3 blink

extern void scroll_screen();
extern uint cursor;
extern char abuffer[2401];
extern char sbuffer[2401];
extern uint32_t t_buffer[ROW*COL+1];
void process_recieve(char);
// #define RGB_ACTIVE 639 // change to this if 1 pixel/byte
//Terminal color codes


#endif
