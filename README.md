# ESP32 Wii Controller Arduino + components (unused)

Connect an ESP32 to a wii or a wiimote using FreeRTOS.
Thanks to nickbits1024 for the original work, this version is just the same but for ESP-IDF 5.3.1

## Tools used :

- ESP-IDF v5.3.1
- Python 3.11.7
- ESP32 Dev Module NodeMCU from Joy-It

## How to build

Install ESP-IDF :

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)

Then run in the project folder:
- cd components
  
  (rename arduino-esp32 before)
- git clone --recursive https://github.com/espressif/arduino-esp32.git
- cd arduino-esp32
- git checkout 3.1.0-RC2
  
  (overwrite arduino-esp32 with the one from this branch)

And in the root :
- idf.py build
- idf.py flash -p [COM port]

optional :
- idf.py monitor

## How to use

Press BOOT button to see the menu home

## Technical support and feedback

Feel free to ask in the issues tab. 
