/*  **** Module taken from ESP-IDF and modified for use with the insulin pump project
    **** Started on 15/03/23
    **** This is the .h file for the BT defined in pump_BT.c
*/

#ifndef PUMP_BT_H_INCLUDED
#define PUMP_BT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void run_BT();
void print_transmission(void* arg);
void receive_BT_data(void* arg);
void process_bt_data(void* arg);

#ifdef _cplusplus
}
#endif
#endif