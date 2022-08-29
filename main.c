
//#define Z80_BUS 1
extern void usb_init();
#include  "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "pico/multicore.h"
#include "nrgb.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"
#include <stdio.h>
#include <string.h>
#include "z80io.pio.h"
#include "hardware/irq.h"
#include "main.h"
#include "ansi_terminal.h"
#include "text_mode.h"


#define BELL_ENABLED 1
#ifdef BELL_ENABLED
#include "bell.h"
uint32_t bell_start_tick;
#endif

uint16_t scanline;

//DMA BUFFERS
uint32_t RGB_buffer1[16*180]; //8bpp
uint32_t RGB_buffer2[16*180];
//need 1/4th pixes (32 bit)

//Keyboard buffer
#define KB_BUFFER_SIZE 20
char kb_buffer[KB_BUFFER_SIZE];
uint8_t kb_count;



//z80 bus protocol uses pio1
PIO p1;
uint offset_z80io;
uint sm_z80io;	  


//Callback from hid_task.c. 
void keypress(char p) {
  if (kb_count < KB_BUFFER_SIZE) {
    kb_buffer[kb_count]=p;
    kb_count++;
  }
}

void init_keyboard(){
  kb_count=0;
  for (int a=0; a<KB_BUFFER_SIZE;a++){
    kb_buffer[a]=0;
  }
  return;
}

char get_keypress() {
  char ch=0;
  if (kb_count >0){
    ch = kb_buffer[0];
    for (int a = 1; a < kb_count ; a++){
	kb_buffer[a-1] = kb_buffer[a];
    }
    kb_count--;
  }
  return ch;
}

bool key_ready(){
  if (kb_count > 0)
    return true;
  return false;
}


void io_main() {
  //unsure if still necessary
  irq_set_priority(7, 0x40);
  irq_set_priority(8,0x40);
  irq_set_priority(11, 0x40);
  irq_set_priority(12, 0x40);

  unsigned int count = 0;
  char ch = 0;

  in_escape = 0;
  while(1) {
#ifdef BELL_ENABLED
    if (bell_is_on) {
      if (time_us_32()-bell_start_tick > BELL_DURATION_US || bell_start_tick > time_us_32())
	stop_bell();
    }
#endif	  
        
    tuh_task();
    hid_app_task();

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
    /*    if (pio_interrupt_get(pio0,5)) {
      for (uint i=0; (i < 500) && recieve_buffer_ready()==true; i++){
	process_recieve(recieve_buffer_get());
	if (!pio_interrupt_get(pio0,5))break; 
	
	if (uart_is_readable(UART_ID)){
	  ch = uart_getc(UART_ID);
	  recieve_buffer_put(ch);
	}

	
      }
      
      }*/

    
  }

}
 

void serial_setup() {
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);  
  uart_init(UART_ID, BAUD_RATE);
  uart_set_hw_flow(UART_ID, false, false);  
  uart_set_fifo_enabled(UART_ID, false);
}

 
 

 
void z80io_setup() {
  //  stdio_init_all();
  float freq = 40000000.0;
  float div = (float)clock_get_hz(clk_sys) / freq;
  //gpio_set_dir(13,0);
  p1 = pio1;
  offset_z80io = pio_add_program(p1, &z80io_program);
  sm_z80io = pio_claim_unused_sm(p1, true);	  

  gpio_init(12);
  gpio_init(11);
  gpio_init(10);
  gpio_set_dir(10,GPIO_OUT);	
  gpio_set_dir(11,GPIO_IN);
  gpio_put(10,1);
  gpio_set_dir(12,GPIO_IN);
  gpio_pull_up(11);
  gpio_pull_down(12);
  //for (int i = 0; i < 8; i++) gpio_set_dir(14+i,0);
  z80io_init(p1, sm_z80io, offset_z80io, div);
  //  pio_sm_set_enabled(p1, sm_z80io, true);    
}

//needs to be rewritten to use packed terminal cell format
/*
void bus_read() {
  uint32_t r1,r2,r3,r4;
  uint8_t base;
  uint8_t regs[4];
  if(pio_interrupt_get(p1, 5)){      
    r1 = pio_sm_get(p1,sm_z80io);
    base = (uint8_t)((r1 & 0x0000FF00) >> 8);
    regs[base] = (uint8_t) r1 & 0x000000FF;     
    if (base==0){
      sbuffer[cursor++]=regs[base];
      if (cursor >= 2400) cursor = 0;
    }
    if (base==1){
      if (cursor!=0)
	abuffer[cursor-1]=regs[base];
      else
	abuffer[2400]=regs[base];
    }
    if (base==3){
      if (regs[base]==10)
	cursor=0;
    }
    pio_interrupt_clear(p1, 5);      
  }
  if(pio_interrupt_get(p1, 6)) {	
    r1 = pio_sm_get(p1,sm_z80io);
    base = (uint8_t)((r1 & 0x0000FF00) >> 8);		  
    //	printf("(in) %d, base - %d, val - %d, count - %d\r\n", r1, base, regs[base],r4++ );
    r1 = regs[base];
    r2 = r1 << 24 | r1 << 16 | r1 <<8 | r1;		  
    pio_sm_put(p1, sm_z80io, r2);
    pio_interrupt_clear(p1,6);				
  }
}
*/




int main(){
  set_sys_clock_khz(CPU_FREQ, true);
#ifdef UART_TERMINAL
   serial_setup();
#endif
   irq_set_priority(7, 0x40);
   irq_set_priority(8,0x40);
   irq_set_priority(11, 0x40);
   irq_set_priority(12, 0x40);
   build_f_table();
   usb_init();
   
   multicore_launch_core1(io_main);    
   fill_background();
  PIO pio = pio0;
  uint hsync_offset = pio_add_program(pio, &hsync_program);
  uint vsync_offset = pio_add_program(pio, &vsync_program);
  uint rgb_offset = pio_add_program(pio, &nrgb_program);
  
  // Manually select a few state machines from pio instance pio0.
  uint hsync_sm = 0;
  uint vsync_sm = 1;
  uint rgb_sm = 2;
  float freq = PIXEL_CLOCK;
  float div1 = ((float)clock_get_hz(clk_sys)) / freq;
  float div2 = ((float)clock_get_hz(clk_sys)) / (freq*5); //run it 3 times faster?
  //float div3 = ((float)clock_get_hz(clk_sys)) / (freq*2);

  // DMA channels - 0 sends color data, 1 reconfigures and restarts 0
  int rgb_chan_0 = 0;
    

  // Channel Zero (sends color data to PIO VGA machine)
  dma_channel_config c0 = dma_channel_get_default_config(rgb_chan_0);  // default configs
  channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);              // 32-bit txfers
  channel_config_set_read_increment(&c0, true);                        // yes read incrementing
  channel_config_set_write_increment(&c0, false);                      // no write incrementing
  channel_config_set_dreq(&c0, DREQ_PIO0_TX2) ;                        // DREQ_PIO0_TX2 pacing (FIFO)
  dma_channel_configure(
			rgb_chan_0,                 // Channel to be configured
			&c0,                        // The configuration we just created
			&pio->txf[rgb_sm],          // write address (RGB PIO TX FIFO)
			RGB_buffer1,            // The initial read address (pixel color array)
			TXCOUNT,                    // Number of transfers; in this case each is 1 byte.
			false                       // start immediately.
    );
  
  hsync_program_init(pio, hsync_sm, hsync_offset, HSYNC_PIN, div1);
  vsync_program_init(pio, vsync_sm, vsync_offset, VSYNC_PIN, div1);
  nrgb_program_init(pio, rgb_sm, rgb_offset, RGB_PIN, div2);
  pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
  pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
  pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);
  // Start the two pio machine IN SYNC
  // Note that the RGB state machine is running at full speed,
  // so synchronization doesn't matter for that one. But, we'll
  // start them all simultaneously anyway.
  
  scanline = 0;
  uint16_t buffer_line =0;
  uint16_t pixel = 0;
  uint32_t *rgb;
  uint32_t *rgb_n;
  uint8_t *sync;	
  uint32_t flip = 0;

  // z80io_setup();
  for (int i=0;i < 16; i++){
    fill_scan(RGB_buffer1+(i*162), t_buffer, i, 0);
    fill_scan(RGB_buffer2+(i*162), t_buffer, i, 0);
  }
  uint32_t bstart = 0;
  uint32_t vb;
  pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));
  unsigned int frame = 0;
  uint32_t *ptr;
  uint32_t *tmp_p;
  rgb = (uint32_t *) RGB_buffer1;
  rgb_n = (uint32_t *) RGB_buffer2;
  bstart = 0;
  //  gpio_set_dir(0,true);
  //gpio_set_function(0, GPIO_FUNC_PIO0);
  while (1) {   
    tmp_p = rgb;
    rgb = rgb_n;	    
    rgb_n = tmp_p;
    dma_channel_set_read_addr(rgb_chan_0, rgb, true);
    //fill the buffer for the flip    
    //    scanline+=16;
    bstart = bstart+1;
    if (pio_interrupt_get(pio,5)) {
       pio_interrupt_clear(pio,5); //irq 5 shows we are in vblank. Do not clear
	                          //vblank before last scanline
       if (bstart < 30){
	   bstart=0;
	   dma_channel_abort(rgb_chan_0);
	   pio_sm_clear_fifos(pio, rgb_sm);
	   pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);
	   pio_sm_restart(pio,rgb_sm);
	   pio_sm_clear_fifos(pio, rgb_sm);
       }

    }
    if (bstart==30){	
      bstart =0;
      scanline=0;
      frame++;
    }
    for (int i=0; i <16; i++) {
      fill_scan(rgb_n +(i*162), (uint32_t *)(t_buffer+(bstart*COL)),i,frame);
    }
    if ((frame%60)<30 &&  (cursor/COL)==(bstart/COL)) {
      for (int i=0; i <16; i++) {
	//TODO - should be cursor color/mode
	rgb_n[C_GET_COL(cursor)*2+(i*162)]=0xFFFFFFFF;
	rgb_n[C_GET_COL(cursor)*2+(i*162)+1]=0xFFFFFFFF;
      }
      
    }   
        
    dma_channel_wait_for_finish_blocking(rgb_chan_0);
  }
      
}
