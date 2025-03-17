#!/bin/bash

cd /home/deck/Documents/GitHub/esp32-wii-controller/
. $HOME/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py monitor
