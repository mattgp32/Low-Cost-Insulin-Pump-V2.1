/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "motor.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MOTOR_PIN_nFAULT 	GPIO_NUM_14
#define MOTOR_PIN_DIR 		GPIO_NUM_11
#define MOTOR_PIN_STEP 		GPIO_NUM_12
#define MOTOR_PIN_BOOST 	GPIO_NUM_45
#define MOTOR_PIN_enDRIVER	GPIO_NUM_13
#define MOTOR_PIN_nSLEEP 	GPIO_NUM_10

#define MOTOR_FWD 			0
#define MOTOR_RVS 			1

#define MOTOR_DELAY_TIME 	500

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uint32_t stepsTurned = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Initialise all the pins required to allow the motor to be driven
 */
void MOTOR_init ( void )
{
	// 
    gpio_set_direction( MOTOR_PIN_nFAULT, 	GPIO_MODE_INPUT );
    gpio_set_direction( MOTOR_PIN_DIR, 		GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_PIN_STEP, 	GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_PIN_BOOST, 	GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_PIN_enDRIVER,	GPIO_MODE_OUTPUT );
	gpio_set_direction( MOTOR_PIN_nSLEEP, 	GPIO_MODE_OUTPUT );
    //
	gpio_set_level(     MOTOR_PIN_enDRIVER,	true );
}

/*
 * Enable Motor Driver Functionality
 */
void MOTOR_enable ( void )
{
	gpio_set_level( MOTOR_PIN_BOOST, 	true );
    gpio_set_level( MOTOR_PIN_enDRIVER, false );
	gpio_set_level( MOTOR_PIN_nSLEEP, 	true );
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * Disable Motor Driver Functionality
 */
void MOTOR_disable ( void )
{
	gpio_set_level( MOTOR_PIN_BOOST, 	false );
    gpio_set_level( MOTOR_PIN_enDRIVER, true );
	gpio_set_level( MOTOR_PIN_nSLEEP, 	false );
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * Retrieve Status of Motor Driver Fault Pin
 */
bool MOTOR_readFaultPin ( void )
{
	return gpio_get_level( MOTOR_PIN_nFAULT );
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
	// Set The Motor Driver Direction
	gpio_set_level( MOTOR_PIN_DIR,	dir );
	// Step Motor
	gpio_set_level( MOTOR_PIN_STEP,	1 );
	vTaskDelay(1);
	gpio_set_level( MOTOR_PIN_STEP,	0 );
	vTaskDelay(1);
	// Increment Step Counter
	stepsTurned += 1;
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