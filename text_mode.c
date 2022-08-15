#include "main.h"
#include "text_mode.h"
#include "stdlib.h"
#include "font.h"


void fill_background() {
  for (int i =0; i < 25; i++)
    for (int j = 0;  j < 80; j++) {
      sbuffer[(i*j)+i] =' ' ;
      abuffer[(i*j)+i] = 0;
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

