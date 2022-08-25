#ifndef TEXT_MODE_H
#define TEXT_MODE_H 0

#include "string.h"
#include  "pico/stdlib.h"
#include "text_mode.h"
#include "ansi_terminal.h"

#define C_START_OF_ROW(C) ((C/COL)*COL)
#define C_END_OF_ROW(C) ((C/COL)*COL+(COL-1))
#define C_START_OF_NEXT_ROW(C) ((C/COL)*COL+COL)
#define LAST_ROW_START (LAST_CHAR-COL)
#define C_GET_ROW(C) (C/COL)
#define C_GET_COL(C) (C%COL)


#define T_ATTRIBUTE  24
#define T_FOREGROUND 16
#define T_BACKGROUND  8
#define T_CHARACTER   0

#define T_GET_ATTRIBUTE(C) ( (uint8_t) ((C & 0xFF000000) >> T_ATTRIBUTE))
#define T_GET_FOREGROUND(C) ( (uint8_t) ((C & 0x00FF0000) >> T_FOREGROUND))
#define T_GET_BACKGROUND(C) ( (uint8_t) ((C & 0x0000FF00) >> T_BACKGROUND))
#define T_GET_CHARACTER(C) ((uint8_t) (C & 0x000000FF))  


extern uint32_t t_color[];
extern uint32_t bt_color[];

void fill_background();
void unpack_font();
void unpack_cell(uint32_t, uint8_t *, uint8_t *, uint8_t *, uint8_t*);
uint32_t pack_cell( uint8_t , uint8_t , uint8_t , uint8_t);
void fill_scan_m(uint8_t *, char *, int);
void fill_scan(uint8_t *, uint32_t *, int, int);

//buffer for text, attributes, background, foreground.
//packed for DMA operations. 
extern uint32_t t_buffer[ROW*COL+1];

char sbuffer[2401];
char abuffer[2401];
char fbuffer[2401];
char bbuffer[2401];
extern uint16_t scanline;
#endif
