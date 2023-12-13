idf_component_register(
    SRCS            "logging.c"
    INCLUDE_DIRS    .
    REQUIRES        fatfs console wear_levelling espressif__esp_tinyusb
)
