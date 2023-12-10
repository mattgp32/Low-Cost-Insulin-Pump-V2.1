/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "motor.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG							"MOTOR"

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
 * Initialises Everything For Motor Module Functionality 
 */
void MOTOR_init ( void )
{
	// LOG
   	ESP_LOGI(TAG, "Initialising Motor Module");

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
 * Enable Motor Driver Functionality
 */
void MOTOR_enable ( void )
{
	// LOG
   	ESP_LOGI(TAG, "Enable Motor Driver");

	// ENABLE POWER SUPPLY AND MOTOR DRIVER
	gpio_set_level( MOTOR_POWER_GPIO_EN,   true );
    gpio_set_level( MOTOR_DRIVER_GPIO_EN, false );
	gpio_set_level( MOTOR_DRIVER_GPIO_nSLEEP, true );

	// DELAY TO ALLOW POWER SUPPLY TO STABILISE
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * Disble Motor Driver Functionality
 */
void MOTOR_disable ( void )
{
	// LOG
   	ESP_LOGI(TAG, "Disable Motor Driver");

	// DISABLE POWER SUPPLY AND MOTOR DRIVER
	gpio_set_level( MOTOR_POWER_GPIO_EN,   false );
    gpio_set_level( MOTOR_DRIVER_GPIO_EN, true );
	gpio_set_level( MOTOR_DRIVER_GPIO_nSLEEP, false );

	// DELAY TO ALLOW POWER SUPPLY TO CRASH
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * Read The Fult Pin On The Motor Driver
 *
 * Returns:	True - Fault Present
 * 			False - No Fault
 */
bool MOTOR_getFault ( void )
{
	bool retVal = !gpio_get_level( MOTOR_DRIVER_GPIO_nFAULT );
   	ESP_LOGI(TAG, "Motor Driver Fault Status Requested: %d", retVal);
	return retVal;
}

/*
 * Sets The Direction Of the Stepper Motor
 *
 * Direction Will Maintain Until Power Cycle 
 */
void MOTOR_setDir ( bool dir )
{
   	ESP_LOGI(TAG, "Motor Driver Direction Set: %d", dir);
	gpio_set_level( MOTOR_DRIVER_GPIO_DIR, dir );
}

/*
 * Step The Motor One Step In The Direction Set By MOTOR_setDir()
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
   	ESP_LOGI(TAG, "Move Motor %d Step", steps);
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