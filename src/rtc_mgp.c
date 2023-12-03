/*  **** Module written by Matt Payne as part as part of the BT Ultra Low Cost Insulin Pump Project
    **** Started on 7/3/23
    **** This is the .h file for the motor functions defined in rtc.c
*/

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h" 
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "ins_rate.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"
#include "esp_sleep.h"
#include "rtc_mgp.h"


#define PROFILE_A_APP_ID 0
#define TIMER_WAKEUP_TIME_US (20 * 1000 * 1000)

void sleep_for_20(void* arg)
{
    for(;;){

    
        // esp_err_t status = esp_ble_gatts_app_unregister(PROFILE_A_APP_ID);
        // if (status != ESP_OK) {
        //     printf("esp_ble_gatts_app_unregister status=%d\n", status);
            
        // }

        // status = esp_bluedroid_disable();
        // if (status != ESP_OK) {
        //     printf("esp_bluedroid_disable status=%d\n", status);
            
        // }

        // status = esp_bluedroid_deinit();
        // if (status != ESP_OK) {
        //     printf("esp_bluedroid_deinit status=%d\n", status);
            
        // }

        // status = esp_bt_controller_disable();
        // if (status != ESP_OK) {
        //     printf("esp_bt_controller_disable status=%d\n", status);
            
        // }

        // status = esp_bt_controller_deinit();
        // if (status != ESP_OK) {
        //     printf("esp_bt_controller_deinit status=%d\n", status);
            
        // }
        // status = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
        // if (status != ESP_OK) {
        //     printf("esp_bt_controller_mem_release status=%d\n", status);
            
        // }
        puts("goodnight");
        esp_sleep_enable_timer_wakeup(20000000);
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }  
}
