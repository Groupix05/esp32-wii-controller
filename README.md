# ESP32 Wii Controller with Components V1

Connect an ESP32 to a wii or a wiimote using FreeRTOS.
Thanks to nickbits1024 for the original work, this version is just the same but for ESP-IDF 5.3.1
With the "components" version, you can :
- Pair your ESP32 with a real wii
- Communicate real buttons with the wii
- Use a Joystick to simulate the wiimote's IR
- Use your accelerometer ADXL345 on the wii.

Missing feature :
- Power on and off
- Reconnect to the wii after a disconnection
- Use games that uses specific data reports that we didn't do.

## Tools used

- ESP-IDF v5.3.1
- Python 3.11.7
- ESP32 Dev Module NodeMCU from Joy-It

## Components Used

- ACCELEROMETER ADXL345
- OLED SCREEN SSD1306
- DISTANCE SENSOR VL53L1X
- JOYSTICK ADA512

## How to build

Install ESP-IDF :

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)

(not required anymore)
Then run in the project folder:
- cd components
- git clone --recursive https://github.com/espressif/arduino-esp32.git
- cd arduino-esp32
- git checkout 3.1.0-RC2


Manual transfer:
In the root :
- idf.py build
- idf.py flash -p [COM port]

optional :
- idf.py monitor

Automatic (only on Linux)
- Launch "Build and Flash ESP32.desktop"

## How to use

Check the branch [Kicad](https://github.com/Groupix05/esp32-wii-controller/tree/kicad) to know what to connect on what on the ESP32 before using.
Use the same components.

Once flash on the ESP32, the 4 leds should blink. If it's the case, then press the wii's sync button and you can now use your ESP32 like a real Wiimote. If you launch any games, restart your ESP32 and then redo the same process for connection.

## Documentation used

- [Wiibrew](https://wiibrew.org/wiki/Wiimote)
- [WiimoteEmulator](https://github.com/JRogaishio/WiimoteEmulator/tree/master)

## Future plans

- Clean the code
- Optimizations
- Make Power button work
- Automatic Reconnection
- Add wii connector for I2C addons (like Nunchuck, wii motion plus and more)

## Technical support and feedback

Feel free to ask in the issues tab. 
