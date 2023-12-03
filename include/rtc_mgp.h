/*  **** Module written by Matt Payne as part as part of the BT Ultra Low Cost Insulin Pump Project
    **** Started on 7/3/23
    **** This is the .h file for the motor functions defined in rtc.c
*/

#ifndef RTC_H_INCLUDED
#define RTC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <stdbool.h>
#include "stdio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"

void sleep_for_20(void* arg);


#ifdef __cplusplus
}
#endif
#endif
