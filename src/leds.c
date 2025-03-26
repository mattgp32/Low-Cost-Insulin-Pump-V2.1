/* This module written by Matt Payne as part of the Bluetooth insulin pump project.
   This module is designed to control the leds on the board
   Started on 10/3/2023
*/

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "motor.h"
#include "ble_comp_test.h"
#include "adc.h"
#include "leds.h"
#include "driver/ledc.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          GPIO_NUM_21
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (2500) // Frequency in Hertz. Set frequency at 5 kHz

#define LED1 GPIO_NUM_1
#define LED2 GPIO_NUM_2
#define LED3 GPIO_NUM_42

#define BATT_HIGH 2000
#define BATT_MED 1900
#define BATT_LOW 1800

#define LED_FLASH_TIME 100
#define LED_FLASH_TIME2 50


extern QueueHandle_t battLevelQueue;
extern bool BT_already_on;

void buzzer_init(void)
{
   // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void init_leds()
{
   gpio_reset_pin(LED3);
   // initialise all LEDs and set them to be turned off initially
   gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
   gpio_set_direction(LED3, GPIO_MODE_OUTPUT);
   gpio_set_level(LED1, true);
   gpio_set_level(LED2, true);
   gpio_set_level(LED3, true);
}

void led_on(int LED)
{
   gpio_set_level(LED, false);
}

void led_off(int LED)
{
   gpio_set_level(LED, true);
}

void display_batt_level(void* arg)
{
   for(;;)
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
            led_wave();
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

void led_five_flash()
{
   for(int i = 0; i < 5; i++)
   {
      led_on(LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      led_off(LED2);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

void led_double_flash()
{
   for(int i = 0; i < 5; i++)
   {
      led_on(LED2);
      led_on(LED1);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      led_off(LED2);
      led_off(LED1);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
   }
}

void led_wave()
{
   for (int i = 0; i <4;i++)
   {
      led_on(LED1);
      vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      led_on(LED2);
      led_off(LED1);
       vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      led_on(LED3);
      led_off(LED2);
       vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME));
      led_off(LED3);
   }

}

void annoying_buzzer(void* arg)
{
   for(;;)
   {
   ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
   vTaskDelay(pdMS_TO_TICKS(1000));
   }
}

void no_br_warning(void*arg)
{
   for(;;){
      puts("no br warning begin");
   int basal_rate = 0;
   nvs_handle_t br_handle;
   nvs_flash_init_partition("rate_storage");
   nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
   nvs_get_i32(br_handle, "basal_rate", &basal_rate);

   if(basal_rate == 0)
   {
      led_double_flash();
   }
   vTaskDelay(pdMS_TO_TICKS(120000));
   puts("no br warning end");
}
}

void BT_running_alert(void*args)
{
   for(;;)
   {
      puts("BT_running_alert - begin");
      if (BT_already_on == true)
      {
      led_on(2);
      vTaskDelay(pdMS_TO_TICKS(500));
      led_off(2);
      vTaskDelay(pdMS_TO_TICKS(500));
      } else {
         vTaskDelay(pdMS_TO_TICKS(2000));
      }
      puts("BT_running_alert - end");
   }
}

void pump_is_alive(void*args)
{
   for(;;)
   {
      puts("pump_is_alive - begin");
      if(BT_already_on == false){
        
            led_on(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
            led_off(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
            led_on(LED2);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
            led_off(LED2);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
             led_on(LED3);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
            led_off(LED3);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
             led_on(LED2);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
            led_off(LED2);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
             led_on(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
            led_off(LED1);
            vTaskDelay(pdMS_TO_TICKS(LED_FLASH_TIME2));
         

      } vTaskDelay(pdMS_TO_TICKS(60000));
      puts("pump_is_alive - end");
   }
}
