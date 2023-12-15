/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "adc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG                     "ADC"

#define ADC_GPIO_POTOUT         GPIO_NUM_35
#define ADC_GPIO_POTIN          GPIO_NUM_4
#define ADC_POT_CHANNEL         ADC_CHANNEL_3

#define ADC_BATT_GPIO           GPIO_NUM_8
#define ADC_BATT_CHANNEL        ADC_CHANNEL_7

#define ADC_BATT_UPDATE_MS      100
#define ADC_POT_UPDATE_MS       10

#define ADC_POT_NUMREAD         10
#define ADC_BATT_NUMREAD        3

#define ADC_BATT_LOW            3600
#define ADC_BATT_CRITICAL       3300

#define ADC_POT_RESET           0
#define ADC_POT_MAX             4095

#define ADC_BATT_TASK_LOOPDELAY 180000

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void ADC_calcBattVoltage ( void );
static void ADC_calcPotPosition ( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint32_t battVoltage = 0;
static uint32_t potPosition = 0;

adc_oneshot_unit_handle_t adc_handle;
adc_oneshot_unit_init_cfg_t adc_config = { 
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

    // SETUP GPIO PINS
    gpio_set_direction( ADC_GPIO_POTIN, GPIO_MODE_INPUT );
    gpio_pullup_dis( ADC_GPIO_POTIN );
    gpio_pulldown_dis( ADC_GPIO_POTIN );
    gpio_set_direction( ADC_GPIO_POTOUT, GPIO_MODE_OUTPUT );
    gpio_set_level( ADC_GPIO_POTOUT, true );

    // INITIALISE ADC
    adc_oneshot_new_unit( &adc_config, &adc_handle );
    adc_oneshot_config_channel( adc_handle, ADC_POT_CHANNEL, &config );
    adc_oneshot_config_channel( adc_handle, ADC_BATT_CHANNEL, &config );

    // TAKE BATTERY + POTENTIOMETER MEASUREMENTS
    ADC_calcBattVoltage();
    ADC_calcPotPosition();
}

/*
 * Returns the Battery Voltage
 */
uint32_t ADC_getBattVoltage ( void )
{
    ADC_calcBattVoltage();
    return battVoltage;
}

/*
 * Returns True If The Battery Voltage Is Low
 */
bool ADC_battLow ( void )
{
    ADC_calcBattVoltage();
    return ( battVoltage <= ADC_BATT_LOW );
}

/*
 * Returns True If The Battery Voltage Is Critically Low
 */
bool ADC_battCritical ( void )
{
    ADC_calcBattVoltage();
    return ( battVoltage <= ADC_BATT_CRITICAL );
}

/*
 * Returns The Potentiometer Position 
 */
uint32_t ADC_getPotPosition ( void )
{
    ADC_calcPotPosition();
    return potPosition;
}

/*
 * Returns True If The Potentiometer Is In The Reset Position (0)
 */
bool ADC_potReset ( void )
{
    ADC_calcPotPosition();
    return ( potPosition <= ADC_POT_RESET );
}

/*
 * Returns True If Potentiometer Is At The Max Position ()
 */
bool ADC_potAtMax ( void )
{
    ADC_calcPotPosition();
    return ( potPosition >= ADC_POT_MAX );
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Calculates The Battery Voltage
 */
static void ADC_calcBattVoltage ( void )
{
    // INITIALISE LOOP VARIABLES
    int adc_raw = 0;
    uint32_t adc_counts = 0;
    TickType_t now = xTaskGetTickCount();
    static TickType_t tick = 0; 

    // DONT BOTHER UPDATING IF ITS ALREADY BEEN CHECKED IN LAST 'ADC_BATT_UPDATE_MS' 
    if ( ((now - tick) >= ADC_BATT_UPDATE_MS) || tick == 0 )
    {
        // READ BATTERY ADC CHANNEL AND PERFORM AVERAGE
        for ( uint8_t i = 0; i < ADC_BATT_NUMREAD; i++ ) {
            adc_oneshot_read(adc_handle, ADC_BATT_CHANNEL, &adc_raw);
            adc_counts += (adc_raw / ADC_BATT_NUMREAD);
        }
        // CALCULATE ACTUAL BATTERY VOLTAGE
        battVoltage = ((adc_counts * 3300)/4095)*2; // *2 Because of 50% voltage dividor
    }

    // LOG UPDATE
    // ESP_LOGI(TAG, "Battery Voltage Requested = %ld [mV]", battVoltage);
}

/*
 * Calculates The Potentiometer Position 
 */
static void ADC_calcPotPosition ( void )
{
    // INITIALISE LOOP VARIABLES
    int adc_raw = 0;
    uint32_t adc_counts = 0;
    TickType_t now = xTaskGetTickCount();
    static TickType_t tick = 0; 

    // DONT BOTHER UPDATING IF ITS ALREADY BEEN CHECKED IN LAST 'ADC_BATT_UPDATE_MS' 
    if ( ((now - tick) >= ADC_POT_UPDATE_MS) || tick == 0 )
    {
        // TURN POT POWER ON AND WAIT TO ALLOW VOLTAGE TO STABILISE 
        // gpio_set_level( ADC_GPIO_POTOUT, true );
        // vTaskDelay( pdMS_TO_TICKS(1) );
        // READ BATTERY ADC CHANNEL AND PERFORM AVERAGE
        for ( uint8_t i = 0; i < ADC_BATT_NUMREAD; i++ ) {
            adc_oneshot_read(adc_handle, ADC_POT_CHANNEL, &adc_raw);
            adc_counts += (adc_raw / ADC_BATT_NUMREAD);
        }
        // RESET GPIO PIN TO SAVE POWER
        // gpio_set_level( ADC_GPIO_POTOUT, false );
        // ASSIGN COUNT TO GLOBAL VARIABLE
        potPosition = adc_counts;
    }

    // LOG UPDATE
    // ESP_LOGI(TAG, "Potentiometer Position Requested = %ld [counts]", potPosition);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */