#include "Arduino.h"
#include "Wire.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_ADXL345_U.h"
#include "Adafruit_Sensor.h"
#include "VL53L1X.h"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
String var= "";
int ADXL345 = 0x53;
float X_out, Y_out, Z_out;
void ArduinoSetup();
typedef enum
{
    OFF,
    ON
}power_state;

power_state wii_power_state;

Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,-1);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
VL53L1X sensor;

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
    fake_wii_remote();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    wii_power_state=OFF;
    char button_press = 0;
    while(1)
    {
        if((digitalRead(0) == LOW)&&(button_press==0))
        {
            button_press = 1;
            if(wii_power_state==OFF)
            {
                printf("powering on...\n");
                connect_and_power_on();
                wii_power_state=ON;
                digitalWrite(2,HIGH);
            }
            else
            {
                printf("powering off...\n");
                connect_and_power_off();
                wii_power_state=OFF;
                digitalWrite(2,LOW);
            }
        }
        else if(digitalRead(0) == HIGH)
        {
            button_press=0;
        }
        sensors_event_t event;
        accel.getEvent(&event);
        X_out = event.acceleration.x;
        Z_out = event.acceleration.z-9;
        Y_out = event.acceleration.y;

        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(20,5);
        display.print("Bienvenue");
        display.setCursor(0,25);
        display.print("Z=");
        display.setCursor(15,25);
        display.println(Z_out);
        display.setCursor(0,35);
        display.print("X=");
        display.setCursor(15,35);
        display.println(X_out);
        display.setCursor(0,45);
        display.print("Y=");
        display.setCursor(15,45);
        display.println(Y_out);
        display.setCursor(50,15);
        display.print("Dist");
        display.setCursor(50,35);
        display.print(sensor.read());
        display.setCursor(80,15);
        display.print("Stick");
        display.setCursor(80,25);
        display.print("Y=");
        display.setCursor(95,25);
        display.print(analogRead(12));//Stick X
        display.setCursor(80,35);
        display.print("X=");
        display.setCursor(95,35);
        display.print(analogRead(13));//Stick Y
        display.setCursor(80,45);
        display.print(!digitalRead(14));//Bouton, actif à l'état bas (besoin d'une resistance entre SEL et VCC de +30k)
        display.display();
        display.clearDisplay();
    }
#endif
}
#ifdef __cplusplus
}
#endif

void ArduinoSetup()
{
    pinMode(2,OUTPUT);
    pinMode(0,INPUT);
    Wire.begin();
    pinMode(14,INPUT); //Bouton Stick
    if(!accel.begin())
    {
        /* There was a problem detecting the ADXL345 ... check your connections */
        printf("Ooops, no ADXL345 detected ... Check your wiring!");
        while(1)
        {
        printf("Erreur accel");
        }
    }
    sensor.setTimeout(500);
    if (!sensor.init())
    {
        while(1){
        printf("Failed to detect and initialize sensor!");
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(20,5);
        display.print("erreur");
        display.display();
        display.clearDisplay();}
    }
    accel.setRange(ADXL345_RANGE_16_G);
    if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C))
    {
        printf("SSD1306 allocation failed");
        while(1)
        {
        printf("Erreur Screen");
        }
    }
    delay(2000);
    display.clearDisplay();
    sensor.setDistanceMode(VL53L1X::Long);
    sensor.setMeasurementTimingBudget(50000);
    sensor.startContinuous(50);
}
