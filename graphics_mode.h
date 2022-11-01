#ifndef GRAPHICS_MODE_H
#define GRAPHICS_MODE_H 1
#define HRES 320
#define VRES 240
//framebuffer
#include "main.h"
extern uint8_t g_fbuffer[HRES*VRES];
extern uint32_t g_position;
extern uint8_t g_bytes_processed;
extern uint8_t g_active_register;
extern uint8_t g_pixel_size;
#define G_SET_X  0x01   //two bytes IO op
#define G_SET_Y  0x02      
#define G_SET_PIXEL_SIZE  0x03 //sets pixel chunk for write. Convenience for snake
#define G_CLEAR_FB  0x04
#define G_SOLID_FILL  0x05
#define G_RESET_REGS  0xFF
#define G_BUFFER  0x00  //reads from framebuffer


//basic operation. SET_X, or SET Y changes the current read address.
//A read or a write advances the cursor

void g_mode_write(uint8_t ch);
char g_mode_read();
void set_pixel(uint16_t x, uint16_t y, uint8_t color);
void solid_fill(uint8_t);
void graphics_mode();
#endif
