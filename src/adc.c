/* This module written by Matt Payne as part of the Bluetooth insulin pump project.
   This module is designed to control the adc to take a reading of the battery level and determine if the device has a low battery.
   Started on 9/3/2023
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_adc/adc_oneshot.h"
#include "adc.h"

#define ADC_BATT_LEVEL GPIO_NUM_8

QueueHandle_t battLevelQueue;

//setup and initialise an adc instance

void get_batt_level(void* arg)
{
    for(;;)
    {
    int adc_raw_value = 0;
    int *pBatt_level;

    battLevelQueue = xQueueCreate(3, sizeof(int));

    if (battLevelQueue==0)
    {
        //printf("Uh oh, the battery level queue craetion failed!");
    }

    // Initialise ADC1 Channel 7
    adc_oneshot_unit_handle_t batt_read_adc_handle;
    
    adc_oneshot_unit_init_cfg_t batt_read_adc_config = { .unit_id = ADC_UNIT_1,
                                                         .ulp_mode = ADC_ULP_MODE_DISABLE,};

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&batt_read_adc_config, &batt_read_adc_handle));

    adc_oneshot_chan_cfg_t config = {.bitwidth = ADC_BITWIDTH_DEFAULT,
                                     .atten = ADC_ATTEN_DB_11,};

    adc_oneshot_config_channel(batt_read_adc_handle, ADC_CHANNEL_7, &config);

    adc_oneshot_read(batt_read_adc_handle, ADC_CHANNEL_7, &adc_raw_value);

    // Close adc instance and free up memory/peripherals
    ESP_ERROR_CHECK(adc_oneshot_del_unit(batt_read_adc_handle));

    int batt_level = ((adc_raw_value * 3100)/4095) + 300;
    pBatt_level = &batt_level;

    xQueueSend(battLevelQueue, pBatt_level, 0);
    vTaskDelay(pdMS_TO_TICKS(10000));
    }

}

void print_batt_level(void* arg)
{
    int* pBattLevel;
    int battlevel;
    pBattLevel = &battlevel;

    xQueueReceive(battLevelQueue, pBattLevel, 10);

    //printf("Current battery level is %d mV", battlevel); // This print was just to check the function works during development and it does. Hence I have commentented it out. 
    // If you want to put it back in make sure you increase the freertos stack size of this function or it will crash because printf uses loads of memory

    vTaskDelay(10000/portTICK_PERIOD_MS);
}
