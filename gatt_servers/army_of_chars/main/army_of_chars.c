/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* This file is for a gatt server CTF (capture the flag). 
*
****************************************************************************/


 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/event_groups.h"
 #include "esp_system.h"
 #include "esp_log.h"
 #include "nvs_flash.h"
 #include "esp_bt.h"
 #include "driver/gpio.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "army_of_chars.h"
#include "gatt_server_common.h"
#include "esp_gatt_common_api.h"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "FLAG_XX"
#define SVC_INST_ID                 0

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 100
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)


static uint8_t adv_config_done       = 0;

uint16_t blectf_handle_table[ARMY_OF_CHARS_IDX_NB];

#define CONFIG_SET_RAW_ADV_DATA
#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power*/
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF, 0x00,
        /* device name (first number is the length) */
	//TODO generate flag name
        0x08, 0x09, 'F','L','A','G','_','X','X'

};
static uint8_t raw_scan_rsp_data[] = {
        /* flags */
        0x02, 0x01, 0x06,
        /* tx power */
        0x02, 0x0a, 0xeb,
        /* service uuid */
        0x03, 0x03, 0xFF,0x00
};

#else
static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 16,
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst blectf_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_TEST                   = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_WRITE_WARP                = 0xFF01;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS1                 = 0xFF02;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS2                 = 0xFF03;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS3                 = 0xFF04;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS4                 = 0xFF05;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS5                 = 0xFF06;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS6                 = 0xFF07;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS7                 = 0xFF08;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS8                 = 0xFF09;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS9                 = 0xFF0a;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS10                 = 0xFF0b;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS11                 = 0xFF0c;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS12                 = 0xFF0d;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS13                 = 0xFF0e;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS14                 = 0xFF0f;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS15                 = 0xFF10;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS16                 = 0xFF11;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS17                 = 0xFF12;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS18                 = 0xFF13;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS19                 = 0xFF14;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS20                 = 0xFF15;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS21                 = 0xFF16;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS22                 = 0xFF17;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS23                 = 0xFF18;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS24                 = 0xFF19;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS25                 = 0xFF1a;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS26                 = 0xFF1b;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS27                 = 0xFF1c;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS28                 = 0xFF1d;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS29                 = 0xFF1e;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS30                 = 0xFF1f;

static const uint16_t GATTS_CHAR_UUID_READ_DOCS31                 = 0xFF20;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS32                 = 0xFF21;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS33                 = 0xFF22;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS34                 = 0xFF23;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS35                 = 0xFF24;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS36                 = 0xFF25;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS37                 = 0xFF26;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS38                 = 0xFF27;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS39                 = 0xFF28;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS40                 = 0xFF29;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS41                 = 0xFF2a;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS42                 = 0xFF2b;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS43                 = 0xFF2c;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS44                 = 0xFF2d;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS45                 = 0xFF2e;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS46     = 0xFF2f;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS47     = 0xFF30;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS48     = 0xFF31;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS49     = 0xFF32;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS50     = 0xFF33;
static const uint16_t GATTS_CHAR_UUID_READ_DOCS51     = 0xFF34;


static const uint16_t GATTS_CHAR_UUID_READ_FLAG                 = 0xFF67;

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                =  ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;

// start ctf data vars
static char writeData[100];
static char docs_value[20] = "cccccccccccccccccccc";
static const char warp_value[] = "write here to goto to scoreboard";

static char flag_army_of_chars_value[] = "12345678901234567890";

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[ARMY_OF_CHARS_IDX_NB] =
{
    // Service Declaration
    [ARMY_OF_CHARS_IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TEST), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS1]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS1]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS1, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS2]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS2]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS2, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS3]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS3]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS3, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS4]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS4]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS4, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS5]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS5]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS5, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS6]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS6]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS6, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS7]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS7]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS7, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS8]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS8]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS8, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS9]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS9]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS9, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS10]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    
    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS10]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS10, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS11]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS11]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS11, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS12]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS12]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS12, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS13]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS13]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS13, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS14]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS14]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS14, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS15]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS15]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS15, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS16]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS16]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS16, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS17]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS17]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS17, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS18]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS18]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS18, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS19]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS19]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS19, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS20]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS20]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS20, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS21]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS21]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS21, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS22]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS22]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS22, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS23]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS23]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS23, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS24]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS24]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS24, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS25]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS25]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS25, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS26]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS26]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS26, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS27]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS27]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS27, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS28]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS28]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS28, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS29]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS29]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS29, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS30]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS30]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS30, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS31]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS31]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS31, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS32]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS32]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS32, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS33]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS33]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS33, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS34]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS34]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS34, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS35]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS35]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS35, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS36]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS36]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS36, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS37]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS37]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS37, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS38]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS38]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS38, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS39]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS39]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS39, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS40]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS40]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS40, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS41]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS41]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS41, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS42]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS42]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS42, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS43]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS43]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS43, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS44]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS44]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS44, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS45]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS45]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS45, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS46]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS46]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS46, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS47]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS47]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS47, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS48]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS48]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS48, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS49]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS49]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS49, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},


    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS50]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS50]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS50, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},

    /* Documentation Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_DOCS51]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Documentation Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS51]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_DOCS51, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(docs_value)-1, (uint8_t *)docs_value}},







    
    /* Flag Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_READ_FLAG]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Flag Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_READ_FLAG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_READ_FLAG, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(flag_army_of_chars_value)-1, (uint8_t *)flag_army_of_chars_value}},
    
    /* Warp Characteristic Declaration */
    [ARMY_OF_CHARS_IDX_CHAR_WRITE_WARP]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Warp Characteristic Value */
    [ARMY_OF_CHARS_IDX_CHAR_VAL_WRITE_WARP]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_WRITE_WARP, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(warp_value)-1, (uint8_t *)warp_value}},

};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    #ifdef CONFIG_SET_RAW_ADV_DATA
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #else
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #endif
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed");
            }else{
                ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "update connetion params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_REG_EVT");
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
    #ifdef CONFIG_SET_RAW_ADV_DATA
            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if (raw_adv_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
    #else
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
    #endif
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, ARMY_OF_CHARS_IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
            //create random values
            const char charset[] = "abcdefghijklmnopqrstuvwxyz1234567890";
            char tmp[20] = "aaaaaaaaaaaaaaaaaaaa";
            for (size_t n = 0; n < 20; n++) {
                int key = rand() % (int) (sizeof charset - 1);
                tmp[n] = charset[key];
            }
            //memset(docs_value, 0, 20);
            ESP_LOGI(GATTS_TABLE_TAG, "memcopy %s", tmp);
            memset(docs_value, 0, sizeof tmp);
            memcpy(docs_value, tmp, sizeof tmp);
            //docs_value[20] = '\0';
            if(param->read.handle < 142){
                esp_ble_gatts_set_attr_value(blectf_handle_table[param->read.handle - 38], 20, (uint8_t *)tmp);
                ESP_LOGI(GATTS_TABLE_TAG, "rewriting");
            }
            ESP_LOGI(GATTS_TABLE_TAG, "write_warp handle = %d", blectf_handle_table[ARMY_OF_CHARS_IDX_CHAR_WRITE_WARP]);
            ESP_LOGI(GATTS_TABLE_TAG, "flag handle = %d", blectf_handle_table[ARMY_OF_CHARS_IDX_CHAR_READ_FLAG]);
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT, handle = %d", param->read.handle);

       	    break;
        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_WRITE_EVT");
            
            if (!param->write.is_prep){
                ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                
                // store write data for flag checking
                memset(writeData, 0, sizeof writeData);
                memcpy(writeData, param->write.value, 20); 
                
                //warp back to scorebord
                if (param->write.handle == blectf_handle_table[ARMY_OF_CHARS_IDX_CHAR_WRITE_WARP]+1){
                    esp_restart();
                }
            }
            else{
                /* handle prepare write */
                ESP_LOGI(GATTS_TABLE_TAG, "PREPARE WRITE TRIGGERED");
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d", param->conf.status);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != ARMY_OF_CHARS_IDX_NB){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_NB(%d)", param->add_attr_tab.num_handle, ARMY_OF_CHARS_IDX_NB);
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(blectf_handle_table, param->add_attr_tab.handles, sizeof(blectf_handle_table));
                esp_ble_gatts_start_service(blectf_handle_table[ARMY_OF_CHARS_IDX_SVC]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CLOSE_EVT");
            break;
        case ESP_GATTS_LISTEN_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_LISTEN_EVT");
            break;
        case ESP_GATTS_CONGEST_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONGEST_EVT");
            break;
        case ESP_GATTS_UNREG_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_UNREG_EVT");
            break;
        case ESP_GATTS_DELETE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DELETE_EVT");
            break;
        case ESP_GATTS_RESPONSE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_RESPONSE_EVT");
            break;
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            blectf_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == blectf_profile_tab[idx].gatts_if) {
                if (blectf_profile_tab[idx].gatts_cb) {
                    blectf_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}
//TODO: generate flag name
void app_main()
{

    //CODEGEN_FLAG_VALUES

    ESP_LOGI(GATTS_TABLE_TAG, "######## FLAG 1 ########");
    esp_err_t ret;

    //uint8_t new_mac[8] = {0xDE,0xAD,0xBE,0xEF,0xBE,0xEF};
    //esp_base_mac_addr_set(new_mac);
    
    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    //set current to 0 to return to dashboard on reset
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGI(GATTS_TABLE_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }else
    {
        ESP_LOGI(GATTS_TABLE_TAG, "writing value = %d", 0);
        err = nvs_set_i32(my_handle, "current_flag", 0);
        err = nvs_commit(my_handle);
    }
    nvs_close(my_handle);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed", __func__);
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}
