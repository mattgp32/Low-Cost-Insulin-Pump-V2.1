/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "button.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG                     "BUTTON"
#define BUTTON_GPIO             GPIO_NUM_5

#define ESP_INTR_FLAG_DEFAULT   0

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void task_BUTTON_handler    ( void * );

void BUTTON_pressedISR      ( void * );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool button_pressed = false;
bool button_pressedFlag = false;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Initialise Everything For Button Module Functionality 
 */
void BUTTON_init ( void )
{
    // LOG
    ESP_LOGI(TAG, "Initialising Button Module");

    // INITIALISE BUTTON GPIO
    gpio_reset_pin( BUTTON_GPIO );
    gpio_set_direction( BUTTON_GPIO, GPIO_MODE_INPUT );
    gpio_set_pull_mode( BUTTON_GPIO, GPIO_FLOATING ); // ------------------------------------ SHOULD HAVE A PULL SO DONT GET FALSE TRIGGERS

    // SETUP BUTTON INTERRUPT
    gpio_set_intr_type( BUTTON_GPIO, GPIO_INTR_ANYEDGE );
    gpio_intr_enable( BUTTON_GPIO );
    gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT );
    gpio_isr_handler_add( BUTTON_GPIO, BUTTON_pressedISR, NULL );
}

/*
 * Start The RTOS Button Handler Task
 */
void BUTTON_start ( void )
{
    xTaskCreate(task_BUTTON_handler,"print num", 4092, NULL, 4, NULL);
}

/*
 * Returns The Button Pressed Flag 
 * 
 * Has the Button Been Pressed And Passed Debouncing Criteria
 */
bool BUTTON_getPressedFlag ( void )
{
    ESP_LOGI(TAG, "Button Pressed Flag Requested");
    return button_pressedFlag;
}

/*
 * Resets The Button Pressed Flag
 */
void BUTTON_resetPressedFlag ( void )
{
    ESP_LOGI(TAG, "Button Pressed Flag Reset");
    button_pressedFlag = false;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * RTOS Task To Debounce The Button And Handle Associated Logic
 */
void task_BUTTON_handler ( void *args )
{
    // LOG
    ESP_LOGI(TAG, "Initialising LED Module");
    
    // LOOP TO INFINITY AND BEYOND
    while(1)
    {
        // INITIALISE LOOP VARIABLES 
        uint8_t butt_read = 0;

        // CHECK FOR A REGISTERED BUTTON PRESS 
        if ( button_pressed )
        {
            // LOG
            ESP_LOGI(TAG, "Button Press Event Detected");
            
            // CHECK IF BUTTON IS DEPRESSED 10 TIMES INCREMENT COUNTER
            for ( int i = 0; i < 10; i++ ) {
                if( gpio_get_level(BUTTON_GPIO) == true ) {
                    // INCREMENT COUNTER
                    butt_read++;
                    vTaskDelay( pdMS_TO_TICKS(10) ); // ------------------------------------ SHOULD THIS NOT BE MOVED DOWN 1 LEVEL TO DELAY ON A FALSE?
                }
            }
            // I
            if ( butt_read > 5 ) {
                ESP_LOGI(TAG, "Button Event Passed Debounce Test");
                ESP_LOGI(TAG, "Escalating Button Event To Global Flag");
                button_pressedFlag = true;
            }

            // RESET BUTTON FLAGS
            button_pressed = false;
        }

        // DELAY?
        vTaskDelay(pdMS_TO_TICKS(200)); //------------------------------------ is this loop pacing
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

/*
 * ISR When The Bluetooth Button Is Pressed
 */
void IRAM_ATTR BUTTON_pressedISR ( void *args )
{
    button_pressed = true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */