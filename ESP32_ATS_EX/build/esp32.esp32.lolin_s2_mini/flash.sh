#!/bin/bash
esptool.py --port /dev/ttyACM0 write_flash 0x00000 ESP32_ATS_EX.ino.merged.bin
