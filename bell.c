#include "bell.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

bool bell_is_on;


void start_bell(uint16_t hz){
  bell_is_on=true;;
  uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
  gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);   
  pwm_set_clkdiv(slice_num, (clock_get_hz(clk_sys)/100)); //should result in 1mhz pwm clock
  pwm_set_wrap(slice_num, hz);
  pwm_set_chan_level(slice_num, PWM_CHAN_A, 900);
  pwm_set_chan_level(slice_num, PWM_CHAN_B, 900);
  pwm_set_enabled(slice_num, true);
}

void stop_bell() {
  bell_is_on=false;
  uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
  pwm_set_enabled(slice_num, false);

}
