#ifndef GRAPHICS_MODE_H
#define GRAPHICS_MODE_H 1
#define HRES 320
#define VRES 240
//framebuffer
#include "main.h"
extern uint8_t g_fbuffer[HRES*VRES];
extern uint32_t g_position;
void set_pixel(uint16_t x, uint16_t y, uint8_t color);
void solid_fill(uint8_t);
void graphics_mode();
#endif
