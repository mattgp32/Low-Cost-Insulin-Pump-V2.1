/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "leds.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define GPIO_LED1       GPIO_NUM_1
#define GPIO_LED2       GPIO_NUM_2
#define GPIO_LED3       GPIO_NUM_42

#define LED_HANDLER_DELAY 60000

#define BATT_HIGH       2000
#define BATT_MED        1900
#define BATT_LOW        1800

#define LED_FLASH_TIME  100
#define LED_FLASH_TIME2 50

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern QueueHandle_t battLevelQueue;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void LED_on                   ( int );
void LED_off                  ( int );

void LED_patternNoBasal       ( void );
void LED_patternVoltageLow    ( void );
void LED_patternBluetoothON   ( void );
void LED_patternNormal        ( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * INITIALISE APPROPRIATE INFO FOR LED MODULE FUNCTIONALITY 
 */
void LED_init ( void )
{
   gpio_reset_pin(GPIO_LED3); // ------------------------------------------ WHY ARE YOU RESETTING?

   gpio_set_direction(GPIO_LED1, GPIO_MODE_OUTPUT);
   gpio_set_direction(GPIO_LED2, GPIO_MODE_OUTPUT);
   gpio_set_direction(GPIO_LED3, GPIO_MODE_OUTPUT);

   LED_off(GPIO_LED1);
   LED_off(GPIO_LED2);
   LED_off(GPIO_LED3);

   LED_wave();
}

/*
 * PERFORM LED WAVE
 */
void LED_wave ( void )
{
   for ( int i = 0; i < 4; i++ )
   {
      LED_on( GPIO_LED1 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED2 );

      LED_off( GPIO_LED1 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED3 );

      LED_off( GPIO_LED2 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED3 );
   }

}

/*
 * FLASH LED2 FIVE TIMES
 */
void LED_flashFive_single ( void )
{
   LED_off( GPIO_LED1 );

   for( int i = 0; i < 5; i++ )
   {
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED1 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED1 );
   }
}

/*
 * FLASH LED1 AND LED2 FIVE TIMES
 */
void LED_flashFive_double ( void )
{
   LED_off( GPIO_LED1 );
   LED_off( GPIO_LED2 );

   for(int i = 0; i < 5; i++)
   {
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED1 );
      LED_on( GPIO_LED2 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED1 );
      LED_off( GPIO_LED2 );
   }
}

/*
 * FLASH LED1 AND LED2 FIVE TIMES
 */
void LED_flashFive_triple ( void )
{
   LED_off( GPIO_LED1 );
   LED_off( GPIO_LED2 );
   LED_off( GPIO_LED3 );

   for(int i = 0; i < 5; i++)
   {
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED1 );
      LED_on( GPIO_LED2 );
      LED_on( GPIO_LED3 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED1 );
      LED_off( GPIO_LED2 );
      LED_off( GPIO_LED3 );
   }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 *
 */
void task_LED_handler ( void *args )
{
   // INITIALISE FUNCTION VARIABLES
   int32_t basal_rate = 0;
   nvs_handle_t br_handle;     
   // int* pBattLevel;
   // int battlevel;
   // pBattLevel = &battlevel;

   // INITIALISE NVS PARTITION AND RETRIEVE HANDLE
   nvs_flash_init_partition("rate_storage");
   nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);

   while (1)
   {
      // CHECK AND RETRIEVE BATTERY VOLTAGE MEASUREMENT IN QUEUE
      if ( uxQueueMessagesWaiting(battLevelQueue) > 0 ) {
         xQueueReceive( battLevelQueue, pBattLevel, 10 );
      }

      // RETRIEVE CURRENT BASAL RATE FROM STORAGE
      nvs_get_i32(br_handle, "basal_rate", &basal_rate);

      // IS BASAL RATE LOW
      if( basal_rate == 0 ) {
         LED_patternNoBasal();
         vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      }
      // // IS BATTERY IS LOW
      // else if ( battlevel < BATT_MED ) {
      //    LED_patternVoltageLow();
      //    vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      // }
      // IS BLUETOOTH IS ON
      else if ( BT_isON() ) {
         LED_patternBluetoothON();
      }
      // NORMAL OPERATION
      else {
         LED_patternNormal();
         vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      }
   }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * TURN THE PROVIDED LED GPIO ON 
 */
void LED_on ( int LED )
{
   gpio_set_level(LED, false);
}

/*
 * TURN THE PROVIDED LED GPIO OFF
 */
void LED_off ( int LED )
{
   gpio_set_level(LED, true);
}

/*
 *
 */
void LED_patternNoBasal ( void )
{
   LED_flashFive_double();
}

/*
 *
 */
void LED_patternVoltageLow ( void )
{
   for ( int i = 0; i < 3; i++ ) {
      LED_wave();
   }
}

/*
 *
 */
void LED_patternBluetoothON ( void )
{
   LED_on(2);
   vTaskDelay(pdMS_TO_TICKS(500));
   LED_off(2);
   vTaskDelay(pdMS_TO_TICKS(500));
}

/*
 *
 */
void LED_patternNormal ( void )
{
   LED_on(GPIO_LED1);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_off(GPIO_LED1);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_on(GPIO_LED2);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_off(GPIO_LED2);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_on(GPIO_LED3);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_off(GPIO_LED3);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_on(GPIO_LED2);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_off(GPIO_LED2);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_on(GPIO_LED1);
   vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
   LED_off(GPIO_LED1);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */