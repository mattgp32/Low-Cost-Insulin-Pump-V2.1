#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" 
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "motor.h"
#include "ble_comp_test.h"
#include "adc.h"
#include "leds.h"
#include "stdio.h"
#include "pump_BT.h"
#include <assert.h>
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_err.h"
#include "esp_pm.h"
#include "esp_log.h"
#include "ins_rate.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "driver/gptimer.h"
#include "esp_pm.h"
#include "driver/gpio.H"
#include "esp_private/esp_clk.h"


#define POT_POWER GPIO_NUM_37
#define BUTTON_PIN GPIO_NUM_5
#define ESP_INTR_FLAG_DEFAULT 0

int button_pressed = 0;
bool BT_already_on = true;
bool switch_on = false;
void IRAM_ATTR button_isr(void* args)
{
    button_pressed += 1;
}

void init_button()
{
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_FLOATING);
}

void init_isr()
{
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_ANYEDGE);
    gpio_intr_enable(BUTTON_PIN);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, NULL);
}

void print_num(void*args)
{
    for(;;){

        uint8_t butt_read = 0;

        if(button_pressed!= 0)
        {
            for (int i = 0; i<10; i++)
            {
                if(gpio_get_level(BUTTON_PIN) == true)
                {
                    butt_read++;
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }

        if (butt_read > 5)
        {
            puts("Button Pressed");
            switch_on = true;
            butt_read = 0;
            button_pressed = 0;
        } else {
            butt_read = 0;
            button_pressed = 0;
        }
    vTaskDelay(pdMS_TO_TICKS(200));
    } 
}

void app_main(void)
{
//Initialise system peripherals to be used after freeRTOS starts
    esp_pm_config_esp32s3_t pm_config = {
            .max_freq_mhz = 80,
            .min_freq_mhz = 40,
           .light_sleep_enable = false,
    };
    ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
    // small delay might be necessary for the frequency setting to take effect — the idle task should have a chance to run
    vTaskDelay(pdMS_TO_TICKS(10));
    // now the frequency should be 40 MHz
    assert(esp_clk_cpu_freq() == 80 * 1000000);

    init_leds();
    led_wave();
    //buzzer_init();
    run_BT();
    init_motor();

    init_button();
    init_isr();

    // Create all tasks for the freeRTOS scheduler
    xTaskCreate(get_batt_level, "Read ADC and write batt level to a queue", 1024, NULL, 5, NULL);
    xTaskCreate(display_batt_level, "Blink LED depending on batt level", 8192, NULL, 5, NULL);
    xTaskCreate(receive_BT_data, "get data from bt buffer",8192, NULL, 10, NULL);
    xTaskCreate(process_bt_data, "print data from bt buffer",8192, NULL, 10, NULL);
    xTaskCreate(retreive_data, "Display rate data - for debugging only", 8192, NULL, 5, NULL);
    xTaskCreate(no_br_warning, "flash led if br = 0", 2048, NULL, 5, NULL);
    xTaskCreate(give_insulin, "start insulin deliveries", 4096, NULL, 20, NULL);
    xTaskCreate(bolus_delivery, "give bolus", 4092, NULL, 21, NULL);
    xTaskCreate(read_pot, "potentimoeter read", 4092, NULL, 5, NULL);
    xTaskCreate(rewind_plunge, "rewind motor if flag set", 4092, NULL, 4, NULL);
    xTaskCreate(print_num,"print num", 4092, NULL, 4, NULL);
    xTaskCreate(BT_off, "turn off BT", 4092, NULL, 4, NULL);
    xTaskCreate(BT_Control_Task, "BT_Control_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BT_running_alert, "flash_led_when BT active", 2048, NULL, 1, NULL);
    //xTaskCreate(begin_low_power, "enter sleep mode", 2048 , NULL, 19,NULL);
    //install gpio isr service
}