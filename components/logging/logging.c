/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* DESCRIPTION:
 * This example contains code to make ESP32-S3 based device recognizable by USB-hosts as a USB Mass Storage Device.
 * It either allows the embedded application i.e. example to access the partition or Host PC accesses the partition over USB MSC.
 * They can't be allowed to access the partition at the same time.
 * For different scenarios and behaviour, Refer to README of this example.
 */

#include "logging.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TAG "LOGGING"

#define EPNUM_MSC       1
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

#define BASE_PATH "/data" // base path to mount the partition

#define PROMPT_STR CONFIG_IDF_TARGET

enum {
    ITF_NUM_MSC = 0,
    ITF_NUM_TOTAL
};

enum {
    EDPT_CTRL_OUT = 0x00,
    EDPT_CTRL_IN  = 0x80,
    EDPT_MSC_OUT  = 0x01,
    EDPT_MSC_IN   = 0x81,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


void LOGGING_checkAndCreateLog  ( void );

void task_LOGGING_handler       ( void * );

// static int          USB_read                    ( int, char ** );
// static int          USB_write                   ( int, char ** );
// static int          USB_size                    ( int, char ** );
// static int          USB_status                  ( int, char ** );
// static int          USB_exit                    ( int, char ** );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool connected = false;
bool initialised = false;

const char *log_filename = BASE_PATH "/log.csv";

time_t unixTime = 0;

static uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EDPT_MSC_OUT, EDPT_MSC_IN, TUD_OPT_HIGH_SPEED ? 512 : 64),
};

static tusb_desc_device_t descriptor_config = {
    .bLength = sizeof(descriptor_config),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A, // This is Espressif VID. This needs to be changed according to Users / Customers
    .idProduct = 0x4002,
    .bcdDevice = 0x100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "University of Canterbury",     // 1: Manufacturer
    "Bluetooth Insulin Pump",       // 2: Product
    "123456",                       // 3: Serials
    "Logging MSC",                  // 4. MSC
};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 *  
 */
void LOGGING_init ( void )
{
    // INITIALISE FUNCTION VARIABLES
    static wl_handle_t wl = WL_INVALID_HANDLE;
    // RETRIEVE DATA PARTITION
    const esp_partition_t *data_partition = esp_partition_find_first( ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL );
    // CHECK PARTITION WAS FOUND
    if ( data_partition == NULL ) { return; }
    // MOUNT WEAR LEVELLING
    wl_mount( data_partition, &wl );
    
    // REGISTER STORAGE TYPE SPIFLASH WITH TINYUSB DRIVER
    const tinyusb_msc_spiflash_config_t config_spi = { .wl_handle = wl };
    tinyusb_msc_storage_init_spiflash(&config_spi);

    // MOUNT IN THE APP BY DEFAULT
    tinyusb_msc_storage_mount(BASE_PATH);

    // 
    LOGGING_checkAndCreateLog();

    //
    initialised = true;
    
    //
    LOGGING_append( reboot, 0 );

    //
    xTaskCreate(task_LOGGING_handler, "Handles LOGGING Functionality", (4*1024), NULL, 15, NULL);
}

/*
 *  
 */
void LOGGING_start ( void )
{
}

/*
 * 
 */
void LOGGING_append ( log_commands cmd, uint32_t value )
{
    if ( initialised && !connected ) 
    {
        //
            FILE *log = fopen(log_filename, "a");
        //
        switch (cmd) {
            case reboot:
            fprintf(log, "%ld,%lld,Device Reboot,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime) );
            ESP_LOGI(TAG,"LOG TO FILE: Device Reboot");
            break;

            case recieve_time:
            unixTime = value;
            fprintf(log, "%ld,%lld,Recieve UNIX Time,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime) );
            ESP_LOGI(TAG,"LOG TO FILE: Recieve UNIX Time, %lld", unixTime);
            break;

            case recieve_basal:
            fprintf(log, "%ld,%lld,Recieve New Basal Rate,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Recieve New Basal Rate (%ld)", value);
            break;

            case recieve_bolus:
            fprintf(log, "%ld,%lld,Recieve New Bolus,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Recieve New Bolus (%ld)", value);
            break;

            case recieve_rewind:
            fprintf(log, "%ld,%lld,Recieve Plunger Rewind Command,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime) );
            ESP_LOGI(TAG,"LOG TO FILE: Recieve Plunger Rewind Command");
            break;

            case recieve_prime:
            fprintf(log, "%ld,%lld,Recieve Plunger Prime Command,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Recieve Plunger Prime Command (%ld)", value);
            break;

            case bluetooth_off:
            fprintf(log, "%ld,%lld,Bluetooth Turned OFF,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Bluetooth Turned OFF");
            break;

            case bluetooth_on_restart:
            fprintf(log, "%ld,%lld,Bluetooth Turned ON (Controlled Reboot),%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Bluetooth Turned ON (Controlled Reboot)");
            break;

            case deliver_basal:
            fprintf(log, "%ld,%lld,Delivering Basal Dose,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Delivering Basal Dose(%ld)", value);
            break;

            case deliver_bolus:
            fprintf(log, "%ld,%lld,Delivering Bolus,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Delivering Bolus (%ld)", value);
            break;

            case plunger_prime:
            fprintf(log, "%ld,%lld,Execiting Plunger Prime,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Executing Plunger Prime (%ld)", value);
            break;

            case plunger_rewindStart:
            fprintf(log, "%ld,%lld,Starting Plunger Rewind,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Starting Plunger Rewind");
            break;

            case plunger_rewindFinish:
            fprintf(log, "%ld,%lld,Finished Plunger Rewind,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Finished Plunger Rewind");
            break;

            default:
            fprintf(log, "%ld,%lld,Unhandled Command,%ld,%s", xTaskGetTickCount(), unixTime, value, ctime(&unixTime));
            ESP_LOGI(TAG,"LOG TO FILE: Unhandled Command, %lld", unixTime);
            break;
        }
        //
        fclose(log);
    } 
}



//     uint32_t sec_count = tinyusb_msc_storage_get_sector_count();
//     uint32_t sec_size = tinyusb_msc_storage_get_sector_size();
//     printf("Storage Capacity %lluMB\n", ((uint64_t) sec_count) * sec_size / (1024 * 1024));
//     printf("storage exposed over USB: %s\n", tinyusb_msc_storage_in_use_by_usb_host() ? "Yes" : "No");

/*
 * 
 */
bool LOGGING_connectedToPC ( void )
{
    return connected;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 *  
 */
void LOGGING_checkAndCreateLog ( void )
{   
    FILE *log = fopen(log_filename, "r");

    if (!log) 
    {
        log = fopen(log_filename, "w");
        fprintf(log, "Tick Count,UNIX,Command,Value,Date\n");
        fprintf(log, "%ld,%lld,Create Log File,0,%s\n", xTaskGetTickCount(), unixTime, ctime(&unixTime));
        fclose(log);
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* RTOS FUNCTIONS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void task_LOGGING_handler(void *arg)
{
    // UPDATE LOOP VARIABLES
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // 
    while (1)
    {
        if ( BUTTON_getFlagUSB() ) 
        {
            BT_disable();
            connected = true;
            BUTTON_resetFlagUSB();
            LED_flashFive_triple();

            // INITIALISE USB MSC
            const tinyusb_config_t tusb_cfg = {
                .device_descriptor = &descriptor_config,
                .string_descriptor = string_desc_arr,
                .string_descriptor_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]),
                .external_phy = false,
                .configuration_descriptor = desc_configuration, };
            tinyusb_driver_install(&tusb_cfg);

            while ( !BUTTON_getFlagUSB() ) { vTaskDelay(pdMS_TO_TICKS(500)); }

            LED_flashFive_triple();

            // Force A Hardware Reset
            gpio_set_direction(GPIO_NUM_6, GPIO_MODE_OUTPUT);
            gpio_set_level(GPIO_NUM_6, false);
            // Wait For Reset
            vTaskDelay(pdMS_TO_TICKS(100)); 
            // If Hardware Mod Not Done Just Do A Software Reset
            esp_restart();
        }

        unixTime += 1;

        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(1000) ); 
    }

}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// /*
//  * READ 'BASE_PATH/README.MD' AND PRINT IT CONTENTS
//  */
// static int USB_read(int argc, char **argv)
// {
//     if (tinyusb_msc_storage_in_use_by_usb_host()) {
//         // ESP_LOGE(TAG, "storage exposed over USB. Application can't read from storage.");
//         return -1;
//     }
//     // ESP_LOGD(TAG, "read from storage:");
//     const char *filename = BASE_PATH "/README.MD";
//     FILE *ptr = fopen(filename, "r");
//     if (ptr == NULL) {
//         // ESP_LOGE(TAG, "Filename not present - %s", filename);
//         return -1;
//     }
//     char buf[1024];
//     while (fgets(buf, 1000, ptr) != NULL) {
//         printf("%s", buf);
//     }
//     fclose(ptr);
//     return 0;
// }

// /*
//  * CREATE FILE 'BASE_PATH/README.MD' IF IT DOES NOT EXIST
//  */
// static int USB_write(int argc, char **argv)
// {
//     if (tinyusb_msc_storage_in_use_by_usb_host()) {
//         // ESP_LOGE(TAG, "storage exposed over USB. Application can't write to storage.");
//         return -1;
//     }
//     // ESP_LOGD(TAG, "write to storage:");
//     const char *filename = BASE_PATH "/README.MD";
//     FILE *fd = fopen(filename, "r");
//     if (!fd) {
//         // ESP_LOGW(TAG, "README.MD doesn't exist yet, creating");
//         fd = fopen(filename, "w");
//         fprintf(fd, "Mass Storage Devices are one of the most common USB devices. It use Mass Storage Class (MSC) that allow access to their internal data storage.\n");
//         fprintf(fd, "In this example, ESP chip will be recognised by host (PC) as Mass Storage Device.\n");
//         fprintf(fd, "Upon connection to USB host (PC), the example application will initialize the storage module and then the storage will be seen as removable device on PC.\n");
//         fclose(fd);
//     }
//     return 0;
// }

// /*
//  * SHOW STORAGE SIZE AND SECTOR SIZE    
//  */
// static int USB_size(int argc, char **argv)
// {
//     if (tinyusb_msc_storage_in_use_by_usb_host()) {
//         // ESP_LOGE(TAG, "storage exposed over USB. Application can't access storage");
//         return -1;
//     }
//     uint32_t sec_count = tinyusb_msc_storage_get_sector_count();
//     uint32_t sec_size = tinyusb_msc_storage_get_sector_size();
//     printf("Storage Capacity %lluMB\n", ((uint64_t) sec_count) * sec_size / (1024 * 1024));
//     return 0;
// }


// /*
//  * EXIT FROM APPLICATION
//  */
// static int USB_status(int argc, char **argv)
// {
//     printf("storage exposed over USB: %s\n", tinyusb_msc_storage_in_use_by_usb_host() ? "Yes" : "No");
//     return 0;
// }

// /*
//  * EXIT FROM APPLICATION
//  */
// static int USB_exit( int argc, char **argv )
// {
//     tinyusb_msc_storage_deinit();
//     printf("Application Exiting\n");
//     exit(0);
//     return 0;
// }

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */