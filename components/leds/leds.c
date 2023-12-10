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
extern bool BT_already_on;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
}

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
 * FLASH LED2 FIVE TIMES
 */
void LED_flashFive ( void )
{
   for( int i = 0; i < 5; i++ )
   {
      LED_on( GPIO_LED2 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED2 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
   }
}

/*
 * FLASH LED1 AND LED2 FIVE TIMES
 */
void LED_flashFive_double ( void )
{
   for(int i = 0; i < 5; i++)
   {
      LED_on( GPIO_LED2 );
      LED_on( GPIO_LED1 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED2 );
      LED_off( GPIO_LED1 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
   }
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void task_LED_displayBattLevel ( void *arg )
{
   // LOOP TO INFINITY AND BEYOND
   while (1)
   {
      // INITIALISE LOOP VARIABLES
      int* pBattLevel;
      int battlevel;
      pBattLevel = &battlevel;

      // CHECK FOR BATTERY VOLTAGE MEASUREMENT IN QUEUE
      if ( uxQueueMessagesWaiting(battLevelQueue) > 0 ) 
      {
         // READ IN BATTERY VOLTAGE
         xQueueReceive( battLevelQueue, pBattLevel, 10 );

         // CHECK IF VOLTAGE IS BELOW THRESHOLD
         if ( battlevel < BATT_MED ) {
            // DO AN LED WAVE TO LET USER KNOW
            for ( int i = 0; i < 3; i++ )
            {
               LED_wave();
               printf("Batt Low\n");
            }
         }
         // DELAY?
         vTaskDelay( pdMS_TO_TICKS(LED_HANDLER_DELAY) );                   // ````````````````````````````````````````````````````` WHY DELAY?
      }

      // LOOP PACING
      vTaskDelay( pdMS_TO_TICKS(LED_HANDLER_DELAY) );
   }
}

/*
 * Description
 */
void task_LED_noBasilWarning ( void *arg )
{
   while(1)
   {
      puts("no br warning begin");
      int32_t basal_rate = 0;
      nvs_handle_t br_handle;
      nvs_flash_init_partition("rate_storage");
      nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
      nvs_get_i32(br_handle, "basal_rate", &basal_rate);

      if(basal_rate == 0)
      {
         LED_flashFive_double();
      }
      vTaskDelay(pdMS_TO_TICKS(120000));
      puts("no br warning end");
   }
}

/*
 * Description
 */
void task_LED_bluetoothRunningAlert ( void *args )
{
   while(1)
   {
      puts("BT_running_alert - begin");
      if (BT_already_on == true)
      {
         LED_on(2);
         vTaskDelay(pdMS_TO_TICKS(500));
         LED_off(2);
         vTaskDelay(pdMS_TO_TICKS(500));
      } else {
         vTaskDelay(pdMS_TO_TICKS(2000));
      }
      puts("BT_running_alert - end");
   }
}

/*
 * Description
 */
void task_LED_pumpIsAlive ( void *args )
{
   while(1)
   {
      puts("pump_is_alive - begin");
      if(BT_already_on == false)
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
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
      } 
      vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      puts("pump_is_alive - end");
   }
}

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