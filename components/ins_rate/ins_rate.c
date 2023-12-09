/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ins_rate.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define STEPS_PER_UNIT              740         // Altered based on testing
#define MIN_DELIVERY_SIZE           25
#define MIN_DELIVERY_STEPS          18
#define MIN_BOLUS_DELIVERY_SIZE     50
#define MIN_BOLUS_DELIVERY_STEPS    36

#define SECONDS_TO_MS               1000
#define THREE_MINUTES               180
#define SECONDS_IN_AN_HOUR          3600
#define uS_TO_S_FACTOR              1000000ULL

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void INS_RATE_writeData_bolus       ( int );
void INS_RATE_writeData_rewind      ( int );
int  INS_RATE_setDeliveryFrequency  ( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uint8_t index_arr[2] = {0};
time_t esp_time;
time_t actual_time;
time_t unix_modifier = 0;
struct tm * timeinfo;
long long int t_current = 0;
long long int t_prev = 0;
QueueHandle_t bolus_delivery_queue;
SemaphoreHandle_t basal_semaphore = NULL;
extern bool BT_already_on;

esp_sleep_wakeup_cause_t wake_cause;

TickType_t frequency = 1000;
int basal_info_array[2];
extern int pot_read_global;
bool bolus_ready = false;
bool RW_flag = false;
extern bool disable_BT;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
        INS_RATE_writeData_bolus(bolus_size_i);
        bolus_ready = true;
    } else if (strcmp(data_type, "RE") == 0) {
        RW_flag=true;
    } else if (strcmp(data_type, "PR") == 0) {
        float prime_size = 0;
        INSRATE_sliceString(data);
        char prime_delivery[index_arr[1]-index_arr[0]];
        prime_delivery[index_arr[1]-index_arr[0]-1] = '\0';
        strncpy(prime_delivery, &data[index_arr[0]+1] , (index_arr[1] - index_arr[0])-1);
        prime_size = atof(prime_delivery);
        printf("Entered prime amount is %d\n", (int)prime_size);

        for(int i = 0; i < (int)prime_size; i++){
            MOTOR_stepX(true, STEPS_PER_UNIT);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

    } else {
         printf("You have entered an invalid type\n");
    }
}

/*
 * Description
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
 * Description
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
 * Desciption 
 */
void INSRATE_writeData_basalRate ( int delivery_amount )
{
    nvs_handle_t br_handle;
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READWRITE, &br_handle);
    delivery_amount = (delivery_amount / 25) * 25; //force answer to a multiple of 0.025U
    if (delivery_amount <= 0){
        LED_flashDouble();
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
 * Description
 */
int INSRATE_setDeliveryFrequencyTest(int freq)
{
    return freq;
}

/*
 * Function to slice a string and find the index of two asterisks contained within it
 */
bool INSRATE_checkBolusCancelled ( void ) 
{
    int32_t bolus_size = 0;
    nvs_handle_t bo_handle;
    bool cancelled = false;

    nvs_flash_init_partition("rate_storage");
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);   
    nvs_get_i32(bo_handle, "bolus_size", &bolus_size);

    if ( bolus_size==0 )
    {
        cancelled = true;
        nvs_set_i32(bo_handle, "bolus_size", 0);
        nvs_commit(bo_handle);
    }

    return cancelled;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Just used for checking if nvs and BT was working, not used in final version
 */
void task_INSRATE_retreiveData ( void *arg )
{
    while(1)
    {
    int32_t basal_rate = 0;
    int32_t bolus_size = 0;
    
    long long int elapsed_time;
    
    setenv("TZ", "UTC-12", 1);
    tzset();
    actual_time = time(&esp_time) + unix_modifier;
    timeinfo = localtime(&actual_time);
    t_prev = t_current;
    t_current = esp_timer_get_time();
    elapsed_time = t_current - t_prev;
    printf ("Current local time and date: %s", asctime(timeinfo));
    printf("Elapsed time is %lld\n", elapsed_time/1000000);

    nvs_handle_t br_handle;
    nvs_handle_t bo_handle;
    nvs_flash_init_partition("rate_storage");
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READONLY, &bo_handle);
    nvs_get_i32(br_handle, "basal_rate", &basal_rate);
    nvs_get_i32(bo_handle, "bolus_size", &bolus_size);
    printf("Current basal rate is %ld\n", basal_rate);
    printf("Current bolus amount is %ld\n", bolus_size);
    vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/*
 * Description 
 */
void task_INSRATE_giveInsulin ( void *arg )
{
    while(1)
    {
        puts("give_insulin begin");
        // if(xSemaphoreTake(basal_semaphore, portMAX_DELAY))
        // {
        frequency = INS_RATE_setDeliveryFrequency() * SECONDS_TO_MS;
        // frequency = INSRATE_setDeliveryFrequencyTest(500) * SECONDS_TO_MS;
        //LED_flashDouble();

            if (frequency <= 0) {
                //puts("basal rate is 0");
                frequency = pdMS_TO_TICKS(THREE_MINUTES*SECONDS_TO_MS);
            } else {
            //puts("Entered motor control block\n");
           // printf("Turning %d steps\n", (int)(STEPS_PER_UNIT*basal_info_array[1])/(basal_info_array[0]*1000));
            MOTOR_stepX(true, (int)(STEPS_PER_UNIT*basal_info_array[1])/(basal_info_array[0]*1000));
            ADC_readpot();
            // MOTOR_stepX(true, (int)(STEPS_PER_UNIT));
            }
             
    //} a
        
        vTaskDelay(pdMS_TO_TICKS(10000));
        puts("give_insulin end");
    }
    
}

/*
 * Description
 */
void task_INSRATE_beginLowPower ( void *args )
{
    while(1)
    {
        if ( BT_already_on == false )
        {
            //basal_semaphore = xSemaphoreCreateBinary();
            puts("goodnight");
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 0);
            esp_sleep_enable_timer_wakeup(10*uS_TO_S_FACTOR);
            esp_light_sleep_start();
            //wake_cause = esp_sleep_get_wakeup_cause();
            LED_flashDouble();
            
            // if (wake_cause == ESP_SLEEP_WAKEUP_EXT0)
            //     {
            //         puts("RESTARTING");
            //         LED_flashFive();
            //         esp_restart();

            //     } else if (wake_cause == ESP_SLEEP_WAKEUP_TIMER)
            //     {
            //         xSemaphoreGive(basal_semaphore);
            //         LED_flashFive();
            //         LED_flashFive();
            //     }
        }
    vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*
 * Description
 */
void task_INSRATE_deliverBolus ( void *arg )
{
    while(1)
    {
        puts("bolus_delivery begin");
        nvs_handle_t bo_handle;
        int32_t bolus_size = 0;

        if(bolus_ready == true)
        {
            nvs_flash_init_partition("rate_storage");
            nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);   
            nvs_get_i32(bo_handle, "bolus_size", &bolus_size);
                if((bolus_size/MIN_DELIVERY_SIZE) == 0){
                    puts("ReQueSteD bOLuS iS ToO SMalL!!!");
                    LED_flashDouble();
                } else {
                    int n_steps = bolus_size/MIN_BOLUS_DELIVERY_SIZE;
                    printf("Delivering %d doses of 0.05U\n", n_steps);
                    for(int i = 0; i < n_steps; i++){
                        if(!INSRATE_checkBolusCancelled()){
                        MOTOR_stepX(true, MIN_BOLUS_DELIVERY_STEPS);
                        vTaskDelay(pdMS_TO_TICKS(1200));
                        } else {
                            puts("Bolus Cancelled");
                            break;
                        }
                    }

                    if((bolus_size % MIN_BOLUS_DELIVERY_SIZE) == MIN_DELIVERY_SIZE) {
                        MOTOR_stepX(true, MIN_DELIVERY_STEPS);
                        puts("Delivering 1 dose of 0.025U");
                    }

                    
                    puts("Bolus delivery complete");
                    ADC_readpot();
                    //disable_BT = true;
            
            }
        }
        nvs_set_i32(bo_handle, "bolus_size", 0);
        nvs_commit(bo_handle);
        bolus_ready = false;
       
        vTaskDelay(pdMS_TO_TICKS(1000));
        puts("bolus_delivery_end");
    } 
}

/*
 * Description
 */
void task_INSRATE_rewindPlunger ( void *arg )
{
    while(1)
    {
         puts("rewind_plunge begin");
        if(RW_flag == true) {
            MOTOR_stepX(false, STEPS_PER_UNIT*2);
            
        }
        ADC_readpot();
        if(pot_read_global <=0){
            RW_flag = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        puts("rewind_plunge end");
    }
    
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void INS_RATE_writeData_bolus ( int delivery_amount )
{
    nvs_handle_t bo_handle;
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);
    delivery_amount = (delivery_amount / 25) * 25; //force answer to a multiple of 0.025U
    if (delivery_amount <= 0){
        LED_flashDouble();
    }
     nvs_set_i32(bo_handle, "bolus_size", delivery_amount);
     nvs_commit(bo_handle);
}

/*
 * Description
 */
void INS_RATE_writeData_rewind ( int delivery_amount )
{
    nvs_handle_t br_handle;
    nvs_open_from_partition("rate_storage", "rewi", NVS_READWRITE, &br_handle);
    delivery_amount = (delivery_amount / 25) * 25; //force answer to a multiple of 0.025U
    if (delivery_amount <= 0){
        LED_flashDouble();
    }

    nvs_set_i32(br_handle, "basal_rate", delivery_amount);
    nvs_commit(br_handle);
   
}

/*
 * Description
 */
int INS_RATE_setDeliveryFrequency ( void )
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
        dels_per_hour = 20;
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
        LED_flashDouble();
    }
     nvs_set_i32(bo_handle, "bolus_size", delivery_amount);
     nvs_commit(bo_handle);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */