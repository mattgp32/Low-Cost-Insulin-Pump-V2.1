/* This module written by Matt Payne as part of the Bluetooth insulin pump project.
   This module is used to store and control insulin delivery data
   Started on 20/3/2023
*/

#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h" 
#include "freertos/semphr.h"
#include "leds.h"
#include "time.h"


uint8_t index_arr[2] = {0};


// Function to slice a string and find the index of two asterisks contained within it
void slice_string(const char *data)
{
    uint8_t index = 0;
    memset(index_arr, 0, sizeof(index_arr));
    
    for(uint8_t i = 0; (i < strlen(data)); i++)
    {
        if(data[i] == 42)
        {
            index_arr[index] = i;
            index++;
        }

    }
}

void init_rate_storage_nvs_partition(void)
{
    
    //esp_err_t ret;
    nvs_flash_init_partition("rate_storage");
    // if (ret == ESP_OK)
    // {
    //     printf("Partition succesfully initialised\n");
    // }
}

void write_bolus_data(int delivery_amount)
{
    nvs_handle_t bo_handle;
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);
    nvs_set_i16(bo_handle, "bolus_size", delivery_amount);
    nvs_commit(bo_handle);
}


void write_basal_rate_data(int delivery_amount)
{
    nvs_handle_t br_handle;
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READWRITE, &br_handle);
    
    // switch (ret)
    // {
    // case ESP_OK:
    //     printf("Partition open successful\n");
    //     break;
    // case ESP_FAIL:
    //     printf("Internal Error\n");
    //     break;
    // case ESP_ERR_NVS_NOT_INITIALIZED:
    //     printf("Partition not initialised\n");
    //     break;
    // case ESP_ERR_NVS_INVALID_NAME:
    //     printf("Name doesnt meet constraints\n");
    //     break;
    // default:
    //     printf("Some other error, work it out yourself!\n");
    //     break;
    // }

    nvs_set_i16(br_handle, "basal_rate", delivery_amount);

    // switch (ret)
    // {
    // case ESP_OK:
    //     printf("Data written\n");
    //     break;
    // case ESP_FAIL:
    //     printf("Internal Error\n");
    //     break;
    // case ESP_ERR_NVS_INVALID_HANDLE:
    //     printf("Handle has been closed or == NULL\n");
    //     break;
    // case ESP_ERR_NVS_READ_ONLY:
    //     printf("Didnt open as read write\n");
    //     break;
    // case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
    //     printf("Not enough memory buddy\n");
    //     break;
    // default:
    //     printf("Some other error, work it out yourself!\n");
    //     break;
    // }

    nvs_commit(br_handle);
    // switch (ret)
    // {
    // case ESP_OK:
    //     printf("Data commited\n");
    //     break;
    // case ESP_ERR_NVS_INVALID_HANDLE:
    //     printf("Handle has been closed or == NULL\n");
    //     break;
    // default:
    //     printf("Some other error, work it out yourself!\n");
    //     break;
    // }
}

void read_and_store_data(const char *data)
{
    char delivery_type[2];
    uint8_t len = (index_arr[1] - index_arr[0])-1;
    char delivery_amount[len];
    strncpy(delivery_type, data, 2);
    slice_string(data);
    //printf("%s\n", delivery_type);
    if (index_arr[0] == 0 || index_arr[1] == 0 ){
        //printf("ERROR - INCORRECT ENTRY FORMAT\n");
    } else {
        memset(delivery_amount, 0, sizeof(delivery_amount));
        strncpy(delivery_amount, &data[index_arr[0]+1] , (index_arr[1] - index_arr[0])-1);
    }

    float delivery_amount_f = atof(delivery_amount) * 100;
    int delivery_amount_i = (int)delivery_amount_f;
    //printf("%d\n", delivery_amount_i);
    init_rate_storage_nvs_partition();

    if (strcmp(delivery_type, "BA") == 0)
    {
        write_basal_rate_data(delivery_amount_i);
        led_five_flash();
    } else if (strcmp(delivery_type, "BO") == 0) {
        write_bolus_data(delivery_amount_i);
        led_double_flash(); 
    }
}

//Just used for checking if nvs and BT was working, not used in final version
void retreive_data(void* arg)
{
    for(;;)
    {
    int16_t basal_rate = 0;
    int16_t bolus_size = 0;
    nvs_handle_t br_handle;
    nvs_handle_t bo_handle;
    nvs_flash_init_partition("rate_storage");
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READONLY, &bo_handle);
    nvs_get_i16(br_handle, "basal_rate", &basal_rate);
    nvs_get_i16(bo_handle, "bolus_size", &bolus_size);
    printf("Current basal rate is %d\n", basal_rate);
    printf("Current bolus amount is %d\n", bolus_size);
    vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void get_current_time(void* arg)
{
    for(;;)
    {
        time_t now = (1679964113);
        char strftime_buf[64];
        struct tm timeinfo;

        setenv("TZ", "UTC-13", 1);
        tzset();

        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf("The currect time in shnaghai is: %s\n", strftime_buf);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
