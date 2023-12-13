/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "leds.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG             "LED"

#define GPIO_LED1       GPIO_NUM_42
#define GPIO_LED2       GPIO_NUM_2
#define GPIO_LED3       GPIO_NUM_1

#define LED_HANDLER_DELAY (60*1000)

#define LED_FLASH_TIME  100
#define LED_FLASH_TIME2 50

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern QueueHandle_t battLevelQueue;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void LED_on    ( int );
void LED_off   ( int );

void LED_patternNoBasal          ( void );
void LED_patternVoltageCritical  ( void );
void LED_patternVoltageLow       ( void );
void LED_patternBluetoothON      ( void );
void LED_patternNormal           ( void );

void task_LED_handler ( void * );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Initialises Everything For LED Module Functionality 
 */
void LED_init ( void )
{
   // LOG
   ESP_LOGI(TAG, "Initialising LED Module");

   //
   gpio_reset_pin(GPIO_LED1);

   // INITIALISE LED GPIO
   gpio_set_direction(GPIO_LED1, GPIO_MODE_OUTPUT);
   gpio_set_direction(GPIO_LED2, GPIO_MODE_OUTPUT);
   gpio_set_direction(GPIO_LED3, GPIO_MODE_OUTPUT);
   
   // ENSURE ALL LED START OFF 
   LED_off(GPIO_LED1);
   LED_off(GPIO_LED2);
   LED_off(GPIO_LED3);

   // PERFORM LED WAVE TO INDICATE STARTUP
   LED_waveFour();
}

/*
 * Starts The RTOS LED Handler Task 
 */
void LED_start ( void )
{
   xTaskCreate(task_LED_handler, "Handles LED Functionality", (4*1024), NULL, 15, NULL);
}

/*
 * Perform LED Wave Pattern
 */
void LED_wave ( void )
{
   LED_on( GPIO_LED1 );
   vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
   LED_on( GPIO_LED2 );
   vTaskDelay( pdMS_TO_TICKS(40) );
   LED_off( GPIO_LED1 );
   vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
   LED_on( GPIO_LED3 );
   vTaskDelay( pdMS_TO_TICKS(40) );
   LED_off( GPIO_LED2 );
   vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
   LED_off( GPIO_LED3 );
}

/*
 * Perform LED Wave Pattern
 */
void LED_waveFour ( void )
{
   for ( int i = 0; i < 4; i++ )
   {
      LED_wave();
   }
}

/*
 * Flash LED1 Five Times
 */
void LED_flashFive_single ( void )
{
   // PREPARE LEDS
   LED_off( GPIO_LED1 );
   LED_off( GPIO_LED2 );
   LED_off( GPIO_LED3 );

   // FLASH LEDS
   for( int i = 0; i < 5; i++ )
   {
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED1 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED1 );
   }
}

/*
 * Flash LED1 and LED2 Five Times
 */
void LED_flashFive_double ( void )
{
   // PREPARE LEDS
   LED_off( GPIO_LED1 );
   LED_off( GPIO_LED2 );
   LED_off( GPIO_LED3 );

   // FLASH LEDS
   for(int i = 0; i < 5; i++)
   {
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED1 );
      LED_on( GPIO_LED2 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED1 );
      LED_off( GPIO_LED2 );
   }
}

/*
 * Flash LED1, LED2, and LED3 Five Times
 */
void LED_flashFive_triple ( void )
{
   // PREPARE LEDS
   LED_off( GPIO_LED1 );
   LED_off( GPIO_LED2 );
   LED_off( GPIO_LED3 );

   // FLASH LEDS
   for(int i = 0; i < 5; i++)
   {
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_on( GPIO_LED1 );
      LED_on( GPIO_LED2 );
      LED_on( GPIO_LED3 );
      vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
      LED_off( GPIO_LED1 );
      LED_off( GPIO_LED2 );
      LED_off( GPIO_LED3 );
   }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * RTOS Task That Handles LED Behaviour Based on Other Module Status
 */
void task_LED_handler ( void *args )
{
   // LOG
   ESP_LOGI(TAG, "Starting LED Handler Task");

   while (1)
   {
      // IS BATTERY IS CRITICALLY LOW
      if ( ADC_battCritical() ) {
         // ESP_LOGI(TAG, "Pattern: Critically Low Battery Voltage");
         LED_patternVoltageCritical();
         vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      }
      // IS BASAL RATE LOW
      else if( INSRATE_zeroBasalRate() ) {
         // ESP_LOGI(TAG, "Pattern: No Basal Rate Detected");
         LED_patternNoBasal();
         vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      }
      // IS BATTERY IS LOW
      else if ( ADC_battLow() ) {
         // ESP_LOGI(TAG, "Pattern: Low Battery Voltage");
         LED_patternVoltageLow();
         vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      }
      // IS BLUETOOTH IS ON
      else if ( BT_isON() ) {
         // ESP_LOGI(TAG, "Pattern: Bluetooth ON");
         LED_patternBluetoothON();
      }
      // NORMAL OPERATION
      else {
         // ESP_LOGI(TAG, "Pattern: Normal Operation");
         LED_patternNormal();
         vTaskDelay(pdMS_TO_TICKS(LED_HANDLER_DELAY));
      }
   }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Turn the Provided LED GPIO ON 
 */
void LED_on ( int LED )
{
   gpio_set_level(LED, false);
}

/*
 * Turn The Provided LED GPIO OFF
 */
void LED_off ( int LED )
{
   gpio_set_level(LED, true);
}

/*
 * LED Pattern When No Basal Rate Is Detected
 */
void LED_patternNoBasal ( void )
{
   LED_flashFive_double();
}

/*
 * LED Pattern When Battery Voltage Is Low
 */
void LED_patternVoltageCritical ( void )
{
   for ( int i = 0; i < 3; i++ ) {
      LED_waveFour();
   }
}

/*
 * LED Pattern When Battery Voltage Is Low
 */
void LED_patternVoltageLow ( void )
{
   for ( int i = 0; i < 3; i++ ) {
      LED_waveFour();
   }
}

/*
 * LED Pattern When Bluetooth Is Running
 */
void LED_patternBluetoothON ( void )
{
   LED_on(GPIO_LED1);
   vTaskDelay(pdMS_TO_TICKS(500));
   LED_off(GPIO_LED1);
   LED_on(GPIO_LED3);
   vTaskDelay(pdMS_TO_TICKS(500));
   LED_off(GPIO_LED3);
}

/*
 * LED Pattern During Normal Operation
 */
void LED_patternNormal ( void )
{
   LED_on(GPIO_LED1);
   vTaskDelay( pdMS_TO_TICKS(LED_FLASH_TIME) );
   LED_off(GPIO_LED1);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */