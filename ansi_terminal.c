#include "ansi_terminal.h"
#include "text_mode.h"
#include <stdlib.h>
#include <stdio.h>
bool start_escape;
bool in_escape;
char escape_buffer[MAX_ARG_LENGTH+1];
uint8_t escape_buffer_position;
unsigned int escape_arguments[MAX_ESCAPE_ARGUMENTS];
uint8_t num_arguments;
uint8_t cursor_attributes;




#define T_BLACK 0
#define T_RED 1
#define T_GREEN 2
#define T_YELLOW 3
#define T_BLUE 4
#define T_MAGENTA 5
#define T_TURQUOIS 6
#define T_WHITE 7
#define T_DEFAULT 8

uint8_t COLOR_CODE[] = {T_BLACK, T_RED, T_GREEN,T_YELLOW,T_BLUE,T_MAGENTA,T_TURQUOIS, T_WHITE, T_DEFAULT};



uint8_t cursor_attributes = 0;
uint8_t num_arguments = 0;
bool at_eol = false;
uint16_t old_cursor;
uint8_t current_foreground = 8;
uint8_t current_background = 0;
uint32_t current_attributes_packed = 0;




void dma_blank_reigon(uint32_t *dest, uint32_t count){
  int chan1 = 1; //dma channel 1.
  uint32_t zero=0;
  dma_channel_config c1 = dma_channel_get_default_config(chan1);  // default configs
  channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
  channel_config_set_read_increment(&c1, false);                       
  channel_config_set_write_increment(&c1, true);                      
  
  dma_channel_configure(
			chan1,                
			&c1,                  
			dest,          
			&zero,           
			count,       
			true                     
    );
  dma_channel_wait_for_finish_blocking(chan1);

}


void dma_copy_reigon(uint32_t *source, uint32_t *dest, uint32_t count){
  int chan1 = 5; //dma channel 1. 
  dma_channel_config c1 = dma_channel_get_default_config(chan1);  // default configs
  channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
  channel_config_set_read_increment(&c1, true);                       
  channel_config_set_write_increment(&c1, true);                      
  //set priority to be LOW when lots of scrolling. 
  //channel_config_set_dreq(&c1, 0x3b);
  // dma_timer_set_fraction(0,1,);
  dma_channel_configure(
			chan1,                
			&c1,                  
			dest,          
			source,           
			count,       
			true                     
    );
  dma_channel_wait_for_finish_blocking(chan1);

}


void scroll_screen(){
  //use DMA to scroll the screen. 
  dma_copy_reigon(t_buffer+COL, t_buffer, LAST_CHAR-COL);
  /*for (int a=0; a < (LAST_CHAR-COL); a++){
    t_buffer[a]=t_buffer[a+COL];
    sleep_us(50);
    }*/

  for (int a =LAST_CHAR; a >=(LAST_CHAR-COL);a--){
    t_buffer[a]=0;
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
      current_foreground=T_WHITE;
      current_background=T_BLACK;
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
    default:          //must be a color code
      if (escape_arguments[a] >=30 && escape_arguments[a] <40)
	current_foreground=escape_arguments[a]-30;
      else if (escape_arguments[a] >=40 && escape_arguments[a] < 50)
	current_background=escape_arguments[a]-40;
     
    }
  }
  current_attributes_packed = pack_cell(0,cursor_attributes, current_foreground, current_background);
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
	  t_buffer[a] = current_attributes_packed;
	}
      }
      //clear from cursor up (inclusive)
      else if (command =='1') {
	for (int a = cursor; a >=0; a--) {
	  t_buffer[a] = current_attributes_packed;
	}
      }
      //clear screen
      else if (command =='2') {
	for (int a = 0; a < LAST_CHAR; a++) {
	  t_buffer[a] = current_attributes_packed;
	}
      }
      else if (command =='3') {}
    }
    else if (c == 'K') {
      //From Cursor to end of line inclusive of cursor
      if (escape_buffer_position ==0 || command =='0') {    
	working = cursor;
	int tmp = C_GET_ROW(cursor);        //while on same row
	while (C_GET_ROW(working) == tmp) {
	  t_buffer[working] = current_attributes_packed;
	  working++;
	}
	
      }
      //from start of line to cursor
      else if (command =='1') {
	working = C_START_OF_ROW(cursor);         //while on same row
	while (working != cursor) {
	  t_buffer[working]= current_attributes_packed;
	  working++;
	}
	t_buffer[working]=current_attributes_packed;
      }
      //Clear line
      else if (command =='2') { 
	working = C_START_OF_ROW(cursor);              
	for (int a =working; a < cursor+COL; a++){
	  t_buffer[working]=current_attributes_packed;
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
    //TODO - This is wrong
    if (source >=LAST_CHAR) {
      for (int a=LAST_CHAR; a >= LAST_CHAR-COL; a--){
	t_buffer[a]=0;
	source = COL*(ROW-1);
      }        
    }
    else {
      int t=destination;
      //from source to end of line
      for (int a=source; a <LAST_CHAR;a++) {
	t_buffer[t]=t_buffer[a];
	t++;
      }
      t = num_lines*COL;
      while (t >= 0){
	t_buffer[LAST_CHAR-t]=current_attributes_packed;
	t--;
      }
      cursor = C_START_OF_ROW(cursor);
    }
  }
  if (c == 'L') {
    //insert lines
    int source = C_START_OF_ROW(cursor);;
    int destination = source + COL*num_lines;
    //TODO - this is likely wrong
    if (destination >= LAST_CHAR) {
      //blank current line to end of screen and set cursor to start of row
      cursor = C_START_OF_ROW(cursor);
      for (int i = cursor; i < LAST_CHAR; i++){
	t_buffer[i] = current_attributes_packed;
      }
    }
    else {
      for (int i=LAST_CHAR-1; i >= destination;i--){
	t_buffer[i]=t_buffer[i-(COL*num_lines)];
      }
      while (source != destination) {
	t_buffer[source] = current_attributes_packed;
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
      t_buffer[cursor] = pack_cell(c, cursor_attributes, current_foreground, current_background);
      if (C_GET_COL(cursor)==COL-1){   
	at_eol=true;
      }
      else {
	cursor++;
      }     
    }
    else if (c=='\r'){
      cursor = C_START_OF_ROW(cursor); //Place at start of row
    }
    else if (c=='\n') {
      cursor = cursor + COL;  //Advance cursor one row
    }
    else if (c == '\b' ) {    
      if (cursor!=0 && C_GET_COL(cursor)!=0)
	cursor--;
      t_buffer[cursor]= pack_cell(0, cursor_attributes, current_foreground, current_background);
    }
    else if (c == 0x1B) {
      start_escape = true;
    }
    else if (c == 0x7F) {
    }
  }
  else if (start_escape ==true){
    start_escape = false;
    if (c == '['){
      in_escape=true;
    }
    if (c == 'M') {               //TODO - 7 bit DEC codes
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

 
    switch (c) {
    case 'H':
    case 'f':
      handle_h(c);    //position cursor absolute
      in_escape=false;    
      break;
    case 'A':         //Position cursor relative
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
    case 'J':           //Erase in screen
    case 'K':
      handle_erase(c);
      in_escape=false;
      break;
    case 'L':           //Insert and Delete lines
    case 'M':
      handle_insert(c);
      in_escape=false;
      break;
    case 'm':
      handle_m(c);      //set attributes
      in_escape=false;
      break;
    case ';':          //terminate an argument
      get_argument();
      break;
    case 0x1b:
      in_escape=false;
      break;
    default:
      escape_buffer[escape_buffer_position] = c; //we are reading part of an argument
      escape_buffer_position++;
      escape_buffer[escape_buffer_position]='\0';
      if ((c >= 'a' && c <= 'z') || (c >='A' && c<= 'Z')) //Invalid or unsupported code
	escape_buffer_position=MAX_ARG_LENGTH;            //trigger end of argument
      if (escape_buffer_position >= MAX_ARG_LENGTH) {
	in_escape = false;
      }
    
    }
    if (in_escape==false) {                            //finished or aborted escape sequence
      escape_buffer_position = 0;                      //zero escape argument read buffer
      num_arguments = 0;                               //and zero number of current arguments
    }
  }
  if (cursor >= LAST_CHAR) {             //if cursor is beyond last position,
    scroll_screen();                     //scroll the screen
    cursor = LAST_ROW_START;
  }
    
}
