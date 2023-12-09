/* This module written by Matt Payne as part of the Bluetooth insulin pump project.
   This module is used to store and control insulin delivery data
   Started on 20/3/2023
*/

#ifndef INS_RATE_H_INCLUDED
#define INS_RATE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <stdbool.h>
#include "stdio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void read_and_store_data(const char *data);
uint8_t slice_string(const char *data, const char *ast);
void init_rate_storage_nvs_partition(void);
void write_basal_rate_data(int delivery_amount);
void retreive_data(void* arg);
void get_current_time(void* arg);
void give_insulin(void* arg);
void bolus_delivery(void* arg);
int set_delivery_frequency_test(int freq);
void rewind_plunge(void* arg);
bool check_bolus_cancelled();
void begin_low_power(void*args);

#ifdef _cplusplus
}
#endif
#endif