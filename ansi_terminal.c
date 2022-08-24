#include "ansi_terminal.h"
#include "text_mode.h"
#include <stdlib.h>
#include <stdio.h>

#define C_START_OF_ROW(C) ((C/COL)*COL)
#define C_END_OF_ROW(C) ((C/COL)*COL+(COL-1))
#define C_START_OF_NEXT_ROW(C) ((C/COL)*COL+COL)
#define LAST_ROW_START (LAST_CHAR-COL)
#define C_GET_ROW(C) (C/COL)
#define C_GET_COL(C) (C%COL)



uint8_t cursor_attributes = 0;
uint8_t num_arguments = 0;
bool at_eol = false;
uint16_t old_cursor;


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






void scroll_screen(){

  //use DMA channel to scroll the screen. 
  int chan1 = 1; //dma channel 1. 
  dma_channel_config c1 = dma_channel_get_default_config(chan1);  // default configs
  channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
  channel_config_set_read_increment(&c1, true);                       
  channel_config_set_write_increment(&c1, true);                      
  
  dma_channel_configure(
			chan1,                
			&c1,                  
			sbuffer,          
			sbuffer+COL,           
			(LAST_CHAR-COL)/4,       
			true                     
    );
  dma_channel_wait_for_finish_blocking(chan1);
  
  dma_channel_configure(
			chan1,                 // Channel to be configured
			&c1,                        // The configuration we just created
			abuffer,          // write address (RGB PIO TX FIFO)
			abuffer+COL,            // The initial read address (pixel color array)
			(LAST_CHAR-COL)/4,                    // Number of transfers; in this case each is 1 byte.
			true                       // start immediately.
    );

  dma_channel_wait_for_finish_blocking(chan1);

  for (int a =LAST_CHAR; a >=(LAST_CHAR-COL);a--){
    abuffer[a]=0;
    sbuffer[a]=0;
  }
}

void get_argument() {
  unsigned int argument;
  int conversions = 0;
  if (escape_buffer_position > 0) {
    conversions = sscanf(escape_buffer, "%u", &argument);
    if (conversions == 1){
      if (num_arguments != MAX_ESCAPE_ARGUMENTS){
	escape_arguments[num_arguments++] = argument;
	escape_buffer_position=0;
	escape_buffer[0]='\0';
	return;
      }
    }
  }
  in_escape=false;
  return;
}

void handle_m(char c){
  if (escape_buffer_position !=0){
    get_argument();
    if (in_escape==false) return;
  }
  if (num_arguments ==0) return;
  for (int a = 0; a < num_arguments; a++){
    switch (escape_arguments[a]){
    case 0:
      cursor_attributes = 0;
      break;
    case 1:
      cursor_attributes = cursor_attributes | BOLD;
      break;
    case 2:
      cursor_attributes = cursor_attributes| DIM;
    case 4:
      cursor_attributes = cursor_attributes | UNDERSCORE;
      break;
    case 5:
      cursor_attributes = cursor_attributes | BLINK;
      break;
    case 7:
     cursor_attributes = cursor_attributes | REVERSE;
     break;
    case 22:
      cursor_attributes = (cursor_attributes | BOLD) ^BOLD;
      cursor_attributes = (cursor_attributes | DIM) ^DIM;
      break;
    case 24:
      cursor_attributes = (cursor_attributes | UNDERSCORE) ^UNDERSCORE;
      break;
    case 27:
      cursor_attributes = (cursor_attributes | REVERSE) ^ REVERSE;
      break;
    }
  }
  in_escape=false;
}


void handle_h(char c){
   unsigned int t_row = 0;
   unsigned int t_col = 0;
   uint8_t conversions = 0;
   if (escape_buffer_position ==0 && (c == 'H' || c=='f')) {
    cursor = 0;
   }
   else{
     get_argument();
     if (num_arguments != 2){      
       return;
     }
     t_row = escape_arguments[0];
     t_col = escape_arguments[1];
 
     if (t_row > ROW || t_col > COL || t_col < 1 || t_row <1)  {
       return;
     }
     //ANSI home is 1,1
     cursor = (t_row-1)*COL + (t_col-1);

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
  if (c =='A') {                  //move up move_count lines
    if ((cursor/COL) - move_count <0) 
      cursor = cursor % COL;    //stop at top
    else
      cursor = cursor - (move_count*COL);  //move up
  }
  if (c == 'B') {
    if (cursor + move_count*COL >=LAST_CHAR) //below last line?
      cursor = LAST_ROW_START+(cursor%COL);        //place in same column on last row
    else
      cursor = cursor + (move_count*COL);    //move down
  }
  if (c == 'C') {                               //move right
    if (cursor%COL + move_count >=COL)          //beyond column boundry?
      cursor = C_END_OF_ROW(cursor);      //place at end or row
     else
       cursor = cursor + move_count;    //move right
  }
  if (c == 'D') {                       //move left
    if (cursor%COL - move_count < 0)          
      cursor = C_START_OF_ROW(cursor);  //place at start of row;
    else
      cursor = cursor - move_count;
  }
  if (c == 'E') { 
  }
  
  if (c == 'F') {
    
  }
  if ( c == 'G') {
    cursor = C_START_OF_ROW(cursor) + move_count;
  }
  //tricky one. Keypresses.
  if (c == 'n' && move_count ==6) {
    
  }
  if (c == 's')
    old_cursor = cursor;
  if (c =='u')
    cursor = old_cursor;  
}

void handle_erase(char c){
  char command = escape_buffer[0];
  int working = 0;
  if (escape_buffer_position ==0 || escape_buffer_position == 1) {
    if (c == 'J') {
      //clear from cursor down (inclusive)
      if (escape_buffer_position ==0 || command == '0') {
	for (int a = cursor; a < LAST_CHAR; a++) {
	  sbuffer[a] = 0;
	  abuffer[a] = 0;
	}
      }
      //clear from cursor up (inclusive)
      else if (command =='1') {
	for (int a = cursor; a >=0; a--) {
	  sbuffer[a] = 0;
	  abuffer[a] = 0;
	}
      }
      //clear screen
      else if (command =='2') {
	for (int a = 0; a < LAST_CHAR; a++) {
	  sbuffer[a] = 0;
	  abuffer[a] = 0;
	}
	//cursor=0;
      }
      else if (command =='3') {}
    }
    else if (c == 'K') {
      //From Cursor to end of line inclusive of cursor
      if (escape_buffer_position ==0 || command =='0') {    
	working = cursor;
	int tmp = C_GET_ROW(cursor);        //while on same row
	while (C_GET_ROW(working) == tmp) {
	  abuffer[working] = 0;
	  sbuffer[working] = 0;
	  working++;
	}
	
      }
      //from start of line to cursor
      else if (command =='1') {
	working = C_START_OF_ROW(cursor);         //while on same row
	while (working != cursor) {
	  abuffer[working] = 0;
	  sbuffer[working] = 0;
	  working++;
	}
	abuffer[working]=0;
	sbuffer[working]=0;
      }
      //Clear line
      else if (command =='2') { 
	working = C_START_OF_ROW(cursor);              
	for (int a =working; a < cursor+COL; a++){
	  abuffer[a] = 0;
	  sbuffer[a] = 0;
	}
      }
    }
  }    
  
}

void handle_insert(char c) {

  int num_lines = 0;
  if (escape_buffer_position==0)
    num_lines = 1;
  else {
    get_argument();
    if (num_arguments==0) {
      //malformed...
      return;
    }
    num_lines = escape_arguments[0];
  }
  
  if (c == 'M') {
    //delete M lines
    int destination = C_START_OF_ROW(cursor);
    int source = destination + COL*num_lines;
    if (source >=LAST_CHAR) {
      for (int a=LAST_CHAR; a >= LAST_CHAR-COL; a--){
	abuffer[a]=0;
	sbuffer[a]=0;
	source = COL*(ROW-1);
      }
        
    }
    else {
      int t=destination;
      //from source to end of line
      for (int a=source; a <LAST_CHAR;a++) {
	abuffer[t]=abuffer[a];
	sbuffer[t]=sbuffer[a];
	t++;
      }
      t = num_lines*COL;
      while (t >= 0){
	abuffer[LAST_CHAR-t]='\0';
	sbuffer[LAST_CHAR-t]='\0';
	t--;
      }
      cursor = C_START_OF_ROW(cursor);
    }
  }
  if (c == 'L') {
    //insert lines
    int source = C_START_OF_ROW(cursor);;
    int destination = source + COL*num_lines;
    if (destination >= LAST_CHAR) {
      //blank current line to end of screen and set cursor to start of row
      cursor = C_START_OF_ROW(cursor);
      for (int i = cursor; i < LAST_CHAR; i++){
	abuffer[i] = '\0';
	sbuffer[i]= '\0';
      }
    }
    else {
      for (int i=LAST_CHAR-1; i >= destination;i--){
	abuffer[i]=abuffer[i-(COL*num_lines)];
	sbuffer[i]=sbuffer[i-(COL*num_lines)];

      }
      while (source != destination) {
	abuffer[source] = '\0';
	sbuffer[source] = '\0';
	source++;
      }
 
      cursor=C_START_OF_ROW(cursor);;

    }
  }

}

void process_recieve(char c) {
  
  if (!in_escape &&!start_escape) {
    if (c >= ' ' && c <= '~') {
      if (at_eol==true){           //emulate vt-100. Cursor doesn't wrap
	                          //until a printable character is recieved	                         
	if (C_GET_COL(cursor)==COL-1) {  //check that the cursor wasn't repositioned 
	  cursor++;	          //advance cursor
	}
	at_eol=false;   
      }
      sbuffer[cursor] = c;
      abuffer[cursor] = cursor_attributes;
      if (C_GET_COL(cursor)==COL-1){   
	at_eol=true;
      }
      else {
	cursor++;
      }      
    }
    else if (c=='\r'){
      cursor = C_START_OF_ROW(cursor); //integer division. Place at start of row
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
      if (C_GET_ROW(cursor) ==0)
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
      in_escape=false;
      break;
    case 'J':
    case 'K':
      handle_erase(c);
      in_escape=false;
      break;
    case 'L':
    case 'M':
      handle_insert(c);
      in_escape=false;
      break;
    case 'm':
      handle_m(c);
      in_escape=false;
      break;
    case ';':
      get_argument();
      break;
    case 0x1b:
      in_escape=false;
      break;
    default:
      escape_buffer[escape_buffer_position] = c;
      escape_buffer_position++;
      escape_buffer[escape_buffer_position]='\0';
      if ((c >= 'a' && c <= 'z') || (c >='A' && c<= 'Z')) //Unhandled code
	escape_buffer_position=MAX_ARG_LENGTH;   
      if (escape_buffer_position >= MAX_ARG_LENGTH) {
	in_escape = false;
      }
    
    }
    if (in_escape==false) {
      escape_buffer_position = 0;
      num_arguments = 0;      
    }
  }
  if (cursor >= LAST_CHAR) {
    scroll_screen();
    cursor = LAST_ROW_START;
  }
    
}
