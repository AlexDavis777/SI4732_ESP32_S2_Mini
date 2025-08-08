#!/bin/bash
esptool --port /dev/ttyACM0 write_flash 0x00000 ESP32_OLED_CHINESE.ino.merged.bin
