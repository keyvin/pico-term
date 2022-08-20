#ifndef MAIN_H
#define MAIN_H 1
#define UART_TERMINAL 1
#define CPU_FREQ 180000
#define PIXEL_CLOCK 25175000
#include  "pico/stdlib.h"
//Static values for screen layout
#define ROW 30
#define COL 80
#define LAST_CHAR 2400

#ifdef UART_TERMINAL
#include "hardware/uart.h"
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 16
#define UART_RX_PIN 17
#endif

#define VSYNC_PIN 8
#define HSYNC_PIN 9
#define RGB_PIN 0
#define H_ACTIVE   655    // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE   480    // (active - 1)
#define RGB_ACTIVE 639    
#define TXCOUNT 640

extern uint32_t unpacked_font[8400];
extern uint32_t unpacked_mask[8400];

//character buffer - 80x30
extern char sbuffer[LAST_CHAR+1];
//attribute buffer - 80x30
extern char abuffer[LAST_CHAR+1];
extern void tuh_task();
extern void hid_app_task();

uint cursor;
uint mode;


#define T_MONOCHROME 0;
#define T_64C_80x30 1;  //2grb, 2status
#endif
