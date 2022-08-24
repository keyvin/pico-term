#ifndef BELL_H
#define BELL_H 0
#include "pico/stdlib.h"



#define BUZZER_PIN 27

#define BELL_CHAR 0x07
#define BELL_HZ 1000
#define BELL_DURATION_US 800000 //half second
void start_bell(uint16_t hz);
void stop_bell();

extern bool bell_is_on;

#endif
