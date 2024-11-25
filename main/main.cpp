#include "Arduino.h"
#include "Wire.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_ADXL345_U.h"
#include "Adafruit_Sensor.h"
#include "VL53L1X.h"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
void ArduinoSetup();

#ifdef __cplusplus
extern "C" {
#endif
#include "wii_controller.h"

// #ifdef WII_REMOTE_HOST
// #define SPOOF_WIIMOTE
// #else
// #define SPOOF_WIIMOTE
// #endif

void app_main(void)
{
    //arduino setup:
    ArduinoSetup();

    bd_addr_t addr;

#if defined(SPOOF_WIIMOTE)
    //00:19:1d:54:d1:a4
    addr[0] = 0x00;
    addr[1] = 0x19;
    addr[2] = 0x1d;
    addr[3] = 0x54;
    addr[4] = 0xd1;
    addr[5] = 0xa4 - 2;
#elif defined(SPOOF_WII)
    addr[0] = 0x00;
    addr[1] = 0x19;
    addr[2] = 0x1d;
    addr[3] = 0x22;
    addr[4] = 0x73;
    addr[5] = 0x29 - 2;
#else
    addr[0] = 0x00;
    addr[1] = 0x19;
    addr[2] = 0x1d;
    addr[3] = 0x54;
    addr[4] = 0xd1;
    addr[5] = 0xa0;
#endif

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ret = esp_base_mac_addr_set(addr);
    ESP_ERROR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ESP_ERROR_CHECK(ret);

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    ESP_ERROR_CHECK(ret);

    wii_controller_init();

    static esp_vhci_host_callback_t vhci_host_cb =
    {
        .notify_host_send_available = NULL,
        .notify_host_recv = queue_packet_handler,
    };
    ret = esp_vhci_host_register_callback(&vhci_host_cb);
    ESP_ERROR_CHECK(ret);

#if defined(WII_REMOTE_HOST)
    wii_remote_host();
#elif defined(WII_MITM)
    wii_mitm();
#else
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    fake_wii_remote();

    char button_press = 0;
    while(1)
    {
        if((digitalRead(0) == LOW)&&(button_press==0))
        {
            button_press = 1;
            home=1;
            digitalWrite(2,HIGH);
        }
        else if(digitalRead(0) == HIGH)
        {
            button_press=0;
            home=0;
            digitalWrite(2,LOW);
        }
    }
#endif
}
#ifdef __cplusplus
}
#endif

void ArduinoSetup()
{
    pinMode(2,OUTPUT);
    digitalWrite(2,LOW);
    pinMode(0,INPUT);
    //Wire.begin();
    pinMode(14,INPUT); //Bouton Stick
}
