//Recieve buffer
#define RECIEVE_BUFFER_SIZE 2000 //far in excess of the 240 bytes that could arrive
                                 //during frame.
uint16_t recieve_buffer_head =0;
uint16_t recieve_buffer_tail =0;
uint8_t recieve_ring_buffer[RECIEVE_BUFFER_SIZE];
char recieve_buffer_get(){
  
  char to_ret = '\0';
  if (recieve_buffer_head == recieve_buffer_tail)
    return '\0'; //empty
  to_ret = recieve_ring_buffer[recieve_buffer_head];
  recieve_buffer_head++;
  if (recieve_buffer_head == RECIEVE_BUFFER_SIZE)
    recieve_buffer_head = 0;

  
  return to_ret;
}

bool recieve_buffer_put(char recieved) {
  uint16_t rbn= recieve_buffer_tail+1; //recieve buffer next;
  if (rbn == RECIEVE_BUFFER_SIZE)
    rbn=0;
  if (rbn == recieve_buffer_head)
    return false; //full
  recieve_ring_buffer[recieve_buffer_tail] = recieved;
  recieve_buffer_tail = rbn;
  return true;
}

bool recieve_buffer_full() {
uint16_t rbn= recieve_buffer_tail+1; //recieve buffer next;
  if (rbn == RECIEVE_BUFFER_SIZE)
    rbn=0;
  if (rbn == recieve_buffer_head)
    return false; //full
}

bool recieve_buffer_ready() {
  if ((recieve_buffer_head != recieve_buffer_tail))  
    return true;
  return false;
}

void clear_ring_buffer() {
  recieve_buffer_head = 0;
  recieve_buffer_tail = 0;
  for(uint16_t i =0; i < RECIEVE_BUFFER_SIZE; i++)
    recieve_ring_buffer[i] = '\0';
}

