#include  "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "pico/multicore.h"
//#include "rgb.pio.h"
//#include "sync.pio.h"
#include "nrgb.pio.h"
#include "vsync.pio.h"
#include "hsync.pio.h"
#include "font.h"
#include <stdio.h>
#include <string.h>
#include "z80io.pio.h"

#define VSYNC_PIN 8
#define HSYNC_PIN 9
#define RGB_PIN 0
#define H_ACTIVE   655    // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE   479    // (active - 1)
#define RGB_ACTIVE 639    // (horizontal active)/2 - 1
#define TXCOUNT 640
// #define RGB_ACTIVE 639 // change to this if 1 pixel/byte

//Goal is to create functions that 
//create buffers with the timing pixels
//embedded. 

//Ex. a 640x480@60hz has 800 pixels per line
//640 visible (RGB values, but no H&V sync)
//16 Front porch, 96 sync pulse, 48 back porch
//after 480 of these lines, we have a few vblank lines
//10 Front porch (hsync normal), 2 sync pulse (vsync and hysnc), 33 back porch

//The PIO must change pin values every
// (1/25.175mhz)s

//the pico defaults to 125mhz.
//code given in datasheet...
//float freq = 25175000
//float div = clock_get_hz(clk_sys) / ();
//     sm_config_set_clkdiv(&c, div)


//we pregenerate several lines, give them to the
//dma engine, then prepare several more
//until we've done enough lines it's time
//for a vsync

//My first mode will use 160x120 upscaled to 
//640x480 as my monitors do not aspect correct
//lower. Each pixel from the z80 will be x4
//and every scanline will repeat 4x. The pico
//only has 240k RAM. 

//Our Dma buffers Will take ~1kb per line. 

//Hsync pin is first, vsync pin is second
//480 lines of hsync buffer
//10 lines of blank, with hsync lines
//and 0s on RGB pins
//2 lines of vsync buffer
//33 back porch  - normal hsync lines

//need to be divisible by 4 for simplest possible pio. 
//dma buffers
uint32_t t_color[] = { 0x00000000,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0xFFFFFFFF
};

uint32_t bt_color[] = { 0x00000000,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0x02020202,
			 0xC0C0C0C0,
			 0x38383838,
			 0xFFFFFFFF
};
			

uint8_t RGB_buffer[2][800]; //8bpp


uint32_t unpacked_font[8400];
uint32_t unpacked_mask[8400];
//character buffer
char sbuffer[2401];
//attribute buffer
char abuffer[2401];
//cursor
uint cursor;
uint mode;

#define T_MONOCHROME 0;
#define T_64C_80x30 1;  //2grb, 2status

void generate_rgb_scan(uint8_t *);
void generate_vblank_rgb(uint8_t *);
void generate_hsync_scan(uint8_t *);
void generate_vsync_scan(uint8_t *);


void fill_background() {
  for (int i =0; i < 25; i++)
    for (int j = 0;  j < 80; j++) {
      sbuffer[(i*j)+i] =' ' ;
    }
  //static string.
  strcpy(sbuffer, "Initial buffer - IO 39-10 to reset");
}

void unpack_font() {
  uint8_t buffer[8];
  uint8_t mask[8];
  uint8_t offs;
  for (int i = 0; i < 256; i++) {
    for (int l = 0;l< 16;l++) {      
      offs = VGA8_F16[(i*16)+l];
      buffer[7] = offs & 0x01 ? 255: 0;
      mask[7] = offs & 0x01 ? 0: 255;
      buffer[6] = offs & 0x02 ? 255: 0;
      mask[6] = offs & 0x02 ? 0:255;
      buffer[5] = offs & 0x04 ? 255: 0;
      mask[5] = offs & 0x02 ? 0:255;
      buffer[4] = offs & 0x08 ? 255: 0;
      mask[4] = offs & 0x02 ? 0:255;
      buffer[3] = offs & 0x10 ? 255: 0;
      mask[3] = offs & 0x02 ? 0:255;
      buffer[2] = offs & 0x20 ? 255: 0;
      mask[2] = offs & 0x02 ? 0:255;
      buffer[1] = offs & 0x40 ? 255: 0;
      mask[1] = offs & 0x02 ? 0:255;
      buffer[0] = offs & 0x80 ? 255: 0;
      mask[0] = offs & 0x02 ? 0:255;
      unpacked_font[((i*2)*16)+(l*2)] = *((uint32_t *) buffer);
      unpacked_font[((i*2)*16)+(l*2)+1] = *((uint32_t *) (buffer+4));
      unpacked_mask[((i*2)*16)+(l*2)] = *((uint32_t *) mask);
      unpacked_mask[((i*2)*16)+(l*2)+1] = *((uint32_t *) (mask+4));
    }
  }
}

//fill DMA buffer at current scanline
void fill_scan_m(uint8_t *buffer, char *string, int line) {
  unsigned int p;
  uint32_t *b= (uint32_t *) buffer;
  uint32_t offs;
  for (int i =0; i < 80; i++) {
    p = 2*i;
    offs = ((string[i]*2)*16)+(2*line);
    b[p] = unpacked_font[offs];
    b[p+1] = unpacked_font[offs+1];    
  }
}


/*void fill_scan(uint8_t *buffer, char *string, char*attr, int line) {

  if (line%4==0 || line%4==1){
    for (int a = 0; a < 320; a++)buffer[a]=0xff;
    for (int a = 320; a<640; a++)buffer[a]=0x00;
  }
  else{
    for (int a = 0; a < 320; a++)buffer[a]=0x00;
    for (int a = 320; a<640; a++)buffer[a]=0xff;
  }
  buffer[639] = 0;
  
}
*/


void fill_scan(uint8_t *buffer, char *string, char*attr, int line) {
  unsigned int p;

  uint32_t *b= (uint32_t *) buffer;
  uint8_t stats;
  uint32_t offs;
  uint32_t foreground =  0xC0C0C0C0;
  uint32_t background =  0x02020202;
  for (int i =0; i < 80; i++) {
    p = 2*i;    
    offs = ((string[i]*2)*16)+(2*line);
    stats = attr[i];
    if (stats){
      foreground = t_color[stats & 0x0F];
      background = bt_color[(stats & 0xF0) >> 4];
    }
    else {
      foreground = 0xFFFFFFFF;
      background = 0x00000000;
	
    }
      
    b[p] = unpacked_font[offs] & foreground | (unpacked_mask[offs] & background);  
    b[p+1] = unpacked_font[offs+1] & foreground | (unpacked_mask[offs+1] & background);         
  }
  b[160]=0;
}


//bus protocol uses pio1
PIO p1;
uint offset_z80io;
uint sm_z80io;	  

extern void tuh_task();
extern void hid_app_task();

void usb_main() {
  unsigned int count = 0;
 while(1) {
   
	tuh_task();
	hid_app_task();
   
}

}

void keypress(char p) {
 sbuffer[cursor++] = p;
 if (cursor >=2000) cursor = 0;
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


//uses pio1

extern void usb_init();
int main(){
  //our output is 480 lines of rgb and hsync.
  //10 lines of vblank and hsync
  //2 lines of vblank and vsync
  //33 lines of vblank and hsync
  //  char th[] = "This is the message I'd like to repeat. 1234567890\xb0\xb1\xBB";
  usb_init();
  set_sys_clock_khz(180000, true);
  multicore_launch_core1(usb_main);
   
  unpack_font();
  // generate_rgb_scan(RGB_buffer[0]);
  //	generate_rgb_scan(RGB_buffer[1]);
  //	generate_vblank_rgb(Vblank);
  //	generate_hsync_scan(Hsync_buffer);
  //	generate_vsync_scan(Vsync_buffer);	
  fill_background();
  fill_scan(RGB_buffer[0],sbuffer,abuffer,0);
  fill_scan(RGB_buffer[1],sbuffer,abuffer,0);
  PIO pio = pio0;
  uint hsync_offset = pio_add_program(pio, &hsync_program);
  uint vsync_offset = pio_add_program(pio, &vsync_program);
  uint rgb_offset = pio_add_program(pio, &nrgb_program);
  
  // Manually select a few state machines from pio instance pio0.
  uint hsync_sm = 0;
  uint vsync_sm = 1;
  uint rgb_sm = 2;
  float freq = 25175000.0;
  float div1 = ((float)clock_get_hz(clk_sys)) / freq;
  float div2 = ((float)clock_get_hz(clk_sys)) / (freq*5); //run it 3 times faster?


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
        &RGB_buffer[0],            // The initial read address (pixel color array)
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
  

  
  
  uint16_t scanline = 0;
  uint16_t buffer_line =0;
  uint16_t pixel = 0;
  uint8_t *rgb;
  uint8_t *sync;	
  uint32_t flip = 0;
    
  z80io_setup();
  fill_scan((uint8_t *)RGB_buffer[0], sbuffer, abuffer, 0);
  fill_scan((uint8_t *)RGB_buffer[1], sbuffer, abuffer, 0);
  uint32_t bstart = 0;
  uint32_t bold =0;
  uint32_t vb;
  // dma_channel_set_read_addr(rgb_chan_0, &RGB_buffer[0], true);
  pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));
 
  while (1) {   
    
    if (scanline <  480) {
      bstart = (scanline / 16)*80;
      if (flip==1)flip=0;
      else if (flip==0)flip=1;
      rgb = (uint8_t *) RGB_buffer[flip];	    
      dma_channel_set_read_addr(rgb_chan_0, rgb, true);
      //fill the buffer for the flip
      scanline++;      
      fill_scan(RGB_buffer[((flip+1)%2)], (char *)(sbuffer+bstart),
		(char *)(abuffer+bstart),scanline%16);
      //while(dma_channel_is_busy(rgb_chan_0)){
	//bus_read();
      //      }
      dma_channel_wait_for_finish_blocking(rgb_chan_0);
    }
    
    else {
    scanline =0;
    //continue;
    }
    //we could alternate buffers, assign blocks, etc. 
    
  }
}
