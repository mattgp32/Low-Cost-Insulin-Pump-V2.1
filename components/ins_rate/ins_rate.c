/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ins_rate.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG                         "INSRATE"

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
#define INSRATE_BOLUS_LOOP_DELAY    250
#define INSRATE_REWIND_LOOP_DELAY   1000

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void INSRATE_sliceString                ( const char * );
void INSRATE_writeData_basalRate        ( int );
void INSRATE_writeData_bolus            ( int );
void INSRATE_calculateBasalFrequency    ( void );

void task_INSRATE_deliverBasal          ( void * );
void task_INSRATE_deliverBolus          ( void * );
void task_INSRATE_rewindPlunger         ( void * );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool        motorAvaliable = true;

int32_t     basal_rate = 0;
int32_t     basal_rateTemp = 0;
bool        basal_rateNew = false;
int32_t     basal_delPerHour = 0;
TickType_t  basal_frequency = 0;

int32_t     bolus_size = 0;
bool        bolus_new = false;

uint8_t index_arr[2] = {0};
time_t unix_modifier = 0;

bool bolus_ready = false;
bool RW_flag = false;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Initialise Everything For Insulin Rate Module Functionality
 */
void INSRATE_init ( void )
{
    // LOG
    ESP_LOGI(TAG, "Initialising Insulin Rate Module");

    // INITIALISE FUNCTION VARIABLES   
    nvs_handle_t handle;

    // INITIALISE RATE STORAGE NVS PARTITION
    nvs_flash_init_partition("rate_storage");

    // RETRIEVE BASAL RATE FROM NVS
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &handle);
    nvs_get_i32(handle, "basal_rate", &basal_rate);
    ESP_LOGI(TAG, "Retrieved Basal Rate From NVS = %ld", basal_rate);

    // CALCULATE DELIVERY STATS
    INSRATE_calculateBasalFrequency();
    ESP_LOGI(TAG, "Calculated Basal Delivery Frequency = %ld", basal_frequency);
    ESP_LOGI(TAG, "Calculated Delivery Per Hour = %ld", basal_delPerHour);
}

/*
 * Start the RTOS Insulin Rate Tasks
 */
void INSRATE_start ( void )
{
    xTaskCreate(task_INSRATE_deliverBasal, "start insulin deliveries", 4096, NULL, 21, NULL);
    xTaskCreate(task_INSRATE_deliverBolus, "give bolus", 4092, NULL, 20, NULL);
    // xTaskCreate(task_INSRATE_rewindPlunger, "rewind motor if flag set", 4092, NULL, 4, NULL);
}

/*
 * Take-In Process And Save New Data
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
    //
    ESP_LOGI(TAG, "Data type to recieve and store: %s", data_type);

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
        //
        ESP_LOGI(TAG, "Extracting UNIX Time"); 
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
        // LOG BASAL RATE
        ESP_LOGI(TAG, "Extracted Basal Rate: %d", dataInsulin_i); 
        // INITIALISE PARTITION AND WRITE BASIL RATE
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
        // LOG BOLUS SIZE
        ESP_LOGI(TAG, "Extracted Bolus Size: %d", dataInsulin_i); 
        // INITIALISE PARTITION AND WRITE BOLUS
        INSRATE_writeData_bolus(dataInsulin_i);
    } 

    // DATATYPE: MOTOR REWIND
    else if ( strcmp(data_type, "RE") == 0 ) 
    {
        //
        ESP_LOGI(TAG, "Setting Rewind Flag"); 
        //
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
        
        //
        ESP_LOGI(TAG, "Extracted Plunger Prime Amount: %d [Units]", (int)dataInsulin); 
        ESP_LOGI(TAG, "Starting Plunger Priming Process"); 

        // INITIATE SYRINGE PRIMING
        for(int i = 0; i < (int)dataInsulin; i++) 
        {
            ESP_LOGI(TAG, "Priming Unit %d / %d", i+1, (int)dataInsulin); 
            MOTOR_stepX(true, STEPS_PER_UNIT);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        //
        ESP_LOGI(TAG, "Finishing Plunger Priming Process"); 
    } 

    // DATATYPE: UNKNOWN
    else {
        ESP_LOGI(TAG, "You have entered an invalid type"); 
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Handle Delivery of Basal Insulin
 */
void task_INSRATE_deliverBasal ( void *arg )
{
    // LOG
    ESP_LOGI(TAG, "Starting Basal Insulin Delivery Handler Task");

    // LOOP TO INFINITY AND BEYOND
    while (1)
    {
        // CHECK FOR NEW BASAL RATE
        if ( basal_rateNew ) 
        {
            // MOVE NEW BASAL RATE INTO WORKING RATE 
            basal_rate = basal_rateTemp;
            ESP_LOGI(TAG, "New Basal Rate Applied To Working Rate (%ld)", basal_frequency);
            // CALCULATE DELIVERY STATS
            INSRATE_calculateBasalFrequency();
            ESP_LOGI(TAG, "Calculated New Basal Delivery Frequency = %ld", basal_frequency);
            ESP_LOGI(TAG, "Calculated New Delivery Per Hour = %ld", basal_delPerHour);
        }

        // CHECK FOR 'ZERO' RATE FREQUENCY
        if ( basal_frequency <= 0 ) 
        {
            ESP_LOGI(TAG, "Delivery Frequency = 0. Waiting For Update");
            // LOOP PACING
            vTaskDelay( pdMS_TO_TICKS( THREE_MINUTES*SECONDS_TO_MS ) ); // Three minutes is the fastest delivery time
        }
        // NORMAL OPERATION  
        else 
        {
            if ( !motorAvaliable ) {
                ESP_LOGI(TAG, "Motor Unavaliable To Deliver Basal, Wait Until Rescource Becomes Avaliable");
                // WAIT FOR MOTOR CONTROL TO BECOME AVALIABLE
                while ( !motorAvaliable ) { vTaskDelay( pdMS_TO_TICKS(10) ); }
            }
            // TAKE CONTROL OF MOTOR 
            motorAvaliable = false;
            // LOG DELIVERY OF INSULIN
            ESP_LOGI(TAG, "Delivering Next Basal Dose");
            // STEP MOTOR
            MOTOR_stepX( true, (int)(STEPS_PER_UNIT * basal_rate) / (basal_delPerHour*1000) );
            // READ IN SYRING POT POSITION
            ADC_updatePot(); // ---------------------------------------------------------------------------------------why are you reading pot? you arnt doing anything with it
            // RELEASE CONTROL OF MOTOR
            motorAvaliable = true;
            // LOOP PACING
            vTaskDelay( pdMS_TO_TICKS(basal_frequency) );
        }
    }
}

/*
 * Handle Delivery of Bolus Insulin
 */
void task_INSRATE_deliverBolus ( void *arg )
{
    // LOG 
    ESP_LOGI(TAG, "Starting Basal Insulin Delivery Handler Task");

    // LOOP TO INFINITY AND BEYOND
    while (1)
    {
        // CHECK FOR NEW BOLUS
        if ( bolus_new )
        {
            // RESET NEW BOLUS FLAG AND COPY BOLUS INFORMATION LOCALLY - DONE NOW TO DETECT CHANGE OR CANCELLATION 
            bolus_new = false;
            int32_t bolus_sizeLocal = bolus_size;

            // CHECK FOR 'ZERO' SIZE BOLUS
            if ( (bolus_sizeLocal / MIN_DELIVERY_SIZE) == 0 )
            {
                ESP_LOGI(TAG, "Requested Bolus = 0");
                LED_flashFive_double();
            } 

            // NORMAL OPERATION
            else 
            {
                // CHECK IF MOTOR BEING USED BUY OTHER TASKS
                if ( !motorAvaliable ) {
                    ESP_LOGI(TAG, "Motor Unavaliable To Deliver Bolus, Wait Until Rescource Becomes Avaliable");
                    // WAIT FOR MOTOR CONTROL TO BECOME AVALIABLE
                    while ( !motorAvaliable ) { vTaskDelay( pdMS_TO_TICKS(10) ); }
                }
                // TAKE CONTROL OF MOTOR 
                motorAvaliable = false;
                // CALCULATE REQUIRED DOSES TO REACH TOTAL BOLUS
                uint32_t numDoses = bolus_sizeLocal / MIN_BOLUS_DELIVERY_SIZE;
                // LOG BOLUS INFORMATION
                ESP_LOGI(TAG, "Requested Bolus = %ld (Delivering in %ld Doses of 0.05U)", bolus_sizeLocal, numDoses);
                ESP_LOGI(TAG, "Starting Bolus Delivery");
                // DELIVER INSULIN
                for ( uint8_t i = 0; i < numDoses; i++ )
                {
                    // DETECT CANCELLED OR CHANGED BOLUS SIZE
                    if ( bolus_new )
                    {
                        // LOG CANCELLATION
                        ESP_LOGI(TAG, "Bolus Cancelled. Delivered %d / %d Doses", i, n_steps);
                        // BREAK FROM DELIVERY LOOP
                        break;
                    }
                    // CONTINUE DELIVERING BOLUS
                    else
                    {
                        // LOG DELIVERY
                        ESP_LOGI(TAG, "Delivering Bolus Dose %d / %d", i+1, numDoses);
                        // DRIVE MOTOR
                        MOTOR_stepX( true, MIN_BOLUS_DELIVERY_STEPS );
                        // WAIT 
                        vTaskDelay(pdMS_TO_TICKS(1200));
                    }
                }

                // 
                if ( !bolus_new && (bolus_sizeLocal % MIN_BOLUS_DELIVERY_SIZE) == MIN_DELIVERY_SIZE) // ------------------------- why not just deliver all doses at 0.025U?
                {
                    // LOG DELIVERY
                    ESP_LOGI(TAG, "Delivering additional dose of 0.025U to meet requested bolus size of %ld", bolus_sizeLocal);
                    // DRIVE MOTOR
                    MOTOR_stepX(true, MIN_DELIVERY_STEPS);
                }

                // LOG COMPLETION OF BOLUS
                if ( bolus_new && bolus_size == 0 ) {
                    ESP_LOGI(TAG, "Bolus Value Updated - Starting Bolus Delivery");
                } else if () {
                    ESP_LOGI(TAG, "Bolus Value Updated - Starting Bolus Delivery");

                } else {
                    ESP_LOGI(TAG, "Finishing Bolus Delivery");
                }

                // READ IN SYRING POT POSITION
                ADC_updatePot(); // ---------------------------------------------------------------------------------------why are you reading pot? you arnt doing anything with it
                
                // RELEASE CONTROL OF MOTOR
                motorAvaliable = true;
            }
        }

        // LOOP PACING 
        vTaskDelay( pdMS_TO_TICKS(INSRATE_BOLUS_LOOP_DELAY) );
    } 
}

// /*
//  * Description
//  */
// void task_INSRATE_rewindPlunger ( void *arg )
// {
//     // LOG
//     ESP_LOGI(TAG, "Starting Rewind Plunger Task");

//     //
//     while (1)
//     {
//         // REWIND FLAG
//         if ( RW_flag == true ) 
//         {
//             // LOG
//             ESP_LOGI(TAG, "Plunger Rewind Task = True");
//             // MOVE PLUNGER BACK
//             MOTOR_stepX( false, STEPS_PER_UNIT*2 );    
//             // UPDATE POT POSITION DATA
//             ADC_updatePot();
//             // CHECK FOR SYRINGE RESET CONDITION POSITION
//             if ( ADC_getPotPosition() <= 0 ) {
//                 ESP_LOGI(TAG, "Plunger Rewind Task = Complete");
//                 RW_flag = false;
//             }
//         }

//         // LOOP PACING
//         vTaskDelay(pdMS_TO_TICKS(INSRATE_REWIND_LOOP_DELAY));
//     }
// }

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
        if ( data[i] == 42 )
        {
            index_arr[index] = i;
            index++;
        }
    }
}

/*
 * Process New Basal Rate Data
 */
void INSRATE_writeData_basalRate ( int basal )
{
    // INITIALISE FUNCTION VARIABLES
    nvs_handle_t handle;
    // OPEN 'BASAL RATE' FROM INSULIN DELIVERY PARTITION 
    nvs_flash_init_partition("rate_storage"); // ------------------------------------------------- do we need to init partition every time
    nvs_open_from_partition( "rate_storage", "basal_rate", NVS_READWRITE, &handle );

    // TRUNCATE BASAL RATE TO 0.025U
    basal = (basal / MIN_DELIVERY_SIZE) * MIN_DELIVERY_SIZE;
    // CHECK IF ACCIDENTALLY PUT A ZERO RATE
    if ( basal <= 0 ) {
        LED_flashFive_double();
    }

    // ASSIGN NEW BASAL RATE TO KEY AND COMMIT TO NVS
    nvs_set_i32(handle, "basal_rate", basal);
    nvs_commit(handle);
    ESP_LOGI(TAG, "New Basal Rate Saved to NVS = %d", basal);

    // SAVE BASAL RATE TO VARIABLE FOR USE AND TOGGLE FLAG
    basal_rateTemp = basal;
    basal_rateNew = true;
}

/*
 * Process New Bolus Data
 */
void INSRATE_writeData_bolus ( int bolus )
{
    // TRUNCATE BOLUS TO 0.025U
    bolus = (bolus / MIN_DELIVERY_SIZE) * MIN_DELIVERY_SIZE;
    // CHECK IF ACCIDENTALLY PUT A ZERO RATE
    if ( bolus <= 0 ) {
        LED_flashFive_double();
    }

    // SAVE BOLUS SIZE TO VARIABLE FOR USE AND TOGGLE FLAG
    bolus_size = bolus;
    bolus_new = true;
}

/*
 * EXTRACTS BASAL INFO FROM NVS AND CALCULATED DELIVERY FREQUENCY
 */
void INSRATE_calculateBasalFrequency ( void )
{
    // INITIALISE FUNCTION VARIABLES
    int dels_per_hour = 0;
    int dels_freq = 0;

    // CALCULATE REQUIRED DELIVERYS PER HOUR
    dels_per_hour = basal_rate / MIN_DELIVERY_SIZE;

    // CHECK DELIVERY LIMITS
    if ( dels_per_hour > DELIVERY_PER_HR_MAX )
    {
        ESP_LOGI(TAG, "Calculated Deliveries Per Hour (%d) > Max Allowable (%d). So truncating to Max", dels_per_hour, DELIVERY_PER_HR_MAX);
        dels_per_hour = DELIVERY_PER_HR_MAX;
    }

    // CALCULATE DELIVERY FREQUENCY
    if ( dels_per_hour <= 0 ) {
        dels_freq = 0;
    } else {
        dels_freq = SECONDS_IN_AN_HOUR/dels_per_hour; //seconds between doses = dels_freq
    }

    //
    basal_delPerHour = dels_per_hour;
    basal_frequency = dels_freq * SECONDS_TO_MS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */