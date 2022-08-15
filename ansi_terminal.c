#include "ansi_terminal.h"
#include "text_mode.h"
#include <stdlib.h>
#include <stdio.h>
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

uint16_t old_cursor;


void handle_h(char c){
   int t_row = 0;
   int t_col = 0;
   uint8_t conversions = 0;
   if (escape_buffer_position ==0 && c == 'H') {
    cursor = 0;
   }
   else{
    conversions = sscanf(escape_buffer, "%d;%d", &t_row, &t_col);
    //failure conditions
    if (conversions != 2){
      
      return;
    }
    if (t_row >= ROW || t_col >= COL) {
      cursor = 0;
      return;
    }
    cursor = t_row* COL + t_col;
   }
}


void handle_move(char c){
  int move_count = 0;
  uint8_t conversions = 0;
  conversions = sscanf(escape_buffer, "%d", &move_count);
  if (conversions !=1 && escape_buffer_position ==0) {
    move_count = 1;
  }
  else
    return;
  if (c =='A') {
    if ((cursor/COL) - move_count <0) 
      cursor = cursor % COL; 
    else
      cursor = cursor - (move_count*COL); 	  
  }
  if (c == 'B') {
    if (cursor + move_count*COL >=(ROW*COL))
      cursor =(ROW-1)*COL+cursor%COL;
    else
      cursor = cursor + (move_count*COL);
  }
  if (c == 'C') {
    if (cursor%COL + move_count >=COL)
      cursor = COL;
    else
      cursor = cursor + move_count;
  }
  if (c == 'D') {
    if (cursor%COL - move_count < 0)
      cursor = cursor/COL;
    else
      cursor - cursor - move_count;
  }
  if (c == 'E') { 
  }
  
  if (c == 'F') {
    
  }
  if ( c == 'G') {
    cursor = cursor/COL + move_count;
  }
  //tricky one. Keypresses.
  if (c == 'n' && move_count ==6) {
    
  }
  if (c == 's')
    old_cursor = cursor;
  if (c =='u')
    cursor = old_cursor;
  
  in_escape = false;
  escape_buffer_position = 0;

}

void handle_erase(char c){
  char command = escape_buffer[0];
  int working;
  if (escape_buffer_position ==0 || escape_buffer_position == 1) {
    if (c == 'J') {
      if (escape_buffer_position ==0 || command == '0') {
	for (int a = cursor; a < ROW*COL; a++) {
	  sbuffer[a] = 0;
	  abuffer[a] = 0;
	}
      }
      else if (command =='1') {
	for (int a = cursor; a >=0; a--) {
	  sbuffer[a] = 0;
	  abuffer[a] = 0;
	}
      }
      else if (command =='2') {
	for (int a = 0; a < ROW*COL; a++) {
	  sbuffer[a] = 0;
	  abuffer[a] = 0;
	}
	cursor = 0;
      }
      else if (command =='3') {}
    }
    if (c == 'K') {
      if (escape_buffer_position ==0 || command =='0') {
	working = cursor;
	
	while (working%COL != 0) {
	  abuffer[working] = 0;
	  sbuffer[working] = 0;
	  working++;
	}
      }
      if (command =='1') {
	working = cursor;
       
	while (working%COL != COL-1) {
	  abuffer[working] = 0;
	  sbuffer[working] = 0;
	  working--;
	}
	
	
      }
      if (command =='2') {
	working = cursor/COL;
	for (int a =0; a < COL; a++){
	  abuffer[a] = 0;
	  sbuffer[a] = 0;
	}
      }

    }
    
  }    
  in_escape = false;
  escape_buffer_position = 0;  
}

void scroll_screen(){
  uint32_t *ptr = (uint32_t *) sbuffer;
  for (unsigned int a = 0; a < ((ROW-1)*COL)/4; a++)
    ptr[a] = ptr[a+20];
  for (unsigned int a = 0; a < COL; a++)
    sbuffer[LAST_CHAR-a-1] = '\0';
  
}


void process_recieve(char c) {

  if (!in_escape &&!start_escape) {
    if (c >= ' ' && c <= '~') {

      sbuffer[cursor] = c;
      cursor++;    
    }
    else if (c=='\r'){
      cursor = (cursor / COL)*COL; //integer division. Place at start of row
    }
    else if (c=='\n') {
      cursor = cursor + COL;
    }
 
    else if (c == '\b' ) {
      if (cursor!=0)
	cursor--;
      sbuffer[cursor]='\0';
    }
    else if (c == 0x1B) {
      start_escape = true;
    }
  }
  else if (start_escape ==true){
    start_escape = false;
    if (c == '['){
      in_escape=true;
    }
    if (c == 'M') {
      if (cursor/COL ==0)
	cursor = 0;
      else
	cursor = cursor - COL;
    }
    if (c == '7'){
      old_cursor = cursor;
    }
    if (c== '8') {
      cursor = old_cursor;
    }
  }
 
  else {   //manage escape codes
    //^[H -- return home
    switch (c) {
    case 'H':
    case 'f':
      handle_h(c);
      escape_buffer_position=0;
      in_escape=false;
      break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 's':
    case 'u':
      handle_move(c);
      escape_buffer_position=0;
      in_escape=false;
      break;
    case 'J':
    case 'K':
      handle_erase(c);
      escape_buffer_position=0;
      in_escape=false;
      break;
    case 'm':
      in_escape=false;
      escape_buffer_position=0;
      break;
    default:
      escape_buffer[escape_buffer_position] = c;
      escape_buffer_position++;
      if ((c >= 'a' && c <= 'z') || (c >='A' && c<= 'Z')) //Unhandled code
	  escape_buffer_position=30;   
      if (escape_buffer_position >= 30) {
	in_escape = false;
	escape_buffer_position = 0;
      }
    }    
    
  }
  if (cursor >= LAST_CHAR) {
      scroll_screen();
      cursor = LAST_CHAR-COL;
  }
    
}
