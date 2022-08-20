#ifndef TEXT_MODE_H
#define TEXT_MODE_H 0

#include "string.h"
#include  "pico/stdlib.h"
#include "text_mode.h"
#include "ansi_terminal.h"

extern uint32_t t_color[];
extern uint32_t bt_color[];

void fill_background();
void unpack_font();
void fill_scan_m(uint8_t *, char *, int);
void fill_scan(uint8_t *, char *, char *, int, int);

uint32_t unpacked_font[8400];
uint32_t unpacked_mask[8400];

char sbuffer[2401];
char abuffer[2401];

extern uint16_t scanline;
#endif
