/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "adc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ADC_BATT_LEVEL  GPIO_NUM_8

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

QueueHandle_t battLevelQueue = NULL;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void ADC_init ( void )
{
    // Launch the RTOS Task
    xTaskCreate(task_ADC_getBattLevel, "Battery_Handler_Task", CONFIG_BATTERY_TASK_HEAP*1024, NULL, CONFIG_BATTERY_TASK_PRIORITY, NULL);
}

/*
 * Description
 */
void task_ADC_getBattLevel ( void *arg )
{
    // Create Function Variables
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Loop To Infinity And Beyond
    while(1)
    {
        int adc_raw_value = 0;
        int *pBatt_level;

        battLevelQueue = xQueueCreate( 3, sizeof(int) );

        if ( battLevelQueue == 0 ) {
            //printf("Uh oh, the battery level queue craetion failed!");
        }

        // Initialise ADC1 Channel 7
        adc_oneshot_unit_handle_t batt_read_adc_handle;
        
        adc_oneshot_unit_init_cfg_t batt_read_adc_config = { .unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE,};

        ESP_ERROR_CHECK(adc_oneshot_new_unit(&batt_read_adc_config, &batt_read_adc_handle));

        adc_oneshot_chan_cfg_t config = {.bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_11,};

        adc_oneshot_config_channel(batt_read_adc_handle, ADC_CHANNEL_7, &config);

        adc_oneshot_read(batt_read_adc_handle, ADC_CHANNEL_7, &adc_raw_value);

        // Close adc instance and free up memory/peripherals
        ESP_ERROR_CHECK(adc_oneshot_del_unit(batt_read_adc_handle));

        int batt_level = ((adc_raw_value * 3100)/4095) + 300;
        pBatt_level = &batt_level;

        xQueueSend(battLevelQueue, pBatt_level, 0);

        // Loop Pacing
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONFIG_BATTERY_TASK_INTERVAL));
    }

}

/*
 * Description
 */
void ADC_printBattLevel ( void )
{
    int* pBattLevel;
    int battlevel;
    pBattLevel = &battlevel;

    xQueueReceive(battLevelQueue, pBattLevel, 10);

    // printf("Current battery level is %d mV", battlevel); 
    // This print was just to check the function works during development and it does. Hence I have commentented it out. 
    // If you want to put it back in make sure you increase the freertos stack size of this function or it will crash because printf uses loads of memory

    vTaskDelay(10000/portTICK_PERIOD_MS);
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
