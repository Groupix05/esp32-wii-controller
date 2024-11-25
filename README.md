# ESP32 Wii Controller Testing

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
- idf.py build
- idf.py flash -p [COM port]

optional :
- idf.py monitor

## How to use

Just power your ESP32, then press the wii sync button and see home button in action.

## Technical support and feedback

Feel free to ask in the issues tab. 
