#include "main.h"
#include "text_mode.h"
#include "stdlib.h"
#include "font.h"


void fill_background() {
  for (int i =0; i < ROW; i++)
    for (int j = 0;  j < COL; j++) {
      sbuffer[(i*COL)+j] = ' ';
      abuffer[(i*COL)+j] = 0;
    }
  //static string.
  strcpy(sbuffer, "Initial buffer - IO 39-10 to reset");
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
//uint32_t unpacked_font[8400];
//uint32_t unpacked_mask[8400];

void unpack_font() {
  /*
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
      unpacked_font[((i*2)*16)+(l*2)] = *((uint32_t *) buffer);
      unpacked_font[((i*2)*16)+(l*2)+1] = *((uint32_t *) (buffer+4));
      unpacked_mask[((i*2)*16)+(l*2)] = *((uint32_t *) mask);
      unpacked_mask[((i*2)*16)+(l*2)+1] = *((uint32_t *) (mask+4));
    }
  }
  */
}


/*
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
*/

void fill_scan(uint8_t *buffer, char *string, char*attr, int line, int frame) {
  unsigned int p=0;
  uint32_t *b= (uint32_t *) buffer;
  uint8_t stats=0;
  uint32_t offs=0;
  uint32_t f0,f1,b0,b1;
  uint32_t foreground =  0x00;
  uint32_t background =  0x00;
  uint32_t tmp;
  for (int i =0; i < COL; i++) {
    p = 2*i;    
    offs = ((string[i]*2)*16)+(2*line);
    stats = attr[i];
    if (stats){
      if (stats & BOLD) {
	foreground = 0x1F1F1F1F;
	background = 0x00000000;
      }
      if (stats & REVERSE) {
	tmp = foreground;
	foreground = background;
	background = tmp;
      }
      if ((stats & BLINK) && (frame%60 < 30)) {
	foreground = background;	
      }
	
    }
    else {
      foreground = 0xFFFFFFFF;
      background = 0x00000000;   
    }
    if ((stats & UNDERSCORE) && line==15){
      b[p] = foreground;
      b[p+1] = foreground;
    }
    else{
      get_f_at(string[i], line, &f0, &f1, &b0,&b1);
      b[p] = f0 & foreground | (b0 & background);  
      b[p+1] = f1 & foreground | (b1 & background);         
    }
  }
  b[160]=0;
}

