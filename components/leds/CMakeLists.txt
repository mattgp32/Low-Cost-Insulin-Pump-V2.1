idf_component_register(
    SRCS            "leds.c"
    INCLUDE_DIRS    .
    REQUIRES        driver adc motor nvs_flash pump_BT
)