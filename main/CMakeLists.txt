set(COMPONENT_REQUIRES "Adafruit_ADXL345" "Adafruit-GFX-Library" "Adafruit_SSD1306" "VL53L1X")
idf_component_register(SRCS "wii_remote_host.c" "wii_mitm.c" "main.cpp" "wii_controller.c" "fake_wii_remote.c"
                    INCLUDE_DIRS ""
                    REQUIRES "bthci" "nvs_flash" "bt" "arduino-esp32")
