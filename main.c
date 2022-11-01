
//#define Z80_BUS 1
extern void usb_init();
#include  "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"

#include "pico/multicore.h"
#include <stdio.h>
#include <string.h>
#include "z80io.pio.h"
#include "hardware/irq.h"

#include "main.h"
#include "ansi_terminal.h"
#include "text_mode.h"
#include "basic_progs.h"

#define BELL_ENABLED 1
#ifdef BELL_ENABLED
#include "bell.h"
uint32_t bell_start_tick;
#endif

uint16_t scanline;


//z80 bus protocol uses pio1
PIO p1;
uint offset_z80io;
uint sm_z80io;	  
bool mode_change;
video_mode current_mode;



void io_main() {
  //unsure if still necessary
  //  irq_set_priority(7, 0x40);
  //irq_set_priority(8, 0x40);
  //irq_set_priority(11, 0x40);
  //irq_set_priority(12, 0x40);
#ifdef Z80_IO
  z80io_setup();
#endif
  unsigned int count = 0;
  char ch = 0;

  in_escape = 0;
  while(1) {
        
    tuh_task();
    hid_app_task();

#ifdef BELL_ENABLED
    if (bell_is_on) {
      if (time_us_32()-bell_start_tick > BELL_DURATION_US || bell_start_tick > time_us_32())
	stop_bell();
    }
#endif	  

#ifdef UART_TERMINAL
    if (uart_is_readable(UART_ID)){
      ch = uart_getc(UART_ID);
  #ifdef BELL_ENABLED
      if (ch==BELL_CHAR){
	start_bell(BELL_HZ);
	bell_start_tick=time_us_32();
      }
   #endif

      process_recieve(ch);
    }
    if (uart_is_writable(UART_ID) && key_ready()){
      ch = get_keypress();
      uart_putc(UART_ID, (char)ch);
    }   
#endif

    if (key_ready()&&kb_buffer[0]==249)
      send_program(mandel);
    if (key_ready()&&kb_buffer[0]==250)
      send_program(sst);


#ifdef Z80_IO
    bus_read();
#endif
  }

}
 
#ifdef UART_TERMINAL
void serial_setup() {
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);  
  uart_init(UART_ID, BAUD_RATE);
  uart_set_hw_flow(UART_ID, false, false);  
  uart_set_fifo_enabled(UART_ID, false);
}
#endif
 
 

 
void z80io_setup() {
  //  stdio_init_all();
  float freq = 60000000.0;
  float div = (float)clock_get_hz(clk_sys) / freq;
  gpio_set_dir(13,0);
  p1 = pio1;
  offset_z80io = pio_add_program(p1, &z80io_program);
  sm_z80io = pio_claim_unused_sm(p1, true);	  
  gpio_init(12);
  gpio_init(11);
  gpio_init(10);
  gpio_set_dir(11,GPIO_OUT);	
  gpio_set_dir(10,GPIO_IN);
  gpio_put(11,1);
  gpio_set_dir(12,GPIO_IN);
  gpio_pull_up(10);
  gpio_pull_up(12);
  for (int i = 0; i < 10; i++) gpio_set_dir(13+i,0);
  z80io_init(p1, sm_z80io, offset_z80io, div);
  pio_sm_set_enabled(p1, sm_z80io, true);    
}


void bus_read() {
  uint32_t r1,r2,r3,r4;
  uint8_t base;
  uint8_t regs[4];
  if(pio_interrupt_get(p1, 5)){      
    r1 = pio_sm_get(p1,sm_z80io);
    //base is our register. 
    base = (uint8_t)((r1 & 0x0000FF00) >> 8);
    char ch = (uint8_t) r1 & 0x000000FF;     

    if (base==0) {
      
	process_recieve(ch);
	
#ifdef BELL_ENABLED
	if (ch==BELL_CHAR){
	  start_bell(BELL_HZ);
	  bell_start_tick=time_us_32();
	}
#endif
      }
    
    else if (base==1){
      g_active_register = ch;
      g_bytes_processed=0;
    }
    else if (base==2){
      g_mode_write(ch);
    }      
    else if (base==3){
      if (ch & 0x80){
	if (current_mode !=graphics) {
	  current_mode=graphics;
	  mode_change=true;
	}
      }
      else {
	if (current_mode != text) {
	  current_mode=text;
	  mode_change=true;
	}
      }
      if (ch & 0x10)
	read_ahead_enabled=true;
    }
    pio_interrupt_clear(p1, 5);          
  }
  if(pio_interrupt_get(p1, 6)) {	
    r1 = pio_sm_get(p1, sm_z80io);
    base = (uint8_t)((r1 & 0x0000FF00) >> 8);		  
    //printf("(in) %d, base - %d, val - %d, count - %d\r\n", r1, base, regs[base],r4++ );
    //keyboard status
    if (base==3){
      if (key_ready())
	r1=0x01;
      else
	r1=0x00;
      if (current_mode == graphics)
	r1=r1|0x80;
      if (read_ahead_enabled)
	r1=r1|0x10;
    }
    //+ 2
    else if (base==1) {
      r1=g_active_register;
    }
    //+1
    else if (base==2) {
      r1=g_mode_read();
    }
    else if (base==0) {
      r1=0;
      if(key_ready()){
	r1=get_keypress();
      }
    }
    
    r2 = r1 << 24 | r1 << 16 | r1 <<8 | r1;		  
    pio_sm_put(p1, sm_z80io, r2);
    pio_interrupt_clear(p1,6);				
  }
}



extern void pattern();

int main(){
  current_mode=text;
  set_sys_clock_khz(CPU_FREQ, true);
#ifdef UART_TERMINAL
  serial_setup();
#endif
   irq_set_priority(7, 0x40);
   irq_set_priority(8, 0x40);
   irq_set_priority(11, 0x40);
   irq_set_priority(12, 0x40);
   build_f_table();

   usb_init();   
   multicore_launch_core1(io_main);    
   fill_background();
   mode_change=false;
   current_mode=text;
   //do_text_mode();
   pattern();
   while (1) {
     if (current_mode==graphics)
       graphics_mode();
     else
       do_text_mode();
     mode_change=false;
   }
}
