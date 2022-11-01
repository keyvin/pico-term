#include "keyboard.h"

char kb_buffer[KB_BUFFER_SIZE];
uint8_t kb_count;
bool read_ahead_enabled;


//primarily callback from hid_task.c. 
void keypress(char p) {
  if (read_ahead_enabled) {
    if (kb_count < KB_BUFFER_SIZE) {
      kb_buffer[kb_count]=p;
      kb_count++;
      
    }
  }
  else {
    kb_count=1;
    kb_buffer[0]=p;
  }
}

void init_keyboard(){
  kb_count=0;
  read_ahead_enabled=false;
  for (int a=0; a<KB_BUFFER_SIZE;a++){
    kb_buffer[a]=0;
  }
  return;
}

uint8_t get_keypress() {
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

void enable_readahead(){
  read_ahead_enabled = true;
}

void disable_readahead(){
  read_ahead_enabled = false;
}
