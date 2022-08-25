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


