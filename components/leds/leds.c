/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "leds.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define LED1 GPIO_NUM_1
#define LED2 GPIO_NUM_2
#define LED3 GPIO_NUM_42

#define BATT_HIGH 2000
#define BATT_MED 1900
#define BATT_LOW 1800

#define LED_FLASH_TIME 100
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
 * Description
 */
void LED_init ( void )
{
   gpio_reset_pin(LED3);
   // initialise all LEDs and set them to be turned off initially
   gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED3, GPIO_MODE_OUTPUT);
   gpio_set_level(LED1, false);
   gpio_set_level(LED2, true);
   gpio_set_level(LED3, true);
}

/*
 * Description
 */
void LED_on ( int LED )
{
   gpio_set_level(LED, false);
}

/*
 * Description
 */
void LED_off( int LED )
{
   gpio_set_level(LED, true);
}

/*
 * Description
 */
void LED_flashFive ( void )
{
   for(int i = 0; i < 5; i++)
   {
      LED_on(GPIO_LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      LED_off(GPIO_LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

/*
 * Description
 */
void LED_flashDouble ( void )
{
   for(int i = 0; i < 5; i++)
   {
      LED_on(LED2);
      LED_on(LED1);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      LED_off(LED2);
      LED_off(LED1);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

/*
 * Description
 */
void LED_wave ( void )
{
   for (int i = 0; i <4;i++)
   {
      LED_on(LED1);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      LED_on(LED2);
      LED_off(LED1);
       vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      LED_on(LED3);
      LED_off(LED2);
       vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      LED_off(LED3);
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
   while(1)
   {
      int* pBattLevel;
      int battlevel;
      pBattLevel = &battlevel;

      if (uxQueueMessagesWaiting(battLevelQueue)> 0) 
      {
         xQueueReceive(battLevelQueue, pBattLevel, 10);

         //printf("Batt level read as %d mV\n", battlevel);

         if (battlevel < BATT_MED)
         {
                  for(int i = 0; i < 3; i++)
            {
               LED_wave();
               printf("Batt Low\n");
            }

               // printf("Batt High\n");

         }
         vTaskDelay(pdMS_TO_TICKS(60000));
         // printf("Queue read\n");
      }
      vTaskDelay(pdMS_TO_TICKS(60000));
      // printf("Queue not read\n");
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
         LED_flashDouble();
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
         LED_on(LED1);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_off(LED1);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_on(LED2);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_off(LED2);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_on(LED3);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_off(LED3);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_on(LED2);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_off(LED2);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_on(LED1);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         LED_off(LED1);
         vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
      } 
      vTaskDelay(pdMS_TO_TICKS(60000));
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