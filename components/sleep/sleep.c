/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sleep.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG                         "SLEEP"

#define BUTTON_GPIO                 GPIO_NUM_5
#define uS_TO_S_FACTOR              1000000ULL
#define SLEEP_TIME_SECONDS          8

#define uS_TO_TICKS_FACTOR          100001

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void task_SLEEP_startLightSleep     (void *);
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
long long int timeBeforeSleep;
long long int timeSpentAsleep;
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void SLEEP_start    ( void )
{
    xTaskCreate(task_SLEEP_startLightSleep, "Let ESP32 Light Sleep When BT Is Off", (1024*8), NULL, 10, NULL);
}
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void task_SLEEP_startLightSleep     ( void *arg )
{
    //LOG
    ESP_LOGI(TAG, "Starting Light Sleep Task");

    //START FREERTOS ENDLESS LOOP
    while(1)
    {
        if (BT_isON() == false)
        {
            // ENABLE GPIO AND SET A TIMER AS WAKEUP SOURCES
            esp_sleep_enable_ext0_wakeup(BUTTON_GPIO, false);
            esp_sleep_enable_timer_wakeup(SLEEP_TIME_SECONDS*uS_TO_S_FACTOR);
            timeBeforeSleep = esp_timer_get_time();
            // LOG MESSAGE AT TIME OF ESP32 GOING TO SLEEP
            ESP_LOGI(TAG, "ESP32 Entering Light Sleep");
            esp_light_sleep_start();
            //  CALCULATE TIME SPENT ASLEEP
            timeSpentAsleep = esp_timer_get_time() - timeBeforeSleep;
            ESP_LOGI(TAG, "ESP32 Woke From Light Sleep After %lld Microseconds", timeSpentAsleep);
            // UPDATE THE TICK COUNT BASED ON TIME SPENT ASLEEP
            xTaskCatchUpTicks((timeSpentAsleep) / uS_TO_TICKS_FACTOR);
        }
        vTaskDelay(12000);
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