/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "adc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

QueueHandle_t battLevelQueue;
QueueHandle_t potReadQueue;
int pot_read_global;
adc_oneshot_unit_handle_t pot_read_adc_handle;
    
adc_oneshot_unit_init_cfg_t pot_read_adc_config = { 
    .unit_id = ADC_UNIT_1,      
    .ulp_mode = ADC_ULP_MODE_DISABLE, };

adc_oneshot_chan_cfg_t config = {   
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    .atten = ADC_ATTEN_DB_11, };

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void ADC_init ( void )
{
    // Initialise ADC1 Channel 3
    adc_oneshot_new_unit(&pot_read_adc_config, &pot_read_adc_handle);
    adc_oneshot_config_channel(pot_read_adc_handle, ADC_CHANNEL_3, &config);
}

/*
 * Description
 */
void ADC_readpot ( void )
{
    int adc_raw_value = 0;
    int i = 0;
    int potsumtot = 0;
    
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);
    gpio_pullup_dis(GPIO_NUM_4);
    gpio_pulldown_dis(GPIO_NUM_4);
    gpio_set_direction(GPIO_NUM_35, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_35, true);
    vTaskDelay(pdMS_TO_TICKS(1000));
   
    while( i < 10 )
    {
        potReadQueue = xQueueCreate(3, sizeof(int));
        adc_oneshot_read(pot_read_adc_handle, ADC_CHANNEL_3, &adc_raw_value);
        // Close adc instance and free up memory/peripherals
        // ESP_ERROR_CHECK(adc_oneshot_del_unit(pot_read_adc_handle));
        int potRead = adc_raw_value;
        
        i++;
        potsumtot += potRead;
    }

    pot_read_global = potsumtot/10;
    printf("Pot read = %d\n", pot_read_global);
    gpio_set_level(GPIO_NUM_37, false);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void task_ADC_getBattLevel ( void* arg )
{
    while(1)
    {
        int adc_raw_value = 0;
        int *pBatt_level;

        battLevelQueue = xQueueCreate(3, sizeof(int));

        // Initialise ADC1 Channel 7
        adc_oneshot_unit_handle_t batt_read_adc_handle;
        
        adc_oneshot_unit_init_cfg_t batt_read_adc_config = { 
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE,};
        adc_oneshot_new_unit(&batt_read_adc_config, &batt_read_adc_handle);

        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_11,};
        adc_oneshot_config_channel(batt_read_adc_handle, ADC_CHANNEL_7, &config);

        adc_oneshot_read(batt_read_adc_handle, ADC_CHANNEL_7, &adc_raw_value);

        // Close adc instance and free up memory/peripherals
        adc_oneshot_del_unit(batt_read_adc_handle);

        int batt_level = ((adc_raw_value * 3100)/4095) + 300;
        pBatt_level = &batt_level;

        xQueueSend(battLevelQueue, pBatt_level, 0);
        vTaskDelay(pdMS_TO_TICKS(180000));
    }

}

/*
 * Description
 */
void task_ADC_printBattLevel ( void* arg )
{
    int* pBattLevel;
    int battlevel;
    pBattLevel = &battlevel;

    xQueueReceive(battLevelQueue, pBattLevel, 10);

    printf("Current battery level is %d mV", battlevel); 
    // This print was just to check the function works during development and it does. Hence I have commentented it out. 
    // If you want to put it back in make sure you increase the freertos stack size of this function or it will crash 

    vTaskDelay(10000/portTICK_PERIOD_MS);
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