#!/bin/bash
esptool --port /dev/ttyACM0 write_flash 0x00000 ESP32_SI473X_ALL_IN_ONE_OLED_CT2JWF_EEPROM_RDS_v8.0.1.ino.merged.bin
