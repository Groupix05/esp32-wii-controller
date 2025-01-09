#include "Arduino.h"
#include "wiimote_display.h"
#include "Wire.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_ADXL345_U.h"
#include "Adafruit_Sensor.h"
#include "VL53L1X.h"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
void ArduinoSetup();
void searching_wii();
void start_external_sensors();
void start_display();
void sensors_and_display();
void wii_display();
void leds(char pinled_1,char pinled_2,char pinled_3,char pinled_4);
void buttons(char pin_button_home,char pin_button_minus,char pin_button_plus,char pin_button_a,char pin_button_b,char pin_button_one,char pin_button_two,char pin_button_power);
void ir(char pin_stick_x,char pin_stick_y);
void d_pad(char pin_stick_x,char pin_stick_y);
VL53L1X sensor;
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,-1);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
bool loup=1;
unsigned long int timer=0;
int ADXL345 = 0x53;
sensors_event_t event;


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
    addr[2] = 0x1d;
    addr[3] = 0x54;
    addr[4] = 0xd1;
    addr[5] = 0xa4 - 2;
#elif defined(SPOOF_WII)
    addr[0] = 0x00;
    addr[2] = 0x1d;
    addr[3] = 0x22;
    addr[4] = 0x73;
    addr[5] = 0x29 - 2;
#else
    addr[0] = 0x00;
    addr[2] = 0x1d;
    addr[3] = 0x54;
    addr[4] = 0xd1;
    addr[5] = 0xa0;
#endif
    if(loup==1)
    {
        addr[1] = 0x20;
    }
    else
    {
        addr[1] = 0x19;
    }

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
    timer = millis();

    bool button_mode=0;
    while(1)
    {
        display.clearDisplay();
        if(wii_searching==1)
        {
            searching_wii();
        }
        else
        {
            if(is_connected==1)
            {
                if(loup==1)
                {
                    buttons(12,14,27,34,19,15,13,4);
                }
                else
                {
                    buttons(0,14,27,34,19,15,13,4);
                }
                if(joystick_mode==0)
                {
                    ir(25,26);
                }
                else
                {
                    d_pad(25,26);
                }
                if((digitalRead(33)==LOW)&&(button_mode==0))
                {
                    button_mode=1;
                    joystick_mode=!joystick_mode;
                    if(joystick_mode==0)
                    {
                        ir_x = 435;
                        ir_y = 320;
                    }
                    else
                    {
                        ir_x = 0;
                        ir_y = 0;
                    }
                }
                else if(digitalRead(33)==HIGH)
                {
                    button_mode=0;
                }
                //sensors
                sensors_and_display();
            }
            //else if((is_connected==0) || ((millis()-timer>=2000)&&(is_connected==2)))
            //{
            //    printf("Reconnection...\n");
            //    timer=millis();
            //    is_connected=2;
            //    reconnect();
            //}
        }
        if(loup==1)
        {
            leds(32,23,18,5);
        }
        else
        {
            leds(2,23,18,5);
        }
        wii_display();
        wii_battery=map(battery,0,100,0,255);
    }
#endif
}
#ifdef __cplusplus
}
#endif

void ArduinoSetup()
{
    pinMode(25,INPUT);   //JOYSTICK X
    pinMode(26,INPUT);   //JOYSTICK Y
    pinMode(12,INPUT);  //BUTTON HOME
    pinMode(14,INPUT);  //BUTTON MINUS
    pinMode(27,INPUT);  //BUTTON PLUS
    pinMode(34,INPUT);  //BUTTON A
    pinMode(19,INPUT);  //BUTTON B
    pinMode(15,INPUT);  //BUTTON ONE
    pinMode(13,INPUT);  //BUTTON TWO
    pinMode(4,INPUT);   //BUTTON POWER
    pinMode(33,INPUT);  //Bouton Joystick/Mode

    pinMode(32,OUTPUT); //LED 1
    pinMode(23,OUTPUT); //LED 2
    pinMode(18,OUTPUT); //LED 3
    pinMode(5,OUTPUT); //LED 4

    digitalWrite(32,HIGH);
    digitalWrite(23,HIGH);
    digitalWrite(18,HIGH);
    digitalWrite(5,HIGH);

    pinMode(0,INPUT);   //BUTTON ESP32
    pinMode(2,OUTPUT);  //LED ESP32
    Wire.begin();
    pinMode(33,INPUT);  //BUTTON STICK
    if(loup==1)
    {
        start_external_sensors();
    }
    start_display();
}
void start_external_sensors()
{
    if(!accel.begin())
    {
        /* There was a problem detecting the ADXL345 ... check your connections */
        printf("Ooops, no ADXL345 detected ... Check your wiring!\n");
        while(1)
        {
        printf("Erreur accel");
        }
    }
    accel.setRange(ADXL345_RANGE_16_G);

    sensor.setTimeout(500);
    if (!sensor.init())
    {
        printf("Failed to detect and initialize sensor!\n");
        while(1)
        {
            printf("Erreur Sensor Dist");
        }
    }

    sensor.setDistanceMode(VL53L1X::Long);
    sensor.setMeasurementTimingBudget(50000);
    sensor.startContinuous(50);
}
void start_display()
{
    if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C))
    {
        printf("SSD1306 allocation failed");
        while(1)
        {
        printf("Erreur Screen");
        }
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
}
void wii_display()
{
    //display.drawBitmap(0, 0,epd_bitmap_wiimote, 158, 94, 1);
    display.drawBitmap(0, 0,epd_bitmap_wiimote_empty, 158, 94, 1);

    //status
    if(wii_searching==1)
    {
        display.setCursor(15,5);
        display.print("Searching...");
        if(player_number_led==0xF)
        {
            display.drawBitmap(0, 0,epd_bitmap_wiimote_player_all, 158, 94, 1);
        }
        display.display();
    }
    else
    {
        display.setCursor(15,5);
        if(is_connected==1)
        {
            display.print("Connected");
        }
        else if(is_connected==0)
        {
            display.print("Disconnected");
        }
        else if(is_connected==2)
        {
            display.print("Reconnection");
        }

    //battery
        display.setCursor(105,5);
        display.println(battery);

        if(millis()-timer>=300)
        {
            timer=millis();

    //Player number
            display.setCursor(105,55);
            if(player_number_led>2)
            {
                if((player_number_led & WII_REMOTE_LED_3)==WII_REMOTE_LED_3)
                {
                    display.drawBitmap(0, 0,epd_bitmap_wiimote_player_three, 158, 94, 1);
                    display.println("3");
                }
                else if((player_number_led & WII_REMOTE_LED_4)==WII_REMOTE_LED_4)
                {
                    display.drawBitmap(0, 0,epd_bitmap_wiimote_player_four, 158, 94, 1);
                    display.println("4");
                }
            }
            else
            {
                display.println(player_number_led);
                if(player_number_led==2)
                {
                    display.drawBitmap(0, 0,epd_bitmap_wiimote_player_two, 158, 94, 1);
                }
                else
                {
                    display.drawBitmap(0, 0,epd_bitmap_wiimote_player_one, 158, 94, 1);
                }
            }

    //buttons
            if(home_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_home, 158, 94, 1);
            }
            if(minus_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_minus, 158, 94, 1);
            }
            if(plus_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_plus, 158, 94, 1);
            }
            if(a_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_a, 158, 94, 1);
            }
            if(b_button==1)
            {
                display.setCursor(25,28);
                display.println("B");
            }
            if(one_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_one, 158, 94, 1);
            }
            if(two_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_two, 158, 94, 1);
            }
            if(power_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_power, 158, 94, 1);
            }
            if(up_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_up, 158, 94, 1);
            }
            if(down_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_down, 158, 94, 1);
            }
            if(left_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_left, 158, 94, 1);
            }
            if(right_button==1)
            {
                display.drawBitmap(0, 0,epd_bitmap_wiimote_right, 158, 94, 1);
            }

    //Joystick mode
            display.setCursor(55,55);
            if(joystick_mode==0)
            {
                display.println("IR");
            }
            else
            {
                display.println("D_PAD");
            }

            display.display();
        }
    }


}

void sensors_and_display()
{
    accel.getEvent(&event);
    acc_x = event.acceleration.x;
    acc_z = event.acceleration.z+10;
    acc_y = event.acceleration.y;
    if(acc_x>10)
    {
        acc_x = 10;
    }
    else if(acc_x<-10)
    {
        acc_x=-10;
    }
    if(acc_y>10)
    {
        acc_y = 10;
    }
    else if(acc_y<-10)
    {
        acc_y=-10;
    }
    if(acc_z>10)
    {
        acc_z = 10;
    }
    else if(acc_z<-10)
    {
        acc_z=-10;
    }
    wii_x = ((unsigned long)((acc_x/16)*3))&0x03FF;
    wii_y = ((unsigned long)((acc_y/16)*3))&0x03FF;
    wii_z = ((unsigned long)((acc_z/16)*3))&0x03FF;

    //wii_y=wii_y+10;
    //if(wii_y>511)
    //{
    //    wii_y=0;
    //}
    //printf("%02lx",wii_x);
    //printf("acc_x = %ld\n",acc_x_ld);

    display.setCursor(0,25);
    display.print("Z=");
    display.setCursor(15,25);
    display.println(wii_z);
    display.setCursor(0,35);
    display.print("X=");
    display.setCursor(15,35);
    display.println(wii_x);
    display.setCursor(0,45);
    display.print("Y=");
    display.setCursor(15,45);
    display.println(wii_y);

    //display.setCursor(75,25);
    //display.println(analogRead(25));
    //display.setCursor(75,35);
    //display.println(analogRead(26));
    display.setCursor(5,55);
    display.print(sensor.read());
}
void searching_wii()
{
    if(millis()-timer>=125)
    {
        if(player_number_led==0xF)
        {
            player_number_led=0x0;
        }
        else
        {
            player_number_led=0xF;
        }
        timer=millis();
    }
}

void leds(char pinled_1,char pinled_2,char pinled_3,char pinled_4)
{
        if((player_number_led & WII_REMOTE_LED_1)==WII_REMOTE_LED_1) //mask to see the LED 1 bit
        {
            digitalWrite(pinled_1,HIGH);
        }
        else
        {
            digitalWrite(pinled_1,LOW);
        }
        if((player_number_led & WII_REMOTE_LED_2)==WII_REMOTE_LED_2) //mask to see the LED 2 bit
        {
            digitalWrite(pinled_2,HIGH);
        }
        else
        {
            digitalWrite(pinled_2,LOW);
        }
        if((player_number_led & WII_REMOTE_LED_3)==WII_REMOTE_LED_3) //mask to see the LED 3 bit
        {
            digitalWrite(pinled_3,HIGH);
        }
        else
        {
            digitalWrite(pinled_3,LOW);
        }
        if((player_number_led & WII_REMOTE_LED_4)==WII_REMOTE_LED_4) //mask to see the LED 4 bit
        {
            digitalWrite(pinled_4,HIGH);
        }
        else
        {
            digitalWrite(pinled_4,LOW);
        }
}
void buttons(char pin_button_home,char pin_button_minus,char pin_button_plus,char pin_button_a,char pin_button_b,char pin_button_one,char pin_button_two,char pin_button_power)
{
    if(digitalRead(pin_button_home) == LOW)//home
    {
        home_button=1;
    }
    else
    {
        home_button=0;
    }
    if(digitalRead(pin_button_minus) == LOW)//minus
    {
        minus_button=1;
    }
    else
    {
        minus_button=0;
    }
    if(digitalRead(pin_button_plus) == LOW)//plus
    {
        plus_button=1;
    }
    else
    {
        plus_button=0;
    }
    if(digitalRead(pin_button_a) == LOW)//a
    {
        a_button=1;
    }
    else
    {
        a_button=0;
    }
    if(digitalRead(pin_button_b) == LOW)//b
    {
        b_button=1;
    }
    else
    {
        b_button=0;
    }
    if(digitalRead(pin_button_one) == LOW)//one
    {
        one_button=1;
    }
    else
    {
        one_button=0;
    }
    if(digitalRead(pin_button_two) == LOW)//two
    {
        two_button=1;
    }
    else
    {
        two_button=0;
    }
    if(digitalRead(pin_button_power) == LOW)//power
    {
        power_button=1;
    }
    else
    {
        power_button=0;
    }
}

void ir(char pin_stick_x,char pin_stick_y)
{
    long int stick_x = analogRead(pin_stick_x);
    long int stick_y = analogRead(pin_stick_y);
    if((stick_x<1000)&&(ir_x>130)&&(ir_x<640))
    {
        ir_x+=10;
    }
    else if((stick_x>3000)&&(ir_x<650)&&(ir_x>140))
    {
        ir_x-=10;
    }
    if((stick_y<1000)&&(ir_y>130)&&(ir_y<520))
    {
        ir_y+=10;
    }
    else if((stick_y>3000)&&(ir_y<530)&&(ir_y>140))
    {
        ir_y-=10;
    }
    up_button=0;
    down_button=0;
    right_button=0;
    left_button=0;
}

void d_pad(char pin_stick_x,char pin_stick_y)
{
    long int stick_x = analogRead(pin_stick_x);
    long int stick_y = analogRead(pin_stick_y);
    if(stick_x<1800)
    {
        right_button=0;
        left_button=1;
    }
    else if(stick_x>2100)
    {
        left_button=0;
        right_button=1;
    }
    else
    {
        right_button=0;
        left_button=0;
    }
    if(stick_y<1800)
    {
        up_button=0;
        down_button=1;
    }
    else if(stick_y>2100)
    {
        down_button=0;
        up_button=1;
    }
    else
    {
        up_button=0;
        down_button=0;
    }
}
