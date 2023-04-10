#include "main.h"
#include "text_mode.h"
#include "stdlib.h"
#include "font.h"

uint cursor;

uint32_t t_buffer[ROW*COL+1];


//DMA BUFFERS
uint32_t RGB_buffer1[16*180]; //8bpp
uint32_t RGB_buffer2[16*180];



uint32_t eight_color_mode[] = {
			       0x00000000, //black   //30 foreground, 40 bg
			       0xD0D0D0D0, //red       31 fg          41 bg
			       0x38383838,  //Green     32             42
			       0xF8F8F8F8,  //yellow    33             43			     
			       0x03030303,  //blue      34             44
			       0xD3D3D3D3, //Magenta    35             45
			       0x3F3F3F3F, //Turquoise  36             46
			       0xFFFFFFFF, //white      37             47   
			       0XFFFFFFFF, //Default    39             49
			       0x00000000
}; //reset codes
			       


void fill_background() {
  for (int i =0; i < ROW; i++)
    for (int j = 0;  j < COL; j++) {
      t_buffer[i*COL+j] = 0;
    }
  //t_buffer[5] = pack_cell('H',0,0,0);
  //t_buffer[4] = pack_cell('I',0,0,0);
  //static string.
  //strcpy(sbuffer, "Initial buffer - IO 39-10 to reset");
}

uint32_t font_table[256][2];
uint32_t mask_table[256][2];

void build_f_table(){
  uint8_t offs=0;
  uint8_t buffer[8];
  uint8_t mask[8];

  for (int a =0; a< 256; a++) {

    offs = a;
    buffer[7] = offs & 0x01 ? 255: 0;
    mask[7] = offs & 0x01 ? 0: 255;
    buffer[6] = offs & 0x02 ? 255: 0;
    mask[6] = offs & 0x02 ? 0:255;
    buffer[5] = offs & 0x04 ? 255: 0;
    mask[5] = offs & 0x04 ? 0:255;
    buffer[4] = offs & 0x08 ? 255: 0;
    mask[4] = offs & 0x08 ? 0:255;
    buffer[3] = offs & 0x10 ? 255: 0;
    mask[3] = offs & 0x10 ? 0:255;
    buffer[2] = offs & 0x20 ? 255: 0;
    mask[2] = offs & 0x20 ? 0:255;
    buffer[1] = offs & 0x40 ? 255: 0;
    mask[1] = offs & 0x40 ? 0:255;
    buffer[0] = offs & 0x80 ? 255: 0;
    mask[0] = offs & 0x80 ? 0:255;
    font_table[a][0] = *((uint32_t *) buffer);
    font_table[a][1] = *((uint32_t *) (buffer+4));
    mask_table[a][0] = *((uint32_t *) mask);
    mask_table[a][1] = *((uint32_t *) (mask+4));
  }

}

void get_f_at(char chr, int line,  uint32_t *f0, uint32_t *f1, uint32_t *b0, uint32_t *b1){
  uint8_t offs = VGA8_F16[(chr*16)+line];
  *f0 = font_table[offs][0];
  *f1 = font_table[offs][1];
  *b0 = mask_table[offs][0];
  *b1 = mask_table[offs][1];
  return;
}

void unpack_cell(uint32_t cell, uint8_t *val, uint8_t *attr, uint8_t *fg, uint8_t *bg){
  *val=T_GET_CHARACTER(cell);
  *attr=T_GET_ATTRIBUTE(cell);
  *fg=T_GET_FOREGROUND(cell);
  *bg=T_GET_BACKGROUND(cell);
}

uint32_t pack_cell(uint8_t val, uint8_t attr, uint8_t fg, uint8_t bg){
  uint32_t working1=0;
  uint32_t working2=0;
  uint32_t working3=0;
  uint32_t working4=0;
  uint32_t ret_val=0;
  working1=val;
  working2=attr;
  working3=fg;
  working4=bg;
  ret_val = working1 | (working2 << T_ATTRIBUTE) | (working3 << T_FOREGROUND)
						    | (working4 << T_BACKGROUND);

  return ret_val;
}

void fill_scan(uint32_t *buffer, uint32_t *t_row, int line, int frame) {
  unsigned int p=0;
  uint32_t *b=  buffer;
  uint8_t attr=0;  //atrribute
  uint8_t fg=0;    //foreground color for cell
  uint8_t bg=0;    //background color for cell
  uint8_t ch='\0'; //text at cell
  
  
  uint32_t offs=0;
  uint32_t f0,f1,b0,b1;   //masks
  uint32_t rgb_foreground =  0x00;
  uint32_t rgb_background =  0x00;
  uint32_t rgb_tmp;
  uint32_t l = 2*line;

  for (int i =0; i < COL; i++) {
    unpack_cell(t_row[i], &ch, &attr, &fg, &bg);
    p = 2*i;    
    offs = ((ch*2)*16)+l;
    rgb_foreground = 0xFFFFFFFF;  //default masks
    rgb_background = 0x00000000;   

/*    if (attr){
      if (attr & BOLD || attr & DIM) {
	rgb_foreground = 0xD0D0D0D0;  //what color?
	rgb_background = 0x00000000;
      }
      if (attr & REVERSE) {
	rgb_tmp = rgb_foreground;
	rgb_foreground = rgb_background;
	rgb_background = rgb_tmp;
      }
      if ((attr & BLINK) && (frame%60 < 30)) {
	rgb_foreground = rgb_background;	
      }
	
    }
*/
    if ((attr & UNDERSCORE) && line==15){
      b[p] = rgb_foreground;
      b[p+1] = rgb_foreground;
     }
    else{
      get_f_at(ch, line, &f0, &f1, &b0,&b1);
      b[p] = f0 & rgb_foreground | (b0 & rgb_background);  
      b[p+1] = f1 & rgb_foreground | (b1 & rgb_background);         
    }
  }
  b[160]=0;
}

//if we need memory, we can dynamically allocate and free. 
void do_text_mode() {
  PIO pio = pio0;
  pio_clear_instruction_memory(pio);
  uint hsync_offset = pio_add_program(pio, &hsync_program);
  uint vsync_offset = pio_add_program(pio, &vsync_program);
  uint rgb_offset = pio_add_program(pio, &nrgb_program);
  
  // Manually select a few state machines from pio instance pio0.
  uint hsync_sm = 0;
  uint vsync_sm = 1;
  uint rgb_sm = 2;
  float freq = PIXEL_CLOCK;
  float div1 = ((float)clock_get_hz(clk_sys)) / freq;
  float div2 = ((float)clock_get_hz(clk_sys)) / (freq*3); //run it 3 times faster?
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
  pio_sm_clear_fifos(pio, hsync_sm);
  pio_sm_clear_fifos(pio, vsync_sm);
  pio_sm_clear_fifos(pio, rgb_sm);
 
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
  uint32_t frame=0;
  for (int i =0; i <2300;i++)t_buffer[i]=40+(i%10);
  
  for (int i=0;i < 16; i++){
    fill_scan(RGB_buffer1+(i*162), t_buffer, i, 0);
    fill_scan(RGB_buffer2+(i*162), t_buffer, i, 0);
  }
  uint32_t bstart = 0;
  uint32_t vb;
  pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

  uint32_t *ptr;
  uint32_t *tmp_p;
  rgb = (uint32_t *) RGB_buffer1;
  rgb_n = (uint32_t *) RGB_buffer2;
  bstart = 0;
  //  gpio_set_dir(0,true);
  //gpio_set_function(0, GPIO_FUNC_PIO0);
  while (mode_change == false) {   
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
      fill_scan(rgb_n +(i*160), (uint32_t *)(t_buffer+(bstart*COL)),i,frame);
    }
    if ((frame%60)<30 &&  (cursor/COL)==bstart) {
      for (int i=0; i <16; i++) {
	//TODO - should be cursor color/mode
	rgb_n[C_GET_COL(cursor)*2+(i*160)]=0xFFFFFFFF;
	rgb_n[C_GET_COL(cursor)*2+(i*160)+1]=0xFFFFFFFF;
      }
      
    }   
        
    dma_channel_wait_for_finish_blocking(rgb_chan_0);
  }
  pio_enable_sm_mask_in_sync(pio, 0);
}

