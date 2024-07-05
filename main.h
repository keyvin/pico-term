#ifndef MAIN_H
#define MAIN_H 1
//#define UART_TERMINAL 1
#include "nrgb.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"
#include  "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/irq.h"
#include "graphics_mode.h"
#include "keyboard.h"

#include "bsp/board.h"
#include "tusb.h"

#define Z80_IO 1
#define CPU_FREQ 218000
#define PIXEL_CLOCK 25172000
#include  "pico/stdlib.h"
//Static values for screen layout
#define ROW 30
#define COL 80
#define LAST_CHAR 2400
//#define UART_TERMINAL 1
#ifdef UART_TERMINAL
#include "hardware/uart.h"
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 16
#define UART_RX_PIN 17
#endif


#ifdef Z80_IO
#include "z80io.pio.h"
void z80io_setup();
void bus_read();
#endif




#define BUZZER_PIN 27

#define VSYNC_PIN 8
#define HSYNC_PIN 9
#define RGB_PIN 0
#define H_ACTIVE   655    // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE   479    // (active - 1)
#define RGB_ACTIVE 639    
#define TXCOUNT 640*16   //1bpp

//character buffer - 80x3
extern uint32_t t_buffer[ROW*COL+1];
extern void tuh_task();
extern void hid_app_task();
extern void build_f_table();

extern bool mode_change;
enum VIDEO_MODE {text, graphics};
typedef enum VIDEO_MODE video_mode;
extern video_mode current_mode;
extern bool vblank_interrupt;
#define T_MONOCHROME 0;
#define T_64C_80x30 1;  //2grb, 2status
#endif
