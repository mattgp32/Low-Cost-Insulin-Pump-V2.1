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

#define DELIVERY_PER_HR_MAX         20

#define SECONDS_TO_MS               1000
#define THREE_MINUTES               180
#define SECONDS_IN_AN_HOUR          3600
#define uS_TO_S_FACTOR              1000000ULL

#define INSRATE_DELIVERY_LOOP_DELAY 10000
#define INSRATE_SLEEP_LOOP_DELAY    1000
#define INSRATE_BOLUS_LOOP_DELAY    1000
#define INSRATE_REWIND_LOOP_DELAY    1000

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void INSRATE_sliceString                ( const char * );
void INSRATE_initStoragePartition       ( void );
void INSRATE_writeData_basalRate        ( int );
void INSRATE_writeData_bolus            ( int );
void INSRATE_writeData_rewind           ( int );
int  INSRATE_truncateAndCheck           ( int );
// int  INSRATE_setDeliveryFrequencyTest   ( int );
bool INSRATE_checkBolusCancelled        ( void );
int  INSRATE_calculateDeliveryFrequency ( void );

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
 * TAKE IN, PROCESS, AND SAVE NEW DATA
 */
void INSRATE_readAndStoreData ( const char *data )
{
    // INITIALISE FUNCTION VARIABLES
    char data_type[3];
    float dataInsulin = 0;
    int dataInsulin_i = 0;

    // COPY IN, FORMAT AND PRINT DATA
    strncpy(data_type, data, 2);
    data_type[2] = '\0';
    printf("The data type here is %s\n", data_type);

    // SLICE DATA STRING
    INSRATE_sliceString(data);

    // DATATYPE: TIME
    if( strcmp(data_type, "TI") == 0 )
    {
        // EXTRACT UNIX TIME 
        char unix_time[index_arr[1]-index_arr[0]];
        strncpy(&unix_time[0], &data[index_arr[0]+1], index_arr[1]-index_arr[0]-1);
        unix_time[10] = '\0';
        unix_modifier = atoi(unix_time);
    } 
    // DATATYPE: BASIL RATE
    else if ( strcmp(data_type, "BA") == 0 ) 
    {        
        // EXTRACT BASIL DATA
        char basal_delivery[index_arr[1]-index_arr[0]];
        basal_delivery[index_arr[1]-index_arr[0]-1] = '\0';
        strncpy(basal_delivery, &data[index_arr[0]+1] , (index_arr[1] - index_arr[0])-1);
        dataInsulin = atof(basal_delivery);
        dataInsulin_i = dataInsulin*1000;

        // INITIALISE PARTITION AND WRITE BASIL RATE
        INSRATE_initStoragePartition();
        INSRATE_writeData_basalRate(dataInsulin_i);
    } 
    // DATATYPE: BOLUS
    else if ( strcmp(data_type, "BO") == 0 ) 
    {        
        // EXTRACT BOLUS DATA
        char bolus_delivery[index_arr[1]-index_arr[0]];
        bolus_delivery[index_arr[1]-index_arr[0]-1] = '\0';
        strncpy(bolus_delivery, &data[index_arr[0]+1] , (index_arr[1] - index_arr[0])-1);
        dataInsulin = atof(bolus_delivery);
        dataInsulin_i = dataInsulin*1000;

        //Print? --------------------------------------------------------------------------------------- why print
        printf("Entered bolus_size is %d\n", dataInsulin_i);
        
        // INITIALISE PARTITION AND WRITE BOLUS
        INSRATE_initStoragePartition();
        INSRATE_writeData_bolus(dataInsulin_i);

        // TOGGLE FLAG 
        bolus_ready = true; // ---------------------------------------------------------- NO ABORT EVEN WHEN 0 RATE BOLUS SET? 
    } 
    // DATATYPE: MOTOR REWIND
    else if ( strcmp(data_type, "RE") == 0 ) 
    {
        RW_flag = true;
    } 
    // DATATYPE: PRIME STRINGE
    else if ( strcmp(data_type, "PR") == 0 ) 
    {
        // EXTRACT PRIME DATA
        char prime_delivery[index_arr[1]-index_arr[0]];
        prime_delivery[index_arr[1]-index_arr[0]-1] = '\0';
        strncpy(prime_delivery, &data[index_arr[0]+1] , (index_arr[1] - index_arr[0])-1);
        dataInsulin = atof(prime_delivery);
        
        // PRINT? ---------------------------------------------------------------------------------
        printf("Entered prime amount is %d\n", (int)dataInsulin);

        // REWIND SYRINGE AS PER DATA
        for(int i = 0; i < (int)dataInsulin; i++) 
        {
            MOTOR_stepX(true, STEPS_PER_UNIT);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    } 
    // DATATYPE: UNKNOWN
    else {
        printf("You have entered an invalid type\n");
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// /*
//  * Just used for checking if nvs and BT was working, not used in final version
//  */
// void task_INSRATE_retreiveData ( void *arg )
// {
//     while (1)
//     {
//         // INITIALISE LOOP VARIABLES
//         int32_t basal_rate = 0;
//         int32_t bolus_size = 0;
//         long long int elapsed_time;
//         nvs_handle_t br_handle;
//         nvs_handle_t bo_handle;
        
//         // TIME STUFF ----------------------------------------------------- VERIFY
//         setenv("TZ", "UTC-12", 1);
//         tzset();
//         actual_time = time(&esp_time) + unix_modifier;
//         timeinfo = localtime(&actual_time);
//         t_prev = t_current;
//         t_current = esp_timer_get_time();
//         elapsed_time = t_current - t_prev;
//         printf ("Current local time and date: %s", asctime(timeinfo));
//         printf("Elapsed time is %lld\n", elapsed_time/1000000);

//         // INITIALISE PARTITION AND READ BASAL AND BOLUS DATA
//         nvs_flash_init_partition("rate_storage");
//         nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
//         nvs_open_from_partition("rate_storage", "bolus_size", NVS_READONLY, &bo_handle);
//         nvs_get_i32(br_handle, "basal_rate", &basal_rate);
//         nvs_get_i32(bo_handle, "bolus_size", &bolus_size);

//         // DISPLAY INSULIN DATA
//         printf("Current basal rate is %ld\n", basal_rate);
//         printf("Current bolus amount is %ld\n", bolus_size);

//         // LOOP PACING
//         vTaskDelay(pdMS_TO_TICKS(10000));
//     }
// }

/*
 * UPDATE DELIVERY FREQUENCY VALUE FROM NVS DATA
 */
void task_INSRATE_giveInsulin ( void *arg )
{
    while (1)
    {
        // 
        puts("give_insulin begin");

        // EXTRACT DELIVERY FREQUENCY FROM NVS
        frequency = INSRATE_calculateDeliveryFrequency() * SECONDS_TO_MS;

        // 
        if (frequency > 0) 
        {
            // STEP MOTOR 
            MOTOR_stepX( true, (int)(STEPS_PER_UNIT * basal_info_array[1]) / (basal_info_array[0]*1000) );
            // READ IN SYRING POT POSITION
            ADC_updatePot();
        }
        else 
        {
            // SET FREQUENCY TO... ? 
            frequency = pdMS_TO_TICKS(THREE_MINUTES*SECONDS_TO_MS);
        }

        // LOOP PACING
        vTaskDelay( pdMS_TO_TICKS(INSRATE_DELIVERY_LOOP_DELAY) );

        //
        puts("give_insulin end"); // --------------------------------------------------------------------------- puts after delay?
    }
    
}

/*
 * Description
 */
void task_INSRATE_beginLowPower ( void *args )
{
    while(1)
    {
        // CHECK IF BLUETOOTH IS OFF
        if ( BT_isON() == false )
        {
            //
            puts("goodnight");
            //
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 0);
            esp_sleep_enable_timer_wakeup(10*uS_TO_S_FACTOR);
            esp_light_sleep_start();
            //
            LED_flashFive_double();
        }
        // 
        vTaskDelay( pdMS_TO_TICKS(INSRATE_SLEEP_LOOP_DELAY) );
    }
}

/*
 * Description
 */
void task_INSRATE_deliverBolus ( void *arg )
{
    while(1)
    {
        // MORE PUTS? 
        puts("bolus_delivery begin");
        
        // INITIALISE LOOP VARIABLES
        nvs_handle_t bo_handle;
        int32_t bolus_size = 0;

        // CHECK FOR NEW BOLUS WRITTEN
        if ( bolus_ready == true )
        {
            // INITIALISE PARTITION AND READ BOLUS AMOUNT
            nvs_flash_init_partition("rate_storage");
            nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);   
            nvs_get_i32(bo_handle, "bolus_size", &bolus_size);

            // DOUBLE CHECK FOR INCORRECT BOLUS
            if ( (bolus_size/MIN_DELIVERY_SIZE) == 0 )
            {
                puts("ReQueSteD bOLuS iS ToO SMalL!!!");
                LED_flashFive_double();
            } 
            // DELIVERY REQUESTED BOLUS
            else 
            {
                // CALCULATE REQUIRED MOTOR STEPS
                int n_steps = bolus_size/MIN_BOLUS_DELIVERY_SIZE;
                // PRINT -----------------------------------------------------------------------------------------------------
                printf("Delivering %d doses of 0.05U\n", n_steps);
                // DELIVERY EVERY MOTOR STEP
                for ( int i = 0; i < n_steps; i++ )
                {
                    if ( !INSRATE_checkBolusCancelled() )
                    {
                        MOTOR_stepX( true, MIN_BOLUS_DELIVERY_STEPS ); // why is this a different number to the minimum bolus delivery size?
                        vTaskDelay(pdMS_TO_TICKS(1200));
                    } 
                    else 
                    {
                        puts("Bolus Cancelled");
                        break;
                    }
                }

                // 
                if ( (bolus_size % MIN_BOLUS_DELIVERY_SIZE) == MIN_DELIVERY_SIZE) 
                {
                    MOTOR_stepX(true, MIN_DELIVERY_STEPS);
                    puts("Delivering 1 dose of 0.025U");
                }

                puts("Bolus delivery complete");
                ADC_updatePot();
                //disable_BT = true;
            }
        }

        // WRITE 0 RATE TO NVS NOW BOLUS IS COMPLETE
        nvs_set_i32(bo_handle, "bolus_size", 0);
        nvs_commit(bo_handle);

        // RESET BOLUS FLAGS
        bolus_ready = false;

        // LOOP PACING 
        vTaskDelay( pdMS_TO_TICKS(INSRATE_BOLUS_LOOP_DELAY) );
        
        // 
        puts("bolus_delivery_end");
    } 
}

/*
 * Description
 */
void task_INSRATE_rewindPlunger ( void *arg )
{
    while (1)
    {
        // WHY SO MANY PUTS?? -------------------------------------------------------------------------------
        puts("rewind_plunge begin");

        // REWIND FLAG
        if ( RW_flag == true ) 
        {
            // MOVE PLUNGER BACK
            MOTOR_stepX( false, STEPS_PER_UNIT*2 );    
            // UPDATE POT POSITION DATA
            ADC_updatePot();
            // CHECK FOR SYRINGE RESET CONDITION POSITION
            if ( ADC_getPotPosition() <= 0 ) {
                RW_flag = false;
            }
        }

        // LOOP PACING
        vTaskDelay(pdMS_TO_TICKS(INSRATE_REWIND_LOOP_DELAY));
        
        //
        puts("rewind_plunge end");
    }
    
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Description
 */
void INSRATE_sliceString ( const char *data )
{
    // INITIALISE FUNCTION VARIABLES
    uint8_t index = 0;

    // RESET ARRAY BEFORE USAGE
    memset(index_arr, 0, sizeof(index_arr));
    
    // ITTERATE THROUGH DATA ARRAY
    for( uint8_t i = 0; (i < strlen(data)); i++ )
    {
        // ? //-------------------------------------------------------------WHAT ARE YOU DOING HERE
        if ( data[i] == 42 )
        {
            index_arr[index] = i;
            index++;
        }
    }
}

/*
 * INITIALISE NVS PARTITION TO STORE DATA
 */
void INSRATE_initStoragePartition ( void )
{
    nvs_flash_init_partition("rate_storage");
}

/*
 * PROCESS NEW BASAL DATA AND WRITE TO NVS
 */
void INSRATE_writeData_basalRate ( int basal )
{
    // INITIALISE FUNCTION VARIABLES
    nvs_handle_t handle;
    // OPEN 'BASAL RATE' FROM INSULIN DELIVERY PARTITION 
    nvs_open_from_partition( "rate_storage", "basal_rate", NVS_READWRITE, &handle );

    // PROCESS DATA AND CHECK
    basal = INSRATE_truncateAndCheck(basal);

    // ASSIGN NEW BASAL RATE TO KEY AND COMMIT TO NVS
    nvs_set_i32(handle, "basal_rate", basal);
    nvs_commit(handle);
}

/*
 * PROCESS BOLUS AMOUNT AND WRITE TO NVS
 */
void INSRATE_writeData_bolus ( int bolus )
{
    // INITIALISE FUNCTION VARIABLES   
    nvs_handle_t handle;
    // OPEN 'BOLUS SIZE' FROM INSULIN DELIVERY PARTITION 
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &handle);

    // PROCESS DATA AND CHECK
    bolus = INSRATE_truncateAndCheck(bolus);

    // ASSIGN BOLUS SIZE TO KEY AND COMMIT TO NVS
    nvs_set_i32(handle, "bolus_size", bolus);
    nvs_commit(handle);
}

/*
 * PROCESS REWIND AMOUNT AND WRITE TO NVS
 */
void INSRATE_writeData_rewind ( int rewind )
{
    // INITIALISE FUNCTION VARIABLES
    nvs_handle_t handle;
    // OPEN 'REWI' FROM INSULIN DELIVERY PARTITION 
    nvs_open_from_partition("rate_storage", "rewi", NVS_READWRITE, &handle);

    // PROCESS DATA AND CHECK
    rewind = INSRATE_truncateAndCheck(rewind);

    // ASSIGN REWIND TO KEY AND COMMIT TO NVS
    nvs_set_i32(handle, "basal_rate", rewind);
    nvs_commit(handle);
}

/*
 * TRUNCATE RATE TO A MULTIPLE OF 0.025U AND CHECK FOR A 'ZERO' RATE
 */
int INSRATE_truncateAndCheck ( int rate )
{
    // TRRUNCATE TO 0.025U
    rate = (rate / MIN_DELIVERY_SIZE) * MIN_DELIVERY_SIZE;
    // CHECK IF ACCIDENTALLY PUT A ZERO RATE
    if ( rate <= 0 ) {
        LED_flashFive_double(); // ---------------------------------------------------------- NO ABORT IF INCORRECT BOLUS GIVEN?
    }
    // RETURN
    return rate;
}

// /*
//  * Description
//  */
// int INSRATE_setDeliveryFrequencyTest ( int freq )
// {
//     return freq;
// }

/*
 * Function to slice a string and find the index of two asterisks contained within it
 */
bool INSRATE_checkBolusCancelled ( void ) 
{
    // INITIALISE FUNCTION VARIABLES
    int32_t bolus_size = 0;
    nvs_handle_t bo_handle;
    bool cancelled = false;

    // INITIALISE PARTITION AND EXTRACT BOLUS DATA
    nvs_flash_init_partition("rate_storage");
    nvs_open_from_partition("rate_storage", "bolus_size", NVS_READWRITE, &bo_handle);   
    nvs_get_i32(bo_handle, "bolus_size", &bolus_size);

    // CHECK IF BOLUS 
    if ( bolus_size == 0 ) // -------------------------------------------------------------------- WHY REWRITE 0 IF ALREADY 0
    {
        cancelled = true;
        nvs_set_i32(bo_handle, "bolus_size", 0);
        nvs_commit(bo_handle);
    }

    // RETURN
    return cancelled;
}

/*
 * EXTRACTS BASAL INFO FROM NVS AND CALCULATED DELIVERY FREQUENCY
 */
int INSRATE_calculateDeliveryFrequency ( void )
{
    // INITIALISE FUNCTION VARIABLES
    int32_t basal_rate = 0;
    int dels_per_hour = 0;
    int dels_freq = 0;
    nvs_handle_t br_handle;

    // INITIALISE PARTITION AND EXTRACT BASAL RATE DELIVERY DATA
    nvs_flash_init_partition("rate_storage");
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &br_handle);
    nvs_get_i32(br_handle, "basal_rate", &basal_rate);

    // CALCULATE REQUIRED DELIVERYS PER HOUR
    dels_per_hour = basal_rate / MIN_DELIVERY_SIZE;

    // CHECK DELIVERY LIMITS
    if ( dels_per_hour > DELIVERY_PER_HR_MAX )
    {
        dels_per_hour = DELIVERY_PER_HR_MAX;
    }

    // CALCULATE DELIVERY FREQUENCY
    if ( dels_per_hour <= 0 ) {
        dels_freq = 0;
    } else {
        dels_freq = SECONDS_IN_AN_HOUR/dels_per_hour; //seconds between doses = dels_freq
    }

    // SAVE DELIVERY INFO TO ARRAY
    basal_info_array[0] = dels_per_hour;
    basal_info_array[1] = basal_rate;

    // RETURN
    return dels_freq;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */