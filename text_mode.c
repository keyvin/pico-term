#include "main.h"
#include "text_mode.h"
#include "stdlib.h"
#include "font.h"

uint32_t t_buffer[ROW*COL+1];


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

    if (attr){
      if (attr & BOLD || attr & DIM) {
	rgb_foreground = 0x1F1F1F1F;
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

