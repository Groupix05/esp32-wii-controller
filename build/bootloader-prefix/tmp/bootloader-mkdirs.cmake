# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/deck/esp/esp-idf/components/bootloader/subproject"
  "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader"
  "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader-prefix"
  "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader-prefix/tmp"
  "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader-prefix/src/bootloader-stamp"
  "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader-prefix/src"
  "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/deck/Documents/GitHub/esp32-wii-controller/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
