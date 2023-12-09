/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ins_rate.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define STEPS_PER_UNIT 740
#define SECONDS_TO_MS 1000
#define THREE_MINUTES 180
#define SECONDS_IN_AN_HOUR 3600
#define MIN_DELIVERY_SIZE 25
#define MIN_DELIVERY_STEPS 18
#define MIN_BOLUS_DELIVERY_SIZE 50
#define MIN_BOLUS_DELIVERY_STEPS 36

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uint8_t index_arr[2] = {0};
time_t esp_time;
time_t actual_time;
time_t unix_modifier = 0;
struct tm * timeinfo;
QueueHandle_t bolus_delivery_queue;

TickType_t frequency = 1000;
int basal_info_array[2];
bool bolus_ready = false;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void INSRATE_start ( void )
{
    xTaskCreate(task_INSRATE_retreiveData,  "Insulin Rate Retreive Data Task", CONFIG_INS_RATE_RETREIVE_TASK_HEAP*1024,      NULL, CONFIG_INS_RATE_RETREIVE_TASK_PRIORITY,      NULL);
    xTaskCreate(task_INSRATE_giveInsulin,   "Insulin Rate Give Insulin Task",  CONFIG_INS_RATE_GIVE_INSULIN_TASK_HEAP*1024,  NULL, CONFIG_INS_RATE_GIVE_INSULIN_TASK_PRIORITY,  NULL);
    xTaskCreate(task_INSRATE_deliverBolus, "Insulin Rate Deliver Bolus Task", CONFIG_INS_RATE_DELIVER_BOLUS_TASK_HEAP*1042, NULL, CONFIG_INS_RATE_DELIVER_BOLUS_TASK_PRIORITY, NULL);
}

/*
 * Description
 */
void INSRATE_readAndStoreData ( const char *data )
{
    char data_type[3];
    strncpy(data_type, data, 2);
    data_type[2] = '\0';
    printf("The data type here is %s\n", data_type);
    
    if(strcmp(data_type, "TI") == 0){
        //printf("This is a time type\n");
        INSRATE_sliceString(data);
        //printf("Index values are at %d, %d\n", index_arr[0], index_arr[1]);
        char unix_time[index_arr[1]-index_arr[0]];
        strncpy(&unix_time[0], &data[index_arr[0]+1], index_arr[1]-index_arr[0]-1);
        unix_time[10] = '\0';
         unix_modifier = atoi(unix_time);
        //printf("UNIX time = %lld\n", unix_modifier);

    } else if (strcmp(data_type, "BA") == 0) {
        float basal_rate = 0;
        int basal_rate_i = 0;
        //printf("This is of basal insulin delivery type\n");
        INSRATE_sliceString(data);
        //printf("Index values are at %d, %d\n", index_arr[0], index_arr[1]);
        char basal_delivery[index_arr[1]-index_arr[0]];
        basal_delivery[index_arr[1]-index_arr[0]-1] = '\0';
        strncpy(basal_delivery, &data[index_arr[0]+1] , (index_arr[1] - index_arr[0])-1);
        basal_rate = atof(basal_delivery);
        basal_rate_i = basal_rate*1000;
        //printf("Current basal rate is %d\n", basal_rate_i);
        INSRATE_initStoragePartition();
        INSRATE_writeData_basalRate(basal_rate_i);

    } else if (strcmp(data_type, "BO") == 0) {
        float bolus_size = 0;
        int bolus_size_i = 0;
        //printf("This is of bolus insulin delivery type %s\n", data);
        INSRATE_sliceString(data);
        //printf("Index values are at %d, %d\n", index_arr[0], index_arr[1]);
        char bolus_delivery[index_arr[1]-index_arr[0]];
        bolus_delivery[index_arr[1]-index_arr[0]-1] = '\0';
        strncpy(bolus_delivery, &data[index_arr[0]+1] , (index_arr[1] - index_arr[0])-1);
        bolus_size = atof(bolus_delivery);
        bolus_size_i = bolus_size*1000;
        printf("Entered bolus_size is %d\n", bolus_size_i);
        INSRATE_initStoragePartition();
        INSRATE_writeData_Bolus(bolus_size_i);
        bolus_ready = true;
    } else {
         //printf("You have entered an invalid type\n");
    }
}

/*
 * Function to slice a string and find the index of two asterisks contained within it
 */
void INSRATE_sliceString ( const char *data ) 
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

/*
 * Function Description
 */
void INSRATE_initStoragePartition ( void )
{
    //esp_err_t ret;
    nvs_flash_init_partition("rate_storage");
    // if (ret == ESP_OK)
    // {
    //     printf("Partition succesfully initialised\n");
    // }
}

/*
 * Function Description
 */
void INSRATE_writeData_basalRate ( int delivery_amount )
{
    nvs_handle_t br_handle;
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READWRITE, &br_handle);
    delivery_amount = (delivery_amount / 25) * 25; //force answer to a multiple of 0.025U
    if (delivery_amount <= 0){
        LED_doubleFlash();
    }
    
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

    nvs_set_i32(br_handle, "basal_rate", delivery_amount);

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

/*
 * Function Description
 */
int INSRATE_setDeliveryFrequencyTest ( void )
{
    int32_t basal_rate = 0;
    nvs_handle_t br_handle;

    nvs_flash_init_partition("rate_storage");
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
    nvs_get_i32(br_handle, "basal_rate", &basal_rate);

    int dels_per_hour = basal_rate / MIN_DELIVERY_SIZE;
    int dels_freq;

    if (dels_per_hour <=0) {
        dels_freq = 0;
    } else if (dels_per_hour < 20){
        dels_freq = SECONDS_IN_AN_HOUR/dels_per_hour; //seconds between doses = dels_freq
    }
    else {
        dels_freq = THREE_MINUTES;
    }
    basal_info_array[0] = dels_per_hour;
    basal_info_array[1] = basal_rate;

    return dels_freq;
}

/*
 * Function Description
 */
void INSRATE_writeData_Bolus ( int delivery_amount )
{
    nvs_handle_t bo_handle;
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);
    delivery_amount = (delivery_amount / 25) * 25; //force answer to a multiple of 0.025U
    if (delivery_amount <= 0){
        LED_doubleFlash();
    }
     nvs_set_i32(bo_handle, "bolus_size", delivery_amount);
     nvs_commit(bo_handle);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Just used for checking if nvs and BT was working, not used in final version
 * Display rate data - for debugging only
 */
void task_INSRATE_retreiveData ( void *arg )
{
    // Create Function Variables
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Loop To Infinity And Beyond
    while(1)
    {
        int32_t basal_rate = 0;
        int32_t bolus_size = 0;
        
        setenv("TZ", "UTC-12", 1);
        tzset();
        actual_time = time(&esp_time) + unix_modifier;
        timeinfo = localtime(&actual_time);
        
        printf ("Current local time and date: %s", asctime(timeinfo));

        nvs_handle_t br_handle;
        nvs_handle_t bo_handle;
        nvs_flash_init_partition("rate_storage");
        nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
        nvs_open_from_partition("rate_storage", "bolus_size", NVS_READONLY, &bo_handle);
        nvs_get_i32(br_handle, "basal_rate", &basal_rate);
        nvs_get_i32(bo_handle, "bolus_size", &bolus_size);
        printf("Current basal rate is %ld\n", basal_rate);
        printf("Current bolus amount is %ld\n", bolus_size);

        // Loop Pacing
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONFIG_INS_RATE_RETREIVE_TASK_INTERVAL));
    }
}

/*
 * start insulin deliveries
 */
void task_INSRATE_giveInsulin ( void* arg )
{
    // Loop To Infinity And Beyond
    while(1)
    {
        frequency = INSRATE_setDeliveryFrequencyTest() * SECONDS_TO_MS;

        if (frequency <= 0) {
            puts("basal rate is 0");
            frequency = pdMS_TO_TICKS(THREE_MINUTES*SECONDS_TO_MS);
        } else {
        puts("Entered motor control block\n");
        MOTOR_stepX(true, (int)(STEPS_PER_UNIT*basal_info_array[1])/(basal_info_array[0]*1000));
        }
        vTaskDelay(pdMS_TO_TICKS(frequency));
    }
}

/*
 * give bolus
 */
void task_INSRATE_deliverBolus ( void* arg )
{
    // Create Function Variables
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Loop To Infinity And Beyond
    while(1)
    {
        nvs_handle_t bo_handle;
        int32_t bolus_size = 0;

        if(bolus_ready == true)
        {
            nvs_flash_init_partition("rate_storage");
            nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);   
            nvs_get_i32(bo_handle, "bolus_size", &bolus_size);

            if((bolus_size/MIN_DELIVERY_SIZE) == 0){
                puts("ReQueSteD bOLuS iS ToO SMalL!!!");
                LED_doubleFlash();
            } else {
                int n_MOTOR_PIN_STEPs = bolus_size/MIN_BOLUS_DELIVERY_SIZE;
                printf("Delivering %d doses of 0.05U\n", n_MOTOR_PIN_STEPs);
                for(int i = 0; i < n_MOTOR_PIN_STEPs; i++){
                    MOTOR_stepX(true, MIN_BOLUS_DELIVERY_STEPS);
                    vTaskDelay(pdMS_TO_TICKS(1200));
                }

                if((bolus_size % MIN_BOLUS_DELIVERY_SIZE) == MIN_DELIVERY_SIZE) {
                    MOTOR_stepX(true, MIN_DELIVERY_STEPS);
                    puts("Delivering 1 dose of 0.025U");
                }

                puts("Bolus delivery complete");

            }

        }
        nvs_set_i32(bo_handle, "bolus_size", 0);
        nvs_commit(bo_handle);
        bolus_ready = false;
        
        // Loop Pacing
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONFIG_INS_RATE_DELIVER_BOLUS_TASK_INTERVAL));
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