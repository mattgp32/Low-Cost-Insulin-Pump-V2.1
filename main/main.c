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
#include "ble_comp_test.h"

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

#define POT_POWER               GPIO_NUM_37
#define uS_TO_S_FACTOR          1000000ULL
#define uS_TO_TICKHZ_FACTOR     10000

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void SYSTEM_init  ( void );

void task_SYSTEM_nap    ( void * );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool BT_already_on = true;
bool low_power_enable = false;
long long int timeb4slp = 0;

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
    //BUZZER_init();
    BT_run();
    MOTOR_init();
    ADC_init();
    BUTTON_init();
    
    // INDICATE INITIALISATION COMPLETE
    LED_wave();

    // START ALL RTOS TASKS
    //xTaskCreate(task_ADC_getBattLevel, "Read ADC and write batt level to a queue", 1024, NULL, 5, NULL);
    //xTaskCreate(task_LED_displayBattLevel, "Blink LED depending on batt level", 8192, NULL, 5, NULL);
    xTaskCreate(task_BT_receiveData, "get data from bt buffer",8192, NULL, 10, NULL);
    xTaskCreate(task_BT_processData, "print data from bt buffer",8192, NULL, 10, NULL);
    //xTaskCreate(task_INSRATE_retreiveData, "Display rate data - for debugging only", 8192, NULL, 5, NULL);
    xTaskCreate(task_LED_noBasilWarning, "flash led if br = 0", 2048, NULL, 5, NULL);
    xTaskCreate(task_INSRATE_giveInsulin, "start insulin deliveries", 4096, NULL, 21, NULL);
    xTaskCreate(task_INSRATE_deliverBolus, "give bolus", 4092, NULL, 20, NULL);
    xTaskCreate(task_INSRATE_rewindPlunger, "rewind motor if flag set", 4092, NULL, 4, NULL);
    xTaskCreate(task_BUTTON_printNum,"print num", 4092, NULL, 4, NULL);
    xTaskCreate(task_BT_off, "turn off BT", 4092, NULL, 4, NULL);
    xTaskCreate(task_BT_handler, "BT_Control_Task", 2048, NULL, 10, NULL); //
    xTaskCreate(task_LED_bluetoothRunningAlert, "flash_led_when BT active", 2048, NULL, 15, NULL); //
    xTaskCreate(task_LED_pumpIsAlive, "flash leds every minute so user knows pump is not dead", 2048, NULL, 15, NULL); //
    //xTaskCreate(task_SYSTEM_nap, "enable low power config after BT is off", 2048, NULL, 1, NULL);
    //xTaskCreate(task_INSRATE_beginLowPower, "enter sleep mode", 2048 , NULL, 19,NULL);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void task_SYSTEM_nap ( void *args )
{
    while (1)
    {
        if ( BT_already_on == false )
        {
            timeb4slp = esp_timer_get_time();
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 0);
            esp_sleep_enable_timer_wakeup(5*uS_TO_S_FACTOR);
            puts("goodnight");
            vTaskDelay(500);
            esp_light_sleep_start();
            vTaskStepTick((esp_timer_get_time() - timeb4slp)/ uS_TO_TICKHZ_FACTOR);
            // LED_flashDouble();
        } 
        vTaskDelay(200);
    } 
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void SYSTEM_init ( void )
{
    //Initialise system peripherals to be used after freeRTOS starts
    esp_pm_config_t pm_config = {
            .max_freq_mhz = 80,
            .min_freq_mhz = 40,
           .light_sleep_enable = false,
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
    // small delay might be necessary for the frequency setting to take effect — the idle task should have a chance to run
    vTaskDelay(pdMS_TO_TICKS(10));
    // now the frequency should be 40 MHz
    //assert(esp_clk_cpu_freq() == 80 * 1000000);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */