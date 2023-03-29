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
#include "stdio.h"
#include "pump_BT.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "ins_rate.h"

void app_main(void)
{
// Initialise system peripherals to be used after freeRTOS starts
    init_leds();
    //buzzer_init();
    run_BT();
    //init_motor();
    // Create all tasks for the freeRTOS scheduler
    xTaskCreate(get_batt_level, "Read ADC and write batt level to a queue", 1024, NULL, 5, NULL);
    xTaskCreate(display_batt_level, "Blink LED depending on batt level", 8192, NULL, 5, NULL);
    xTaskCreate(receive_BT_data, "get data from bt buffer",8192, NULL, 10, NULL);
    xTaskCreate(process_bt_data, "print data from bt buffer",8192, NULL, 10, NULL);
    //xTaskCreate(get_current_time, "get time", 8192, NULL, 5, NULL);
}