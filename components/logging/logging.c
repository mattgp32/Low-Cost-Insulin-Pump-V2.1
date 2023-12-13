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

static void         USB_mount                   ( void );
static int          USB_unmount                 ( int, char ** );
static esp_err_t    USB_initPartitionWearLevel  ( wl_handle_t * );

static int          USB_read                    ( int, char ** );
static int          USB_write                   ( int, char ** );
static int          USB_size                    ( int, char ** );
static int          USB_status                  ( int, char ** );
static int          USB_exit                    ( int, char ** );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
    "Prototype Insulin Pump",       // 2: Product
    "123456",                       // 3: Serials
    "Example MSC",                  // 4. MSC
};

const esp_console_cmd_t cmds[] = {
    {
        .command = "read",
        .help = "read BASE_PATH/README.MD and print its contents",
        .hint = NULL,
        .func = &USB_read,
    },
    {
        .command = "write",
        .help = "create file BASE_PATH/README.MD if it does not exist",
        .hint = NULL,
        .func = &USB_write,
    },
    {
        .command = "size",
        .help = "show storage size and sector size",
        .hint = NULL,
        .func = &USB_size,
    },
    {
        .command = "expose",
        .help = "Expose Storage to Host",
        .hint = NULL,
        .func = &USB_unmount,
    },
    {
        .command = "status",
        .help = "Status of storage exposure over USB",
        .hint = NULL,
        .func = &USB_status,
    },
    {
        .command = "exit",
        .help = "exit from application",
        .hint = NULL,
        .func = &USB_exit,
    }
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void LOGGING_init(void)
{
    // INITIALISE WEAR LEVELING ON THE PARTITION
    static wl_handle_t wl = WL_INVALID_HANDLE;
    USB_initPartitionWearLevel(&wl);
    
    // REGISTER STORAGE TYPE SPIFLASH WITH tinyusb DRIVER
    const tinyusb_msc_spiflash_config_t config_spi = { .wl_handle = wl };
    tinyusb_msc_storage_init_spiflash(&config_spi);

    // MOUNT IN THE APP BY DEFAULT
    USB_mount();

    // INITIALISE USB MSC
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &descriptor_config,
        .string_descriptor = string_desc_arr,
        .string_descriptor_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]),
        .external_phy = false,
        .configuration_descriptor = desc_configuration,
    };
    tinyusb_driver_install(&tusb_cfg);

    // 
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    // PROMPT TO BE PRINTED BEFORE EACH LINE. THIS CAN BE CUSTOMISED
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = 64;
    esp_console_register_help_command();
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_console_new_repl_uart(&hw_config, &repl_config, &repl);
    for (int count = 0; count < sizeof(cmds) / sizeof(esp_console_cmd_t); count++) {
        esp_console_cmd_register(&cmds[count]);
    }
    esp_console_start_repl(repl);
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                    */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * MOUNT THE PARTITION AND SHOW ALL FILES IN THE 'BASE_PATH'
 */
static void USB_mount(void)
{
    // INIT FUNCTION VARIABLES
    struct dirent *d;
    
    // MOUNT STORAGE
    tinyusb_msc_storage_mount(BASE_PATH);

    // LIST ALL FILES IN THIS DIRECTORY
    DIR *dh = opendir(BASE_PATH);

    // CHECK FOR ERRORS WITH DIRECTORY
    if (!dh) {
        // DIRECTORY IS NOT FOUND
        if (errno == ENOENT) {
            // ESP_LOGE(TAG, "Directory doesn't exist %s", BASE_PATH);
        } 
        // DIRECTORY IS NOT READABLE
        else {
            // ESP_LOGE(TAG, "Unable to read directory %s", BASE_PATH);
        }
        // EXIT
        return;
    }

    // PRINT DIRECTORY FILES UNTIL NEXT ENTRY IS NOT READABLE
    while ( (d = readdir(dh)) != NULL ) {
        printf("%s\n", d->d_name);
    }

    return;
}

/*
 * UNMOUNT THE STORAGE
 */
static int USB_unmount(int argc, char **argv)
{
    if (tinyusb_msc_storage_in_use_by_usb_host()) {
        // ESP_LOGE(TAG, "storage is already exposed");
        return -1;
    }
    // ESP_LOGI(TAG, "Unmount storage...");
    ESP_ERROR_CHECK(tinyusb_msc_storage_unmount());
    return 0;
}

/*
 * INITIALISE WEAR LEVELLING?
 */
static esp_err_t USB_initPartitionWearLevel ( wl_handle_t *wl_handle )
{
    // RETRIEVE DATA PARTITION
    const esp_partition_t *data_partition = esp_partition_find_first( ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL );

    // CHECK PARTITION WAS FOUND
    if ( data_partition == NULL ) {
        // FAILED TO FIND FATFS PARTITION. CHECK PARTITION TABLE
        return ESP_ERR_NOT_FOUND;
    }

    // MOUNT WEAR LEVELLING
    return wl_mount( data_partition, wl_handle );
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS                                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * READ 'BASE_PATH/README.MD' AND PRINT IT CONTENTS
 */
static int USB_read(int argc, char **argv)
{
    if (tinyusb_msc_storage_in_use_by_usb_host()) {
        // ESP_LOGE(TAG, "storage exposed over USB. Application can't read from storage.");
        return -1;
    }
    // ESP_LOGD(TAG, "read from storage:");
    const char *filename = BASE_PATH "/README.MD";
    FILE *ptr = fopen(filename, "r");
    if (ptr == NULL) {
        // ESP_LOGE(TAG, "Filename not present - %s", filename);
        return -1;
    }
    char buf[1024];
    while (fgets(buf, 1000, ptr) != NULL) {
        printf("%s", buf);
    }
    fclose(ptr);
    return 0;
}

/*
 * CREATE FILE 'BASE_PATH/README.MD' IF IT DOES NOT EXIST
 */
static int USB_write(int argc, char **argv)
{
    if (tinyusb_msc_storage_in_use_by_usb_host()) {
        // ESP_LOGE(TAG, "storage exposed over USB. Application can't write to storage.");
        return -1;
    }
    // ESP_LOGD(TAG, "write to storage:");
    const char *filename = BASE_PATH "/README.MD";
    FILE *fd = fopen(filename, "r");
    if (!fd) {
        // ESP_LOGW(TAG, "README.MD doesn't exist yet, creating");
        fd = fopen(filename, "w");
        fprintf(fd, "Mass Storage Devices are one of the most common USB devices. It use Mass Storage Class (MSC) that allow access to their internal data storage.\n");
        fprintf(fd, "In this example, ESP chip will be recognised by host (PC) as Mass Storage Device.\n");
        fprintf(fd, "Upon connection to USB host (PC), the example application will initialize the storage module and then the storage will be seen as removable device on PC.\n");
        fclose(fd);
    }
    return 0;
}

/*
 * SHOW STORAGE SIZE AND SECTOR SIZE    
 */
static int USB_size(int argc, char **argv)
{
    if (tinyusb_msc_storage_in_use_by_usb_host()) {
        // ESP_LOGE(TAG, "storage exposed over USB. Application can't access storage");
        return -1;
    }
    uint32_t sec_count = tinyusb_msc_storage_get_sector_count();
    uint32_t sec_size = tinyusb_msc_storage_get_sector_size();
    printf("Storage Capacity %lluMB\n", ((uint64_t) sec_count) * sec_size / (1024 * 1024));
    return 0;
}

/*
 * EXIT FROM APPLICATION
 */
static int USB_status(int argc, char **argv)
{
    printf("storage exposed over USB: %s\n", tinyusb_msc_storage_in_use_by_usb_host() ? "Yes" : "No");
    return 0;
}

/*
 * EXIT FROM APPLICATION
 */
static int USB_exit(int argc, char **argv)
{
    tinyusb_msc_storage_deinit();
    printf("Application Exiting\n");
    exit(0);
    return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */