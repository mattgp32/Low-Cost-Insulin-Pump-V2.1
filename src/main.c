#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "motor.h"
#include "ble_comp_test.h"

#define TEST_PIN GPIO_NUM_45
#define LED GPIO_NUM_1

void app_main() 
{
    init_motor();
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, true);
    printf("Motor driver pins initilaised!\n");

    while(1)
    {
        turn_x_steps(true, 100);
        printf("Motor Turned\n");
        gpio_set_level(LED, false);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        gpio_set_level(LED, true);
    }
}