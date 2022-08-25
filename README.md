# pico-term
pi-pico terminal

Implements an 80x30 text terminal at 640x480 resolution. Currently supports most of the ANSI commands supported by the vt102. 
Will support 256 colors as soon as attribute codes and a lookup table are implemented.

Needs 10 pins for color output, 2 pins for serial, optional bell/piezeo on a pin. 

Most extended keys are not implemented yet on the keyboard. 

