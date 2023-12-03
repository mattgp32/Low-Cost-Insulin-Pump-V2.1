/*  **** Module written by Matt Payne as part of UC Summer Scholarship 2019 - 2020 for the insulin pump project.
    **** Started on 22/11/2019
	**** Modified for BT Insulin Pump Project on 7/3/23
    **** This module contains a collection of functions to be used to drive the motor in the pump to deliver insulin.
*/

// define forward and backward directions and a variable to track how many steps the motor has turned.

#include "stdint.h"
#include <stdbool.h>
#include "stdio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"


#define nFAULT GPIO_NUM_14
#define DIR GPIO_NUM_11
#define STEP GPIO_NUM_12
#define BOOST_EN GPIO_NUM_45
#define MOTOR_DRIVER_ENABLE GPIO_NUM_13
#define nSLEEP GPIO_NUM_10

#define FORWARD 0
#define BACKWARD 1
#define DELAY_TIME 500

uint32_t steps_turned = 0;

void init_motor()
// Initialise all the pins required to allow the motor to be driven
{
    gpio_set_direction(nFAULT, GPIO_MODE_INPUT);
    gpio_set_direction(DIR, GPIO_MODE_OUTPUT);
    gpio_set_direction(STEP, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOOST_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_DRIVER_ENABLE, GPIO_MODE_OUTPUT);
	gpio_set_direction(nSLEEP, GPIO_MODE_OUTPUT);
    gpio_set_level(MOTOR_DRIVER_ENABLE, true);
}

void enable_motor()
{
	gpio_set_level(BOOST_EN, true);
    gpio_set_level(MOTOR_DRIVER_ENABLE, false);
	gpio_set_level(nSLEEP, true);
	vTaskDelay(200/portTICK_PERIOD_MS);
}

void disable_motor()
{
	gpio_set_level(BOOST_EN, false);
    gpio_set_level(MOTOR_DRIVER_ENABLE, true);
	gpio_set_level(nSLEEP, false);
	vTaskDelay(200/portTICK_PERIOD_MS);
}

bool read_motor_fault_pin()
// read the status of the motor fault pin. Return false if there is a fault with the motor and a true if all is good.
{
	bool fault_pin = gpio_get_level(nFAULT);
	return fault_pin;
}

void set_motor_direction(bool direction)
// call this function to change the value of the DIR pin on the IC which controls the direction the motor turns
{
	gpio_set_level(DIR, direction);
}


void step_motor(bool direction)
/* this function will call set_motor_direction() to set the motor function to turn one step in the direction of the passed variable. If there is a falult with the motor nothing will happen.
   One step will then be recorded in the steps_turned variable to keep track of how much the motor has turned */
{
		set_motor_direction(direction);
		gpio_set_level(STEP, 1);
		ets_delay_us(800);
		gpio_set_level(STEP, 0);
		ets_delay_us(800);
		steps_turned += 1;
	
	
}

void turn_x_steps(bool direction, uint16_t steps_to_turn)
/* this function will turn the motor x number of steps in the specified direction. x will have to be worked out from the basal rate, bolus rate, and motor testing to determine
   how much insulin is actually required */ 
{
	enable_motor();

	while(steps_to_turn > 0)
	{
		step_motor(direction);
		steps_to_turn -=1;
	}

	disable_motor();
}