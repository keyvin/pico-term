// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ---- //
// nrgb //
// ---- //

#define nrgb_wrap_target 2
#define nrgb_wrap 7

static const uint16_t nrgb_program_instructions[] = {
    0x80a0, //  0: pull   block                      
    0xa047, //  1: mov    y, osr                     
            //     .wrap_target
    0xc045, //  2: irq    clear 5                    
    0xa022, //  3: mov    x, y                       
    0x24c1, //  4: wait   1 irq, 1               [4] 
    0x80a0, //  5: pull   block                      
    0x6208, //  6: out    pins, 8                [2] 
    0x0045, //  7: jmp    x--, 5                     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program nrgb_program = {
    .instructions = nrgb_program_instructions,
    .length = 8,
    .origin = -1,
};

static inline pio_sm_config nrgb_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + nrgb_wrap_target, offset + nrgb_wrap);
    return c;
}

static inline void nrgb_program_init(PIO pio, uint sm, uint offset, uint pin, float freq) {
        pio_sm_config c = nrgb_program_get_default_config(offset);
        //config, base, num
        sm_config_set_out_pins(&c, pin, 8);	
	sm_config_set_set_pins(&c, pin, 3);
        //connect pins to this pio
        for (int i=0;i<8;i++) pio_gpio_init(pio, pin+i);
        pio_sm_set_consecutive_pindirs(pio, sm, pin, 8, true);
	sm_config_set_clkdiv(&c,freq);
        //init on pio
        pio_sm_init(pio, sm, offset, &c);
}

#endif
