#ifndef KEYBOARD_H
#define KEYBOARD_H 1

#include "main.h"
void keypress(char );
void init_keyboard();
uint8_t get_keypress();
bool key_ready();
void toggle_read_ahead();

//Keyboard buffer
#define KB_BUFFER_SIZE 20
extern char kb_buffer[KB_BUFFER_SIZE];
extern uint8_t kb_count;
extern bool read_ahead_enabled;


#endif
