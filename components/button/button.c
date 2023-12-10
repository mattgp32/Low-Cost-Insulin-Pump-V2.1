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

void BUTTON_pressedISR      ( void * );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool button_pressed = false;

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
 * Returns The Button Pressed Flag 
 * 
 * Has the Button Been Pressed And Passed Debouncing Criteria
 */
bool BUTTON_getPressedFlag ( void )
{
    return button_pressed;
}

/*
 * Resets The Button Pressed Flag
 */
void BUTTON_resetPressedFlag ( void )
{
    button_pressed = false;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
    // INITIALISE FUNCTION VARIABLES
    static TickType_t pressed = 0;
    
    // BUTTON PRESSED
    if ( !gpio_get_level(BUTTON_GPIO) ) 
    { 
        pressed = xTaskGetTickCountFromISR();
    }
    // BUTTON RELEASED
    else
    { 
        TickType_t time = xTaskGetTickCountFromISR() - pressed;
        if ( (time >= 5) && (time <= 100) )
        {
            button_pressed = true;
        }
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */