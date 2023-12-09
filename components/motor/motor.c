/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "stdint.h"
#include <stdbool.h>
#include "stdio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define nFAULT GPIO_NUM_14
#define DIR GPIO_NUM_11
#define STEP GPIO_NUM_12
#define BOOST_EN GPIO_NUM_45
#define MOTOR_DRIVER_ENABLE GPIO_NUM_13
#define nSLEEP GPIO_NUM_10

#define FORWARD 0
#define BACKWARD 1
#define DELAY_TIME 500

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uint32_t steps_turned = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Initialise all the pins required to allow the motor to be driven.
 */
void MOTOR_init ( void )
{
    gpio_set_direction(nFAULT, GPIO_MODE_INPUT);
    gpio_set_direction(DIR, GPIO_MODE_OUTPUT);
    gpio_set_direction(STEP, GPIO_MODE_OUTPUT);
    gpio_set_direction(BOOST_EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_DRIVER_ENABLE, GPIO_MODE_OUTPUT);
	gpio_set_direction(nSLEEP, GPIO_MODE_OUTPUT);
    gpio_set_level(MOTOR_DRIVER_ENABLE, true);
}

/*
 * Enable Motor Driver Functionality
 */
void MOTOR_disable ( void )
{
	gpio_set_level(BOOST_EN, false);
    gpio_set_level(MOTOR_DRIVER_ENABLE, true);
	gpio_set_level(nSLEEP, false);
	vTaskDelay(200/portTICK_PERIOD_MS);
}

/*
 * Read the status of the motor fault pin. Return false if there is a fault with the motor and a true if all is good.
 */
bool MOTOR_readFaultPin ( void )
{
	bool fault_pin = gpio_get_level(nFAULT);
	return fault_pin;
}

/*
 * Call this function to change the value of the DIR pin on the IC which controls the direction the motor turns.
 */
bool MOTOR_readFaultPin ( void )
{
	gpio_set_level(DIR, direction);
}

/*
 * Description
 */
void MOTOR_setDir ( bool direction )
{
	gpio_set_level(MOTOR_PIN_DIR, direction);
}

/*
 * this function will call MOTOR_stepMotorDir() to set the motor function to turn one MOTOR_PIN_STEP in the MOTOR_PIN_DIRection of the passed variable. If there is a falult with the motor nothing will happen.
 * One MOTOR_PIN_STEP will then be recorded in the MOTOR_PIN_stepsTurned variable to keep track of how much the motor has turned
 */
void MOTOR_step ( bool dir )
{
	MOTOR_setDir(direction);
	gpio_set_level(STEP, 1);
	vTaskDelay(1);
	gpio_set_level(STEP, 0);
	vTaskDelay(1);
	steps_turned += 1;
}

/*
 * Turn The Motor 'steps' Number of Steps In Defined Direction 
 */
void MOTOR_stepX ( bool dir, uint16_t steps )
{
	// Enable Motor Driver Functionality
	MOTOR_enable();
	// Turn Motor Defined Steps
	while ( steps > 0 )
	{
		MOTOR_step(dir);
		steps -= 1;
	}
	// Disable Motor For Power Saving
	MOTOR_disable();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */