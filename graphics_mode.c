#include "main.h"
#include "graphics_mode.h"

//extern to module
uint8_t g_dmabuff1[650];
uint8_t g_dmabuff2[650];
uint8_t g_fbuffer[HRES*VRES];
uint32_t g_position;
uint32_t g_cursor_position;
uint8_t g_bytes_processed;
uint8_t g_active_register;
uint8_t g_pixel_size;

void g_mode_write(uint8_t ch){
  uint8_t xh;
  uint8_t yh;
  uint16_t tm;
  switch (g_active_register) {
  case G_BUFFER:    
    if (g_pixel_size < 2 ) {
      g_fbuffer[g_cursor_position]= ch;
      g_cursor_position++;
      if (g_cursor_position >=HRES*VRES)
	g_cursor_position = 0;
    }
    else {
      if ((g_cursor_position%HRES)+g_pixel_size >=HRES)
	xh = HRES-((g_cursor_position%HRES));      
      else
	xh=g_pixel_size;
      if ((g_cursor_position/VRES) + g_pixel_size >=VRES)
	yh = VRES-((g_cursor_position/VRES));
      else
	yh= g_pixel_size;
	  
      for (int j = 0; j < yh; j++) {
	uint32_t buff_base = g_cursor_position + j*HRES;
	for (int i = 0; i < xh; i++){
	  g_fbuffer[buff_base+i] = ch;
	}
      }
      
    }
    break;
  case G_SET_X:
    if (g_bytes_processed !=0){
      g_cursor_position = g_cursor_position + 0xFF;
      g_bytes_processed =0;
      g_active_register=0;
    }
    else {
      g_cursor_position = (g_cursor_position/HRES)*HRES+ch;
      g_bytes_processed++;
    }
    if (g_cursor_position >= HRES*VRES)g_cursor_position=0;
    break;
    
  case G_SET_Y:
    tm = g_cursor_position % HRES;
    g_cursor_position = (ch%VRES)*HRES + tm;
    g_active_register = 0;
    break;
  case G_SET_PIXEL_SIZE:
    g_pixel_size=ch;
    g_active_register = 0;
  case G_RESET_REGS:
  default:
    g_pixel_size = 0;
    g_bytes_processed=0;
    g_cursor_position=0;
    g_active_register=0;
  }
}

char g_mode_read(){
  uint8_t t=0;
  switch (g_active_register) {
  case G_BUFFER:
    t= g_fbuffer[g_cursor_position++];
    if (g_cursor_position >=HRES*VRES)g_cursor_position=0;
    return t;
    break;
  case G_SET_PIXEL_SIZE:
    return g_pixel_size;

  }
  return 0;
}
    

void solid_fill(uint8_t color) {
  for (int i = 0; i < HRES*VRES; i++)
    g_fbuffer[i] = color; 
}

void pattern() {
  for (int i = 0; i < HRES*VRES;i++){
    if ((i/320)>5&& (i/320) < 30)
      g_fbuffer[i] = 0xFF;
  }
}
void gfill_scan(uint8_t *buffer, int sline) {
  int start = (sline/2)*320;
  int s = 0;
  for (int sweep = 0; sweep < 320; sweep++) {
    s = sweep*2;	
    buffer[s] = g_fbuffer[start+sweep];	
    buffer[s+1] = g_fbuffer[start+sweep];
  }
  for (int i =0; i <10;i++){
  buffer[640+i]=0;

  }
}


void graphics_mode(){
  
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
			g_dmabuff1,             // The initial read address (pixel color array)
			648,                    // Number of transfers; in this case each is 1 byte.
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
  gfill_scan(g_dmabuff1,0);
  gfill_scan(g_dmabuff2,0);
  uint8_t *sending = g_dmabuff1;
  uint8_t *filling = g_dmabuff2;
  uint8_t *tmp;

  uint16_t scanline = 0;
  pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));
  while (mode_change==false) {
    
    tmp = filling;
    filling = sending;	    
    sending = tmp;
    dma_channel_set_read_addr(rgb_chan_0, sending, true);
    //fill the buffer for the flip    
    //    scanline+=16;
    scanline=scanline+1;
    if (pio_interrupt_get(pio,5)) {
      pio_interrupt_clear(pio,5); //irq 5 shows we are in vblank. Do not clear
	                          //vblank before last scanline
      if (scanline < 480){
	scanline = 0;
	dma_channel_abort(rgb_chan_0);
	pio_sm_clear_fifos(pio, rgb_sm);
	pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);
	pio_sm_restart(pio,rgb_sm);
	pio_sm_clear_fifos(pio, rgb_sm);
      }
      
    }
    if (scanline==480){	
      scanline=0;
      // frame++;
    }
    
    gfill_scan(filling, scanline);
    
    
    
    dma_channel_wait_for_finish_blocking(rgb_chan_0);
  }
  pio_enable_sm_mask_in_sync(pio, 0);
}
  




