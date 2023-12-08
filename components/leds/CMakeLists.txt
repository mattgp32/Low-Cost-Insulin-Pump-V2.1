idf_component_register(
    SRCS            "leds.c"
    INCLUDE_DIRS    .
    REQUIRES        driver battery motor nvs_flash
)
