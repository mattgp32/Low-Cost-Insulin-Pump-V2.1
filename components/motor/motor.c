/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "motor.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG								"MOTOR"

#define MOTOR_DRIVER_GPIO_DIR 			GPIO_NUM_11
#define MOTOR_DRIVER_GPIO_STEP 			GPIO_NUM_12
#define MOTOR_DRIVER_GPIO_nEN			GPIO_NUM_13
#define MOTOR_DRIVER_GPIO_nSLEEP		GPIO_NUM_10
#define MOTOR_DRIVER_GPIO_nFAULT		GPIO_NUM_14
#define MOTOR_POWER_GPIO_EN 			GPIO_NUM_45

#define MOTOR_DRIVER_DEFAULT_DIR		MOTOR_FWD
#define MOTOR_DRIVER_DEFAULT_STEP		false	
#define MOTOR_DRIVER_DEFAULT_nEN		true
#define MOTOR_DRIVER_DEFAULT_nSLEEP 	false
#define MOTOR_POWER_DEFAULT_EN			false

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void MOTOR_enable 	( void );
void MOTOR_disable 	( void );
void MOTOR_setDir   ( bool );
void MOTOR_step 	( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uint64_t steps_turned = 0;

bool availableForControl = true;

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

	// INITIALISE POWER SUPPLY AND MOTOR DRIVER GPIO - INPUT
    gpio_set_direction( MOTOR_DRIVER_GPIO_nFAULT, GPIO_MODE_INPUT );

	// INITIALISE POWER SUPPLY AND MOTOR DRIVER GPIO - OUTPUT
    gpio_set_direction( MOTOR_POWER_GPIO_EN, 		GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_DRIVER_GPIO_DIR, 		GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_DRIVER_GPIO_STEP, 	GPIO_MODE_OUTPUT );
    gpio_set_direction( MOTOR_DRIVER_GPIO_nEN, 		GPIO_MODE_OUTPUT );
	gpio_set_direction( MOTOR_DRIVER_GPIO_nSLEEP,	GPIO_MODE_OUTPUT );
	gpio_set_level( MOTOR_POWER_GPIO_EN,   		MOTOR_POWER_DEFAULT_EN );
	gpio_set_level( MOTOR_DRIVER_GPIO_DIR, 		MOTOR_DRIVER_DEFAULT_DIR );
	gpio_set_level( MOTOR_DRIVER_GPIO_STEP, 	MOTOR_DRIVER_DEFAULT_STEP );
	gpio_set_level( MOTOR_DRIVER_GPIO_nEN, 		MOTOR_DRIVER_DEFAULT_nEN );
	gpio_set_level( MOTOR_DRIVER_GPIO_nSLEEP, 	MOTOR_DRIVER_DEFAULT_nSLEEP );
}

/*
 * Read The Fult Pin On The Motor Driver
 *
 * Returns:	True - Fault Present
 * 			False - No Fault
 */
bool MOTOR_getFault ( void )
{
	// RETRIEVE FAULT PIN VALUE
	bool retVal = !gpio_get_level( MOTOR_DRIVER_GPIO_nFAULT );

	// LOG FAULT VALUE
   	ESP_LOGI(TAG, "Motor Driver Fault Status Requested: %d", retVal);
	
	// RETURN
	return retVal;
}

/*
 * Turn The Motor 'steps' Number of Steps In Defined Direction 
 */
bool MOTOR_stepX ( bool dir, uint32_t steps )
{
	//
	bool retVal = false;
	// LOW BATTERY DETECTED
	if ( ADC_battCritical() ) {
		ESP_LOGI(TAG, "ERROR: BATTERY CRITICALLY LOW. Refusing to drive motor");
	}
	// POT AT MAX POSITION
	if ( ADC_potAtMax() && dir == MOTOR_FWD ) {
		ESP_LOGI(TAG, "ERROR: POTENTIOMETER AT MAXIMUM POSITION. Refusing to drive motor");
	}
	// POT AT MINIMUM POSITION (Reset)
	if ( ADC_potReset() && dir == MOTOR_RVS ) {
		ESP_LOGI(TAG, "ERROR: POTENTIOMETER AT MINIMUM POSITION. Refusing to drive motor");
	}
	// NORMAL OPERATION
	else {
		// LOG 
		if ( dir == MOTOR_FWD ) {
			ESP_LOGI(TAG, "Command: Drive Motor %ld Steps FWD", steps);
		} else {
			ESP_LOGI(TAG, "Command: Drive Motor %ld Steps RVS", steps);
		}

		// ENABLE MOTOR DRIVER FUNCTION
		MOTOR_enable();
		// UPDATE MOTOR TURN DIRECTION
		MOTOR_setDir( dir );

		// STEP THE MOTOR DEFINED STEPS
		ESP_LOGI(TAG, "Drive Plunger Motor");
		while ( steps > 0 ) 
		{
			// CHECK IF ENDSTOPS REACHED
			if ( dir == MOTOR_FWD && ADC_potAtMax() ) {
				ESP_LOGI(TAG, "Reached Pot Maximum Position, Aborting Remaining Steps (%ld)", steps);
				break;
			} else if ( dir == MOTOR_RVS && ADC_potReset() ) {
				ESP_LOGI(TAG, "Reached Pot Reset Position, Aborting Remaining Steps (%ld)", steps);	
				break;
			}
			// TAKE A STEP AND DECREMENT COUNTER
			MOTOR_step();
			steps -= 1;
		}
		// Did The Full Delivery Of Steps Complete
		if ( steps == 0 ) {
			retVal = true;
		}
		// DISABLE MOTOR FOR POWER SAVING
		MOTOR_disable();
	}
	// RETURN
	return retVal;
}

/*
 *
 */
bool MOTOR_avalibleForControl ( void )
{
	return availableForControl;
}

/*
 *
 */
void MOTOR_takeControl ( void )
{
	availableForControl = false;
}

/*
 *
 */
void MOTOR_releaseControl ( void )
{
	availableForControl = true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Enable Motor Driver Functionality
 */
void MOTOR_enable ( void )
{
	// LOG
   	ESP_LOGI(TAG, "Enable Motor Driver Circuit");

	// ENABLE POWER SUPPLY AND MOTOR DRIVER
	gpio_set_level( MOTOR_POWER_GPIO_EN,   		true );
    gpio_set_level( MOTOR_DRIVER_GPIO_nEN, 		false );
	gpio_set_level( MOTOR_DRIVER_GPIO_nSLEEP, 	true );

	// DELAY TO ALLOW POWER SUPPLY TO STABILISE
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * Disble Motor Driver Functionality
 */
void MOTOR_disable ( void )
{
	// LOG
   	ESP_LOGI(TAG, "Disable Motor Driver Circuit");

	// DISABLE POWER SUPPLY AND MOTOR DRIVER
	gpio_set_level( MOTOR_POWER_GPIO_EN,   		false );
    gpio_set_level( MOTOR_DRIVER_GPIO_nEN, 		true );
	gpio_set_level( MOTOR_DRIVER_GPIO_nSLEEP,	false );

	// DELAY TO ALLOW POWER SUPPLY TO CRASH
	vTaskDelay( 200/portTICK_PERIOD_MS );
}

/*
 * Sets The Direction Of the Stepper Motor
 *
 * Direction Will Maintain Until Power Cycle 
 */
void MOTOR_setDir ( bool dir )
{
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */