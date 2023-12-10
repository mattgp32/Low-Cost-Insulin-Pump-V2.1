/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "motor.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MOTOR_DRIVER_GPIO_DIR 		GPIO_NUM_11
#define MOTOR_DRIVER_GPIO_STEP 		GPIO_NUM_12
#define MOTOR_DRIVER_GPIO_EN		GPIO_NUM_13
#define MOTOR_DRIVER_GPIO_nSLEEP	GPIO_NUM_10
#define MOTOR_DRIVER_GPIO_nFAULT	GPIO_NUM_14

#define MOTOR_POWER_GPIO_EN 		GPIO_NUM_45

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
 * INITIALISE APPROPRIATE GPIO FOR MOTOR MODULE FUNCTIONALITY 
 */
void MOTOR_init ( void )
{
	// INITIALISE POWER SUPPLY AND MOTOR DRIVER GPIO 
    gpio_set_direction( MOTOR_POWER_GPIO_EN, GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_DRIVER_GPIO_nFAULT, GPIO_MODE_INPUT );
    gpio_set_direction( MOTOR_DRIVER_GPIO_DIR, GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_DRIVER_GPIO_STEP, GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_DRIVER_GPIO_EN, GPIO_MODE_OUTPUT );
	gpio_set_direction( MOTOR_DRIVER_GPIO_nSLEEP, GPIO_MODE_OUTPUT );

    gpio_set_level( MOTOR_DRIVER_GPIO_EN, true );
}

/*
 * ENABLE MOTOR DRIVER FUNCTIONALITY
 */
void MOTOR_enable ( void )
{
	// ENABLE POWER SUPPLY AND MOTOR DRIVER
	gpio_set_level( MOTOR_POWER_GPIO_EN,   true );
    gpio_set_level( MOTOR_DRIVER_GPIO_EN, false );
	gpio_set_level( MOTOR_DRIVER_GPIO_nSLEEP, true );

	// DELAY TO ALLOW POWER SUPPLY TO STABILISE
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * DISABLE MOTOR DRIVER FUNCTIONALITY
 */
void MOTOR_disable ( void )
{
	// DISABLE POWER SUPPLY AND MOTOR DRIVER
	gpio_set_level( MOTOR_POWER_GPIO_EN,   false );
    gpio_set_level( MOTOR_DRIVER_GPIO_EN, true );
	gpio_set_level( MOTOR_DRIVER_GPIO_nSLEEP, false );

	// DELAY TO ALLOW POWER SUPPLY TO CRASH
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * READ THE FAULT PIN ON THE MOTOR DRIVER
 * Returns:	True - Fault Present
 * 			False - No Fault
 */
bool MOTOR_getFault ( void )
{
	return !gpio_get_level( MOTOR_DRIVER_GPIO_nFAULT );
}

/*
 * SETS THE DIRECTION OF THE STEPPER MOTOR
 * DIRECTION WILL MAINTAIN UNTIL POWER CYCLE 
 */
void MOTOR_setDir ( bool dir )
{
	gpio_set_level( MOTOR_DRIVER_GPIO_DIR, dir );
}

/*
 * STEP THE MOTOR ONE STEP IN THE DIRECTION SET BY MOTOR_setDir()
 */
void MOTOR_step ( void )
{
	// PULSE STEP PIN ON MOTOR DRIVER IC
	gpio_set_level(MOTOR_DRIVER_GPIO_STEP, 1);
	vTaskDelay(1);
	gpio_set_level(MOTOR_DRIVER_GPIO_STEP, 0);
	vTaskDelay(1);

	// INCREMENT COUNTER
	steps_turned += 1;
}

/*
 * Turn The Motor 'steps' Number of Steps In Defined Direction 
 */
void MOTOR_stepX ( bool dir, uint16_t steps )
{
	// ENABLE MOTOR DRIVER FUNCTION
	MOTOR_enable();
	// UPDATE MOTOR TURN DIRECTION
	MOTOR_setDir( dir );

	// STEP THE MOTOR DEFINED STEPS
	while ( steps > 0 ) {
		MOTOR_step();
		steps -= 1;
	}

	// DISABLE MOTOR FOR POWER SAVING
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