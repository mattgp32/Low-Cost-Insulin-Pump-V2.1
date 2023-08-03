/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "leds.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO GPIO_NUM_21
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4095)                // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY (2500)           // Frequency in Hertz. Set frequency at 5 kHz

#define LED1 GPIO_NUM_35
#define LED2 GPIO_NUM_3
#define LED3 GPIO_NUM_9

#define BATT_HIGH 2000
#define BATT_MED 1900
#define BATT_LOW 1800

#define LED_FLASH_TIME 100

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void task_LED_handler            ( void * );
void task_LED_warningNoBasalRate ( void * );

void LED_on    ( int );
void LED_off   ( int );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern QueueHandle_t battLevelQueue;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Function Description
 */
void LED_init ( void )
{
   // initialise all LEDs and set them to be turned off initially
   gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED3, GPIO_MODE_OUTPUT);

   gpio_set_level(LED1, true);
   gpio_set_level(LED2, true);
   gpio_set_level(LED3, true);

   // Initialias RTOS Task
   xTaskCreate(task_LED_handler,             "LED_Handler_Task",              CONFIG_LED_TASK_HEAP*1024,                         NULL, CONFIG_LED_TASK_PRIORITY, NULL);
   xTaskCreate(task_LED_warningNoBasalRate,  "LED_No_BasalRate_Warning_Task", CONFIG_LED_TASK_HEAP_NO_BASAL_RATE_WARNING*1024,   NULL, CONFIG_LED_TASK_NO_BASAL_RATE_WARNING, NULL);
}

/*
 * Function Description
 */
void LED_fiveFlash ( void )
{
   for (int i = 0; i < 5; i++)
   {
      LED_on(LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      LED_off(LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

/*
 * Function Description
 */
void LED_doubleFlash ( void )
{
   for (int i = 0; i < 5; i++)
   {
      LED_on(LED2);
      LED_on(LED3);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      LED_off(LED2);
      LED_off(LED3);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Blink LED depending on batt level
 */
void task_LED_handler ( void *arg )
{
   // Create Function Variables
   TickType_t xLastWakeTime = xTaskGetTickCount();

   // Loop To Infinity And Beyond
   while (1)
   {
      // Initialise Loop Variables
      int *pBattLevel;
      int battlevel;
      pBattLevel = &battlevel;

      // Is There An Entry In The Battery Queue
      if (uxQueueMessagesWaiting(battLevelQueue) > 0)
      {
         // Retrieve Item In Queue
         xQueueReceive(battLevelQueue, pBattLevel, 10);

         // printf("Batt level read as %d mV\n", battlevel);

         // Battery Level - HIGH
         if (battlevel > BATT_HIGH) {
            // Log Battery Level
            printf("Batt High\n");
            // Flash LED 
            LED_on(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
            LED_off(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
         }  
         // Battery Level - MEDIUM
         else if (battlevel < BATT_HIGH && battlevel > BATT_MED) {
            // Log Battery Level
            printf("Batt Med\n");
            // Flash LED Twice
            for (int i = 0; i < 2; i++) {
               LED_on(LED1);
               vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
               LED_off(LED1);
               vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
            }
         // Battery Level - LOW
         } else {
            // Log Battery Level
            printf("Batt Low\n");
            // Flash LED Thrice
            for (int i = 0; i < 3; i++) {
               LED_on(LED1);
            }
         }
         vTaskDelay(pdMS_TO_TICKS(9500));
         printf("Queue read\n");
      }

      // Loop Pacing
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONFIG_LED_TASK_INTERVAL));

      //
      printf("Queue not read\n");
   }
}

/*
 * flash led if br = 0
 */
void task_LED_warningNoBasalRate ( void *arg )
{
   // Create Function Variables
   TickType_t xLastWakeTime = xTaskGetTickCount();

   // Loop To Infinity And Beyond
   while (1)
   {
      // Initialise Loop Variables
      int32_t basal_rate = 0;
      nvs_handle_t br_handle;

      // 
      nvs_flash_init_partition( "rate_storage" );
      nvs_open_from_partition( "rate_storage", "basal_rate", NVS_READONLY, &br_handle );
      nvs_get_i32( br_handle, "basal_rate", &basal_rate );

      // 
      if ( basal_rate == 0 ) {
         LED_doubleFlash();
      }

      // Loop Pacing
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONFIG_LED_TASK_INTERVAL_NO_BASAL_RATE_WARNING));
   }
}

/*
 * Function Description
 */
void LED_on ( int led )
{
   gpio_set_level(led, false);
}

/*
 * Function Description
 */
void LED_off ( int led )
{
   gpio_set_level(led, true);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */