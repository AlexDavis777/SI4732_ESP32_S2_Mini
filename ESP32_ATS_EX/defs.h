#pragma once

//If you set this def to 0 project will be compiled without RDS 
//and everything related to RDS will be excluded from build
#define USE_RDS 1

#define EEPROM_APP_ID				235
#define EEPROM_DATA_START_ADDRESS	1
#define EEPROM_VERSION_ADDRESS      1000
#define EEPROM_APP_ID_ADDRESS       0
#define EEPROM_SIZE        512

//EEPROM Settings
#define STORE_TIME 30000 // Inactive time to save our settings

// OLED Const values
#define DEFAULT_FONT FONT8X16POB
#define RST_PIN -1

//#define RESET_PIN 12
#define RESET_PIN 35 //esp32

//Battery charge monitoring analog pin (Voltage divider 10-10 KOhm directly from battery)
#define BATTERY_VOLTAGE_PIN 12

#define speakerPin 1  // Define the pin connected to the speaker or buzzer

// Encoder
#define ENCODER_PIN_A 8
#define ENCODER_PIN_B 6

// Buttons
#define MODE_SWITCH      38 
#define BANDWIDTH_BUTTON 36
#define VOLUME_BUTTON    34
#define AVC_BUTTON       21
#define BAND_BUTTON      17 
#define SOFTMUTE_BUTTON  15
#define AGC_BUTTON       13
#define STEP_BUTTON      14

#define ENCODER_BUTTON   10

// Default values
#define MIN_ELAPSED_TIME 100
#define MIN_ELAPSED_RSSI_TIME 150
#define DEFAULT_VOLUME 15
#define ADJUSTMENT_ACTIVE_TIMEOUT 3000

// Band settings
#define SW_LIMIT_LOW		1710
#define SW_LIMIT_HIGH		30000
#define LW_LIMIT_LOW		153
#define M160_LIMIT_LOW		1700
#define M160_LIMIT_HIGH		3499
#define M80_LIMIT_LOW		3500
#define M80_LIMIT_HIGH		4500
#define M40_LIMIT_LOW		6800
#define M40_LIMIT_HIGH		7300
#define M30_LIMIT_LOW		10000
#define M30_LIMIT_HIGH		11200
#define M20_LIMIT_LOW		14000
#define M20_LIMIT_HIGH		14500
#define M17_LIMIT_LOW		18000
#define M17_LIMIT_HIGH		18300
#define M15_LIMIT_LOW		21000
#define M15_LIMIT_HIGH		21900
#define M12_LIMIT_LOW		24890
#define M12_LIMIT_HIGH		26199
#define CB_LIMIT_LOW		26200
#define CB_LIMIT_HIGH		28000

#define BAND_DELAY                 3
#define VOLUME_DELAY               2 

#define buttonEvent                NULL
