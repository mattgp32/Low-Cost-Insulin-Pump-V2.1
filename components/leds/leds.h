/* This module written by Matt Payne as part of the Bluetooth insulin pump project.
   This module is designed to control the leds on the board
   Started on 10/3/2023
*/

#ifndef LEDS_H_INCLUDED
#define LEDS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <stdbool.h>
#include "stdio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void init_leds( void );
void led_on(int LED);
void led_off(int LED);
void display_batt_level(void* arg);
void led_five_flash();
void led_double_flash();
void buzzer_init(void);
void annoying_buzzer(void* arg);
void no_br_warning(void*arg);
void BT_running_alert(void*args);
void led_wave();
void pump_is_alive(void*args);

#ifdef _cplusplus
}
#endif
#endif