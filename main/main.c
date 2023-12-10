/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "stdio.h"
#include <inttypes.h>
#include <assert.h>
#include "nvs_flash.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_bt.h"
#include "esp_err.h"
#include "esp_pm.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_pm.h"
#include "esp_private/esp_clk.h"
#include "esp_sleep.h"
#include "esp_timer.h"
// #include "ble_comp_test.h"

#include "adc.h"
#include "motor.h"
#include "leds.h"
#include "ins_rate.h"
#include "pump_BT.h"
#include "buzzer.h"
#include "button.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define uS_TO_S_FACTOR          1000000ULL
#define uS_TO_TICKHZ_FACTOR     10000

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void SYSTEM_init ( void );

void task_SYSTEM_napTime ( void * );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool low_power_enable = false;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void app_main(void)
{
    // INITIALISE MODULES
    SYSTEM_init();
    LED_init();
    // BUZZER_init();
    BT_init();
    MOTOR_init();
    ADC_init();
    BUTTON_init();

    // START ALL RTOS TASKS
    BT_start();
    INSRATE_start();
    BUTTON_start();
    LED_start();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// /*
//  * Description
//  */
// void task_SYSTEM_napTime ( void *args )
// {
//     // INIT FUNCTION VARIABLES
//     static long long int timeb4slp = 0;

//     // LOOP TO INFINITY AND BEYOND
//     while (1)
//     {
//         if ( BT_already_on == false )
//         {
//             timeb4slp = esp_timer_get_time();
//             esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 0);
//             esp_sleep_enable_timer_wakeup(5*uS_TO_S_FACTOR);
//             puts("goodnight");
//             vTaskDelay(500);
//             esp_light_sleep_start();
//             vTaskStepTick((esp_timer_get_time() - timeb4slp)/ uS_TO_TICKHZ_FACTOR);
//             // LED_flashDouble();
//         } 

//         // LOOP PACING
//         vTaskDelay(200);
//     } 
// }

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void SYSTEM_init ( void )
{
    // INITIALISE AND APPLY CONFIG FOR POWER MANAGEMENT PERIPHERAL
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 40,
        .light_sleep_enable = false, };
    esp_pm_configure(&pm_config);

    // DELAY TO ALLOW CHANGES TO TAKE EFFECT - THE IDLE TASK SHOULD HAVE A CHANCE TO RUN 
    vTaskDelay(pdMS_TO_TICKS(10));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */