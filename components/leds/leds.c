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
void LED_init(void)
{
   // initialise all LEDs and set them to be turned off initially
   gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED3, GPIO_MODE_OUTPUT);
   gpio_set_level(LED1, true);
   gpio_set_level(LED2, true);
   gpio_set_level(LED3, true);
}

/*
 * Function Description
 */
void led_on(int LED)
{
   gpio_set_level(LED, false);
}

/*
 * Function Description
 */
void led_off(int LED)
{
   gpio_set_level(LED, true);
}

/*
 * Function Description
 */
void display_batt_level(void *arg)
{
   for (;;)
   {
      int *pBattLevel;
      int battlevel;
      pBattLevel = &battlevel;

      if (uxQueueMessagesWaiting(battLevelQueue) > 0)
      {
         xQueueReceive(battLevelQueue, pBattLevel, 10);

         // printf("Batt level read as %d mV\n", battlevel);

         if (battlevel > BATT_HIGH)
         {
            led_on(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
            led_off(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));

            printf("Batt High\n");
         }
         else if (battlevel < BATT_HIGH && battlevel > BATT_MED)
         {

            for (int i = 0; i < 2; i++)
            {
               led_on(LED1);
               vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
               led_off(LED1);
               vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));

               printf("Batt Med\n");
            }
         }
         else
         {

            for (int i = 0; i < 3; i++)
            {
               led_on(LED1);
               printf("Batt Low\n");
            }
         }
         vTaskDelay(pdMS_TO_TICKS(9500));
         printf("Queue read\n");
      }
      vTaskDelay(pdMS_TO_TICKS(9500));
      printf("Queue not read\n");
   }
}

/*
 * Function Description
 */
void led_five_flash(void)
{
   for (int i = 0; i < 5; i++)
   {
      led_on(LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      led_off(LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

/*
 * Function Description
 */
void led_double_flash(void)
{
   for (int i = 0; i < 5; i++)
   {
      led_on(LED2);
      led_on(LED3);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      led_off(LED2);
      led_off(LED3);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

/*
 * Function Description
 */
void annoying_buzzer(void *arg)
{
   for (;;)
   {
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
      vTaskDelay(pdMS_TO_TICKS(1000));
   }
}

/*
 * Function Description
 */
void no_br_warning(void *arg)
{
   for (;;)
   {
      int32_t basal_rate = 0;
      nvs_handle_t br_handle;
      nvs_flash_init_partition("rate_storage");
      nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
      nvs_get_i32(br_handle, "basal_rate", &basal_rate);

      if (basal_rate == 0)
      {
         led_double_flash();
      }
      vTaskDelay(pdMS_TO_TICKS(10000));
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