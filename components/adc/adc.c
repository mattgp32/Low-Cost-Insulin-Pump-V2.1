/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "adc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG                     "ADC"

#define ADC_GPIO_POTIN          GPIO_NUM_4
#define ADC_GPIO_POTOUT         GPIO_NUM_35
#define ADC_POT_CHANNEL         ADC_CHANNEL_3

#define ADC_POT_NUMREAD         10

#define ADC_BATT_CHANNEL        ADC_CHANNEL_7

#define ADC_BATT_TASK_LOOPDELAY 180000

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int pot_read_global;

QueueHandle_t battLevelQueue;
QueueHandle_t potReadQueue;

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
 * Initialise Everything For ADC Module Functionality 
 */
void ADC_init ( void )
{
    // LOG
    ESP_LOGI(TAG, "Initialising ADC Module");

    // INITIALISE POT ADC CHANNEL
    adc_oneshot_new_unit( &pot_read_adc_config, &pot_read_adc_handle );
    adc_oneshot_config_channel( pot_read_adc_handle, ADC_POT_CHANNEL, &config );
}

/*
 * Read Stringe Potentiometer Values
 */
void ADC_updatePot ( void )
{
    // LOG
    ESP_LOGI(TAG, "Update Potentiometer Value");

    // INITIALISE FUNCTION VARIABLES
    int adc_raw_value = 0;
    int potsumtot = 0;
    
    // SETUP GPIO PINS
    gpio_set_direction( ADC_GPIO_POTIN, GPIO_MODE_INPUT );
    gpio_pullup_dis( ADC_GPIO_POTIN );
    gpio_pulldown_dis( ADC_GPIO_POTIN );
    gpio_set_direction( ADC_GPIO_POTOUT, GPIO_MODE_OUTPUT );
    gpio_set_level( ADC_GPIO_POTOUT, true );

    // DELAY TO ALLOW VOLTAGE TO STABILISE
    vTaskDelay( pdMS_TO_TICKS(1000) );
    
    // COMPLETE AND AVERAGE 'ADC_POT_NUMREAD' READS OF ADC PINS
    for ( uint8_t i = 0; i < ADC_POT_NUMREAD; i++ )
    {
        potReadQueue = xQueueCreate( 3, sizeof(int) );
        adc_oneshot_read( pot_read_adc_handle, ADC_POT_CHANNEL, &adc_raw_value );
        // Close adc instance and free up memory/peripherals
        // ESP_ERROR_CHECK(adc_oneshot_del_unit(pot_read_adc_handle));
        int potRead = adc_raw_value;
        potsumtot += potRead;
    }
    pot_read_global = ( potsumtot / ADC_POT_NUMREAD );

    // DISPLAY POT VOLTAGE
    ESP_LOGI(TAG, "Pot Read = %d", pot_read_global);
    
    // RESET GPIO PIN TO SAVE POWER
    gpio_set_level( ADC_GPIO_POTOUT, false );
}

/*
 * Returns The Potentiometer Position 
 */
int ADC_getPotPosition ( void )
{
    ESP_LOGI(TAG, "Potentiometer Position Requested");
    return pot_read_global;
}

// /*
//  * THIS PRINT WAS JUST TO CHECK THE FUNCTION WORKS DURING DEVELOPMENT... AND IT DOES.
//  * IF YOU WANT TO PUT IT BACK IN, MAKE SURE YOU INCREASE THE RTOS STACK SIZE OR THE FUNCTION WILL CRASH
//  */
// void ADC_printBattLevel ( void )
// {
//     // INITIALISE FUNCTION VARIABLES
//     int* pBattLevel;
//     int battlevel;
//     pBattLevel = &battlevel;

//     // RECEIVE BATTERY LEVEL INFO FROM QUEUE AND PRINT
//     xQueueReceive( battLevelQueue, pBattLevel, 10 );
//     ESP_LOGI(TAG, "Current Battery Level = %d mV", battlevel);

//     // DELAY TO ALLOW PRINT
//     vTaskDelay( 10000/portTICK_PERIOD_MS );
// }

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * RTOS Task To Read Battery Voltage
 */
void task_ADC_getBattLevel ( void* arg )
{
    // LOG
    ESP_LOGI(TAG, "Starting Battery Level Task");

    // LOOP TO INFINITY AND BEYOND
    while (1)
    {
        // INITIALISE LOOP VARIABLES
        int adc_raw_value = 0;
        int *pBatt_level;

        // CREATE BATTERY LEVEL QUEUE
        battLevelQueue = xQueueCreate(3, sizeof(int));

        // INITIALISE BATTERY ADC CHANNEL
        adc_oneshot_unit_handle_t batt_read_adc_handle;
        adc_oneshot_unit_init_cfg_t batt_read_adc_config = { .unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE, };
        adc_oneshot_new_unit(&batt_read_adc_config, &batt_read_adc_handle);
        adc_oneshot_chan_cfg_t config = { .bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_11, };
        adc_oneshot_config_channel(batt_read_adc_handle, ADC_BATT_CHANNEL, &config);

        // READ BATTERY ADC CHANNEL
        adc_oneshot_read(batt_read_adc_handle, ADC_BATT_CHANNEL, &adc_raw_value);

        // CLOSE BATTERY ADC CHANNEL - FREE UP MEMORY/PERIPHERALS
        adc_oneshot_del_unit(batt_read_adc_handle);

        // PROCESS ADC READS TO BATTERY LEVEL
        int batt_level = ((adc_raw_value * 3100)/4095) + 300;
        pBatt_level = &batt_level;
        xQueueSend(battLevelQueue, pBatt_level, 0);
        
        // LOOP PACING
        vTaskDelay( pdMS_TO_TICKS(ADC_BATT_TASK_LOOPDELAY) );
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