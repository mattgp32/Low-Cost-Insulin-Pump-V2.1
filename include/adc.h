/* This module written by Matt Payne as part of the Bluetooth insulin pump project.
   This module is the .h file for adc.c
   Started on 9/3/2023
*/

#ifndef ADC_H_INCLUDED
#define ADC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <stdbool.h>
#include "stdio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

void get_batt_level(void* arg);
void print_batt_level(void* arg);
void read_pot(void);
void adc_init(void);


#ifdef _cplusplus
}
#endif
#endif