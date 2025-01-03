#!/bin/bash

cd /home/deck/Documents/PlatformIO/esp32-wii-controller-with-components/
. $HOME/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py monitor
