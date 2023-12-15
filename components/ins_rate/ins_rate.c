/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ins_rate.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG                         "INSRATE"

#define STEPS_PER_UNIT              770         // Altered based on testing
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

void INSRATE_start_deliverBolus         ( void );
void INSRATE_start_rewindPlunger        ( void );
void INSRATE_start_primePlunger         ( void );

void INSRATE_sliceString                ( const char * );
void INSRATE_writeData_basalRate        ( int );
void INSRATE_writeData_bolus            ( int );
void INSRATE_calculateBasalFrequency    ( uint32_t *, uint32_t *, TickType_t * );

void task_INSRATE_deliverBasal          ( void * );
void task_INSRATE_deliverBolus          ( void * );
void task_INSRATE_rewindPlunger         ( void * );
void task_INSRATE_primePlunger          ( void * );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t basalRate = 0;
int32_t bolusSize = 0;
int32_t primeSize = 0;
bool    rewindPlunger = false;

uint8_t index_arr[2] = {0};
time_t unix_modifier = 0;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Initialise Everything For Insulin Rate Module Functionality
 */
void INSRATE_init ( void )
{
    // Log Initialisation Of Module
    ESP_LOGI(TAG, "Initialising Insulin Rate Module");
    // Initialise Function Variables   
    nvs_handle_t handle;
    // Initialise Rate Storage NVS Partition
    nvs_flash_init_partition("rate_storage");
    // Retrieve Basal Rate From NVS
    nvs_open_from_partition("rate_storage", "basal_rate", NVS_READONLY, &handle);
    nvs_get_i32(handle, "basal_rate", &basalRate);
    // Log Basal Rate
    ESP_LOGI(TAG, "Retrieved Basal Rate From NVS = %ld", basalRate);
}

/*
 * Start the RTOS Insulin Rate Tasks
 */
void INSRATE_start ( void )
{
    xTaskCreate(task_INSRATE_deliverBasal, "start insulin deliveries", (4*1024), NULL, 20, NULL);
    xTaskCreate(task_INSRATE_deliverBolus, "deliver bolud", (4*1024), NULL, 19, NULL);
    xTaskCreate(task_INSRATE_rewindPlunger, "rewind plunger", (4*1024), NULL, 18, NULL);
    xTaskCreate(task_INSRATE_primePlunger, "prime plunger", (4*1024), NULL, 18, NULL);
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
        //
        LOGGING_append( recieve_time, unix_modifier );
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
        ESP_LOGI(TAG, "Recieved Update Basal Rate Command (%d Units)", dataInsulin_i); 
        // INITIALISE PARTITION AND WRITE BASIL RATE
        INSRATE_writeData_basalRate(dataInsulin_i);
        //
        LOGGING_append( recieve_basal, dataInsulin_i );
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
        ESP_LOGI(TAG, "Recieved Deliver Bolus Command (%d Units)", dataInsulin_i); 
        // INITIALISE PARTITION AND WRITE BOLUS
        INSRATE_writeData_bolus(dataInsulin_i);
        //
        LOGGING_append( recieve_bolus, dataInsulin_i );
    } 

    // DATATYPE: MOTOR REWIND
    else if ( strcmp(data_type, "RE") == 0 ) 
    {
        // LOG REWIND 
        ESP_LOGI(TAG, "Recieved Plunger Rewind Command"); 
        // START TASK TO REWIND PLUNGER
        rewindPlunger = true;
        //
        LOGGING_append( recieve_rewind, 0 );
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
        ESP_LOGI(TAG, "Recieved Plunger Prime Command (%d Units)", (int)dataInsulin); 
        //
        primeSize = dataInsulin;
        //
        LOGGING_append( recieve_prime, dataInsulin_i );
    } 

    // DATATYPE: UNKNOWN
    else {
        ESP_LOGI(TAG, "You have entered an invalid type"); 
    }
}

/*
 * Retrieves The Current Basal Rate
 */
int32_t INSRATE_getBasalRate ( void )
{ 
    return basalRate;
}

/*
 * Returns True If The Basal Rate Is Zero
 */
bool INSRATE_zeroBasalRate ( void )
{ 
    return (basalRate == 0);
}

/*
 * Returns True If The Bolus Delivery Task Is Active
 */
bool INSRATE_deliveringBolus ( void )
{ 
    return ( bolusSize != 0 );
}

/*
 * Returns True If The Plunger/Motor Rewind Task Is Active
 */
bool INSRATE_rewindingPlunger ( void )
{ 
    return rewindPlunger;
}


/*
 * Returns True If The Plunger Prime Task Is Active
 */
bool INSRATE_primingPlunger ( void )
{ 
    return ( primeSize != 0 );
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Handle Delivery of Basal Insulin
 */
void task_INSRATE_deliverBasal ( void *arg )
{
    // Log Start Of Task
    ESP_LOGI(TAG, "Opening Basal Insulin Delivery Handler Task");

    // Initialise Function Variables
    TickType_t  xLastWakeTime = xTaskGetTickCount();
    uint32_t    working_basalRate = 0;
    uint32_t    working_basalDelPerHour = 0;
    TickType_t  working_basalFrequency = 0;

    // Start Delay
    vTaskDelay( pdMS_TO_TICKS(10*1000) );

    // Loop to Infinity And Beyond
    while (1)
    {
        // Check For a New Basal Rate Waiting To Be Implemented
        if ( working_basalRate != basalRate ) 
        {
            // Move Temp Basal To Working Basal Rate
            working_basalRate = basalRate;
            ESP_LOGI(TAG, "New Basal Rate Applied To Working Rate (%ld)", working_basalRate );
            // Calculate Delivery Stats
            INSRATE_calculateBasalFrequency( &working_basalRate, &working_basalDelPerHour, &working_basalFrequency );
            ESP_LOGI(TAG, "Calculated New Basal Delivery Frequency = %ld", working_basalFrequency);
            ESP_LOGI(TAG, "Calculated New Delivery Per Hour = %ld", working_basalDelPerHour);
        }

        // 'ZERO' RATE FREQUENCY
        if ( working_basalFrequency <= 0 ) 
        {
            // Log Rate Error
            ESP_LOGI(TAG, "Delivery Frequency = 0. Waiting For Update");
            // Loop Pacing
            vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( THREE_MINUTES*SECONDS_TO_MS ) ); // Three minutes is the fastest delivery time
        }

        // NORMAL OPERATION  
        else 
        {
            // Check If Motor Is Being Used By Another Module
            if ( !MOTOR_avalibleForControl() ) 
            {
                // Log Busy Wait For Motor Control
                ESP_LOGI(TAG, "Motor Unavaliable To Deliver Basal, Wait Until Rescource Becomes Avaliable");
                // Wait For Motor Control To Become Avalialbe
                while ( !MOTOR_avalibleForControl() ) { vTaskDelay( pdMS_TO_TICKS(10) ); }
            }
            // Take Control of Motor 
            MOTOR_takeControl();
            // Log Delivery Of Insulin
            ESP_LOGI(TAG, "Delivering Next Basal Dose");
            //
            LOGGING_append( deliver_basal, working_basalRate );
            // Step Motor
            MOTOR_stepX( true, (int)(STEPS_PER_UNIT * working_basalRate) / (working_basalDelPerHour*1000) );
            // Release Control of Motor
            MOTOR_releaseControl();
            // Loop Pacing
            vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(working_basalFrequency) );
        }
    }
}

/*
 * Handle Delivery of Bolus Insulin
 */
void task_INSRATE_deliverBolus ( void *arg )
{
    // Log Start of Task
    ESP_LOGI(TAG, "Opening Bolus Insulin Delivery Handler Task");
    // UPDATE LOOP VARIABLES
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if ( bolusSize ) 
        {

            // Initialise Function Variables
            uint32_t working_bolusSize = bolusSize;

            // 'ZERO' SIZE BOLUS
            if ( (working_bolusSize / MIN_DELIVERY_SIZE) == 0 )
            {
                // Log Size Error
                ESP_LOGI(TAG, "Requested Bolus = 0");
                // Notify User
                LED_flashFive_double();
            } 

            // NORMAL OPERATION
            else 
            {
                // Check If Motor Is Being Used By Other Tasks
                if ( !MOTOR_avalibleForControl() ) 
                {
                    // Log Busy Wait For Motor Control
                    ESP_LOGI(TAG, "Motor Unavaliable To Deliver Bolus, Wait Until Rescource Becomes Avaliable");
                    // Wait For Motor Control To Become Avaliable
                    while ( !MOTOR_avalibleForControl() ) { vTaskDelay( pdMS_TO_TICKS(100) ); }
                }
                // Take Control Of Motor 
                MOTOR_takeControl();
                // Calculate Required Number Of Doses To Reach Total Bolus
                uint32_t working_numDoses = working_bolusSize / MIN_BOLUS_DELIVERY_SIZE;
                // Log Bolus Information
                ESP_LOGI(TAG, "Requested Bolus = %ld (Delivering in %ld Doses of 0.05U)", working_bolusSize, working_numDoses);
                ESP_LOGI(TAG, "Starting Bolus Delivery");
                //
                LOGGING_append( deliver_bolus, working_bolusSize );
                // Deliver Insulin
                for ( uint8_t i = 1; i <= working_numDoses; i++ )
                {
                    // Detect Bolus Cancelled
                    if ( bolusSize == 0 )
                    {
                        // Log Cancellation Of Task 
                        ESP_LOGI(TAG, "Bolus Cancelled. Users Cancelled Via Phone App");
                        ESP_LOGI(TAG, "Delivered %d / %ld Doses", (i-1), working_numDoses);
                        // Wipe Working Bolus Size
                        working_bolusSize = 0;
                        // Break From Delivery Loop
                        break;
                    }

                    // Detect Bolus Size Changed
                    else if ( bolusSize != working_bolusSize )
                    {
                        // Calculate Required Number Of Doses To Reach Total Bolus
                        uint32_t temp_bolusSize = bolusSize;
                        uint32_t temp_numDoses = temp_bolusSize / MIN_BOLUS_DELIVERY_SIZE;
                        // Log Change In Bolus Size
                        ESP_LOGI(TAG, "Bolus Size Changed. Original Bolus = %ld, New Bolus = %ld", working_bolusSize, bolusSize);
                        ESP_LOGI(TAG, "New Bolus Requires %ld Dose(s). Delivered %d So Far", temp_numDoses, (i-1));
                        // Have We delivered Less (Or Currently On) The Number Of Doses To Meet The New Bolus 
                        if ( i <= temp_numDoses ) 
                        {
                            ESP_LOGI(TAG, "Updating Bolus Information With New Bolus And Continuing Delivery");
                            // Update Working Variables 
                            working_bolusSize = temp_bolusSize;
                            working_numDoses = temp_numDoses;
                        } 
                        // Have Already Delivered More Doses Than New Bolus Size
                        else 
                        {
                            ESP_LOGI(TAG, "Bolus Cancelled. Already Delivered More Doses Than New Bolus Requires");
                            // Wipe Working Bolus Size
                            working_bolusSize = 0;
                            // Break From Delivery Loop
                            break;
                        }
                    }

                    // Hit End Of Potentiometer
                    else if ( ADC_potAtMax() )
                    {
                        // Log Cancellation Of Task
                        ESP_LOGI(TAG, "Bolus Cancelled, Potentiometer At End Of Range");
                        ESP_LOGI(TAG, "Delivered %d / %ld Doses", (i-1), working_numDoses);               
                        // Wipe Working Bolus Size
                        working_bolusSize = 0;
                        // Break From Delivery Loop
                        break;
                    }

                    // Log Insulin Delivery
                    ESP_LOGI(TAG, "Delivering Bolus Dose %d / %ld", i, working_numDoses);
                    // Step Motor 
                    MOTOR_stepX( true, MIN_BOLUS_DELIVERY_STEPS );
                    // Loop Pacing
                    vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(1200));
                }

                // Was Bolus Cancelled - Only Time This Will Be 0 At This Stage Is If Bolus Was Cancelled
                if ( working_bolusSize ) 
                {
                    // Check If A 0.025U Dose Is Needed To Meetin Total Bolus Requirement
                    if ( (working_bolusSize % MIN_BOLUS_DELIVERY_SIZE) == MIN_DELIVERY_SIZE ) 
                    {
                        // Log Insulin Delivery
                        ESP_LOGI(TAG, "Delivering additional dose of 0.025U to meet requested bolus size of %ld", working_bolusSize);
                        // Step Motor
                        MOTOR_stepX(true, MIN_DELIVERY_STEPS);
                    }
                    // Log Completion Of Delivery
                    ESP_LOGI(TAG, "Finishing Bolus Delivery");
                }
                
                // Release Control Of Motor
                MOTOR_releaseControl();
            }
            //
            bolusSize = 0;
        }
        // Loop Pacing
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(250) );
    }
}

/*
 * RTOS Task To Handle Rewind Of Motor/Plunger
 */
void task_INSRATE_rewindPlunger ( void *arg )
{
    // Log Start Of Rewind Task 
    ESP_LOGI(TAG, "Opening Plunger Rewind Handler Task");
    // UPDATE LOOP VARIABLES
    TickType_t xLastWakeTime = xTaskGetTickCount();
    //
    while (1) 
    {
        //
        if ( rewindPlunger) 
        {
            // Initialise Loop Variables
            uint8_t rewindLoops = 0; 
            // Check If Motor Is Being Used By Other Tasks
            if ( !MOTOR_avalibleForControl() ) 
            {
                // Log Busy Wait For Motor Control
                ESP_LOGI(TAG, "Motor Unavaliable To Rewind Plunger, Wait Until Rescource Becomes Avaliable");
                // Wait For Motor Control To Become Avaliable
                while ( !MOTOR_avalibleForControl() ) { vTaskDelay( pdMS_TO_TICKS(100) ); }
            }
            // Take Control Of Motor 
            MOTOR_takeControl();
            // Log Rewind Of Plunger
            ESP_LOGI(TAG, "Plunger Rewind Starting");
            //
            LOGGING_append( plunger_rewindStart, 0 );
            // Loop Until Pot Is In Reset Position
            while ( !ADC_potReset() )
            {
                // Increment Motor Move Counter
                rewindLoops += 1;
                // Love Plunger Move 
                ESP_LOGI(TAG, "Resetting Plunger, Units Reset = %d", rewindLoops);
                // Step Motor In Reverse 1 Unit
                MOTOR_stepX( MOTOR_RVS, STEPS_PER_UNIT );    
            }
            // Release Control Of Motor
            MOTOR_releaseControl();
            //
            rewindPlunger = false;
            // Log Completion Of Task
            ESP_LOGI(TAG, "Plunger Rewind Complete");
            //
            LOGGING_append( plunger_rewindFinish, 0 );
        }
        // Loop Pacing
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(250) );
    }
}

/*
 *
 */
void task_INSRATE_primePlunger ( void *arg )
{
    // Log Start Of Priming Task 
    ESP_LOGI(TAG, "Opening Prime Plunger Handler Task");
    // UPDATE LOOP VARIABLES
    TickType_t xLastWakeTime = xTaskGetTickCount();
    // 
    while (1)
    {
        if ( primeSize )
        {    
            // Initialise Function Variables
            uint32_t working_primeSize = primeSize;
            // Check If Motor Is Being Used By Other Tasks
            if ( !MOTOR_avalibleForControl() ) 
            {
                // Log Busy Wait For Motor Control
                ESP_LOGI(TAG, "Motor Unavaliable To Prime Plunger, Wait Until Rescource Becomes Avaliable");
                // Wait For Motor Control To Become Avaliable
                while ( !MOTOR_avalibleForControl() ) { vTaskDelay( pdMS_TO_TICKS(100) ); }
            }
            // Take Control Of Motor
            MOTOR_takeControl();
            // LOG
            ESP_LOGI(TAG, "Requested Prime = %ld Units", working_primeSize);
            ESP_LOGI(TAG, "Priming Plunger Starting");
            //
            LOGGING_append( plunger_prime, working_primeSize );
            // Deliver Insulin
            for ( uint8_t i = 1; i <= working_primeSize; i++ )
            {
                // Hit End Of Potentiometer
                if ( ADC_potAtMax() )
                {
                    // Log Cancellation Of Task
                    ESP_LOGI(TAG, "Prime Cancelled, Potentiometer At End Of Range");
                    ESP_LOGI(TAG, "Delivered %d / %ld Doses", (i-1), working_primeSize);               
                    // Wipe Working Bolus Size
                    working_primeSize = 0;
                    // Break From Delivery Loop
                    break;
                }
                // Log Plunger Move 
                ESP_LOGI(TAG, "Priming Plunger Units %d / %ld", i, working_primeSize);
                // Step Motor In Reverse 1 Unit
                MOTOR_stepX( MOTOR_FWD, STEPS_PER_UNIT );    
            }
            // Release Control Of Motor
            MOTOR_releaseControl();
            //
            primeSize = 0;
            // Log Completion Of Task
            ESP_LOGI(TAG, "Priming Plunger Complete");
        }
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(250) );
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
    basalRate = basal;
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
    bolusSize = bolus;
}

/*
 * Takes A Basal Rate And Cakculates The Delivereid Per Hour and Delivery Frequency
 */
void INSRATE_calculateBasalFrequency ( uint32_t *basal, uint32_t *dels, TickType_t *freq )
{
    // Initialise Function Variables
    int temp_dels = 0;
    int temp_freq = 0;

    // Calculate deliveries per hour
    temp_dels = *basal / MIN_DELIVERY_SIZE;

    // Check Delivery Limits
    if ( temp_dels > DELIVERY_PER_HR_MAX )
    {
        ESP_LOGI(TAG, "Calculated Deliveries Per Hour (%d) > Max Allowable (%d). So truncating to Max", temp_dels, DELIVERY_PER_HR_MAX);
        temp_dels = DELIVERY_PER_HR_MAX;
    }

    // Calculate Delivery Frequency
    if ( temp_dels <= 0 ) {
        temp_freq = 0;
    } else {
        temp_freq = (SECONDS_IN_AN_HOUR/temp_dels)* SECONDS_TO_MS;
    }

    // Assign Local Temp Variables To External Working Variables
    *dels = temp_dels;
    *freq = temp_freq;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */