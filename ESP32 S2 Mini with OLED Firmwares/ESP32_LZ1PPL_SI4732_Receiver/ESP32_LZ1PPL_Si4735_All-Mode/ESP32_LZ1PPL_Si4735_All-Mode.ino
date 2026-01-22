/*Environment
Arduino IDE 2.3.7
//STM32 Board manager 2.7.0
PU2CLR Si4735 2.1.8
Etherkit Si5351 2.1.4 https://github.com/etherkit/Si5351Arduino
Adafruit SSD1306 version=2.4.6
*/
#include <Rotary.h>
#include <SI4735.h>
#include <si5351.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "patch_init.h"
//#include "patch_full.h" // SSB patch for whole SSBRX initialization string
#define ESP32_I2C_SDA 39
#define ESP32_I2C_SCL 40
#define I2C_ADDRESS   0x3C    // OLED I2C address
#define SCREEN_WIDTH  128     // OLED display width, in pixels
#define SCREEN_HEIGHT  64     // OLED display height, in pixels
#define OLED_RESET    4       // Reset pin # (or -1 if sharing Arduino reset pin)
#define MIN_ELAPSED_TIME  150
#define ELAPSED_COMMAND  3000
#define IF_FM  65700          //72300 //Enter your IF frequency, from: 64000 to 108000, 0 = to direct convert receiver or RF generator, + will add and - will subtract IF offfset.
#define IF  10700             //Enter your IF frequency, ex: 455 = 455kHz, 10700 = 10.7MHz, 0 = to direct convert receiver or RF generator, + will add and - will subtract IF offfset.
//#define FREQ_INIT  99410000    //Enter your initial frequency at startup, ex: 7000000 = 7MHz, 10000000 = 10MHz, 840000 = 840kHz.
#define FREQ_INIT  197000    //Enter your initial frequency at startup, ex: 7000000 = 7MHz, 10000000 = 10MHz, 840000 = 840kHz.
#define XT_CAL_F  11100       //Si5351 calibration factor, adjust to get exatcly 10MHz. 
//#define XT_CAL_F  0
#define XTAL_F  25000000      //Xtal frequency for Si5351
//#define EEPROM_I2C_ADDR  0x50 // You might need to change this value , if pin 1,2,3 is GND (A0,A1,A2 = GND) most EEPROM address = 0x50 , 1010 A2 A1 A0 = 1010000 = 0x50
#define ENCODER_BUTTON 10   // Used to select the enconder control (BFO or VFO) and SEEK function on AM and FM modes
#define MODE_SWITCH 38       // Switch MODE (Am/LSB/USB)
#define BANDWIDTH_BUTTON 36  // Used to select the banddwith.
#define VOLUME_BUTTON 34     // Volume Up
#define AVC_BUTTON 21        // **** Use this button to implement a new function
#define BAND_BUTTON 17       // Next band
#define SOFTMUTE_BUTTON 15   // **** Use this button to implement a new function
#define AGC_BUTTON 13       // Switch AGC ON/OF
#define STEP_BUTTON 14      // Used to select the increment or decrement frequency step (see tabStep)
#define ENCODER_BUTTON 10   // Used to select the enconder control (BFO or VFO) and SEEK function on AM and FM modes
#define MIN_ELAPSED_RSSI_TIME 150
#define RESET_PIN 35        // Si473x reset pin
#define EEPROM_SIZE 512
#define EEPROM_APP_ID 23
#define EEPROM_APP_ID_ADDRESS 0
#define EEPROM_DATA_START_ADDRESS 1
//EEPROM Settings
#define STORE_TIME 10000 // Inactive time to save our settings

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Rotary r = Rotary(PA5, PA4);
#define ENCODER_PIN_A 6
#define ENCODER_PIN_B 8
SI4735 si4735;
Si5351 si5351;
const int eeprom_address = 0;
const uint16_t size_content = sizeof ssb_patch_content;

volatile int encoder0Pos = 0;
int valF;
byte MenueEnable = 0;
byte menuCount = 1;
byte dir = 0;
byte ssbload = 0;
bool runState = false;

const char *currentMode = "AM";
const char *BandWidth;
const char *agcNdx;
int volume = 25;
int currentBFO = 126;
int ATT;
int Mo = 2;
bool agc = 0;
long elapsedCommand = millis();
long storeTime = millis();
uint8_t countClick = 0;
unsigned long freq = FREQ_INIT;
unsigned long freqold, fstep;
unsigned long firstv, secondv;
long interfreq = IF;
long interfreq_FM = IF_FM;
long cal = XT_CAL_F;
byte encoder = 1;
byte stp;
unsigned int period = 100;   //millis display active
unsigned long time_now = 0;  //millis display active
String steps;

void setup() {
  Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);
  // Encoder pins
  pinMode(ENCODER_BUTTON, INPUT_PULLUP); 
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  
  pinMode(38, INPUT_PULLUP); //add Mode Switch 
  pinMode(36, INPUT_PULLUP); //add Bandwidth Switch 
  pinMode(14, INPUT_PULLUP); //add Step Switch
  pinMode(34, INPUT_PULLUP); //add vol+ Switch 
  pinMode(21, INPUT_PULLUP); //add vol- Switch
  pinMode(17, INPUT_PULLUP); //add band+ Switch 
  pinMode(15, INPUT_PULLUP); //add avc Switch
  pinMode(13, INPUT_PULLUP); //add AGC Switch
  
  EEPROM.begin(EEPROM_SIZE);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(32, 0);
  display.print("LZ1PPL");
  display.display();
  delay(200);
  display.setCursor(44, 20);
  display.setTextSize(1);
  display.print("All Mode");
  display.setCursor(47, 32);
  display.print("Receiver");
  display.setCursor(29, 45);
  display.print("Si4735-Si5351");
  display.display();
  delay(500);

  display.clearDisplay();
  EEPROM.begin(EEPROM_SIZE);
  // If you want to reset the eeprom, keep the ENCODER_BUTTON button pressed during startup
  if (digitalRead(ENCODER_BUTTON) == LOW)
  {
    EEPROM.write(eeprom_address, 0);
    EEPROM.commit();
    display.setCursor(30, 32);
    display.print("EEPROM RESET");
    display.display();
    delay(2000);
    display.clearDisplay();
  }
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 25000000, XT_CAL_F);
  si5351.output_enable(SI5351_CLK0, 1);                  //1 - Enable / 0 - Disable CLK
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA);  //Output current 2MA, 4MA, 6MA or 8MA
  //si4735.setup(RESET_PIN, 1);

  si4735.setI2CFastMode(); // Set I2C bus speed.
  si4735.getDeviceI2CAddress(RESET_PIN); // Looks for the I2C bus address and set it.  Returns 0 if error
  //si4735.setup(RESET_PIN, -1, 1, SI473X_ANALOG_AUDIO);
  si4735.setup(RESET_PIN, 1);


    //Load settings from EEPROM
    if (EEPROM.read(EEPROM_APP_ID_ADDRESS) == EEPROM_APP_ID){   
        readAllReceiverInformation();
    }else
        saveAllReceiverInformation();
  delay(300);
  int fst = 5;
  if (fst == 1)
  {
    fstep = 1;
    steps = "1Hz";
  }
  if (fst == 2)
  {
    fstep = 10;
    steps = "10Hz";
  }    if (fst == 3)
  {
    fstep = 100;
    steps = "100Hz";
  }    if (fst == 4)
  {
    fstep = 1000;
    steps = "1kHz";
  }
  if (fst == 5)
  {
    fstep = 5000;
    steps = "5kHz";
  }
  if (fst == 6)
  {
    fstep = 10000;
    steps = "10kHz";
  }
  if (fst == 7)
  {
    fstep = 100000;
    steps = "100kHz";
  }
  if (fst == 8)
  {
    fstep = 1000000;
    steps = "1MHz";
  }

  if (Mo == 1)
  {
    currentMode = "FM";
    //si4735.reset();
    si4735.setTuneFrequencyAntennaCapacitor(0);
    si4735.setFM(6500, 6600, IF_FM / 10, 1);
    ssbload = 0;
  }
  if (Mo == 2)
  {
    currentMode = "AM";
    //si4735.reset();
    si4735.setTuneFrequencyAntennaCapacitor(1);
    si4735.setAM(10650, 10750, IF, 1);
    ssbload = 0;
  }
  if (Mo == 3)
  {
    currentMode = "LSB";
    /*
      if (ssbload == 0)
      {
      loadSSB();
      ssbload = 1;
      }
    */
    si4735.reset();
    si4735.setTuneFrequencyAntennaCapacitor(1);
    si4735.loadPatch(ssb_patch_content, size_content, 1);
    si4735.setSSB(10650, 10750, IF, 1, 2);
    si4735.setSSBAutomaticVolumeControl(1);
  }
  if (Mo == 4)
  {
    currentMode = "USB";
    /*
      if (ssbload == 0)
      {
      loadSSB();
      ssbload = 1;
      }
    */
    si4735.reset();
    si4735.setTuneFrequencyAntennaCapacitor(1);
    si4735.loadPatch(ssb_patch_content, size_content, 2);
    si4735.setSSB(10650, 10750, IF, 1, 1);
    si4735.setSSBAutomaticVolumeControl(1);
  }
  int Filter = 4;
  if (Filter == 1)
  {
    if (currentMode == "AM")
    {
      si4735.setBandwidth(4, 1);
      BandWidth = "1kHz";
      delay(200);
    }
    if (currentMode == "LSB" || currentMode == "USB")
    {
      si4735.setBandwidth(4, 1);
      si4735.setSSBAudioBandwidth(4);
      si4735.setSBBSidebandCutoffFilter(0);
      BandWidth = "0.5kHz";
    }
    if (currentMode == "FM")
    {
      si4735.setFmBandwidth(0);
      BandWidth = "Auto";
      delay (200);
    }
  }
  if (Filter == 2)
  {
    if (currentMode == "AM")
    {
      si4735.setBandwidth(3, 1);
      BandWidth = "2kHz";
      delay(200);
    }
    if (currentMode == "LSB" || currentMode == "USB")
    {
      si4735.setBandwidth(4, 1);
      si4735.setSSBAudioBandwidth(5);
      si4735.setSBBSidebandCutoffFilter(0);
      BandWidth = "1kHz";
    }
    if (currentMode == "FM")
    {
      si4735.setFmBandwidth(1);
      BandWidth = "110kHz";
      delay (200);
    }
  }
  if (Filter == 3)
  {
    if (currentMode == "AM")
    {
      si4735.setBandwidth(6, 1);
      BandWidth = "2.5kHz";
      delay(200);
    }
    if (currentMode == "LSB" || currentMode == "USB")
    {
      si4735.setBandwidth(3, 1);
      si4735.setSSBAudioBandwidth(0);
      si4735.setSBBSidebandCutoffFilter(0);
      BandWidth = "1.2kHz";
    }
    if (currentMode == "FM")
    {
      si4735.setFmBandwidth(2);
      BandWidth = "84kHz";
      delay (200);
    }
  }
  if (Filter == 4)
  {
    if (currentMode == "AM")
    {
      si4735.setBandwidth(2, 1);
      BandWidth = "3kHz";
      delay(200);
    }
    if (currentMode == "LSB" || currentMode == "USB")
    {
      si4735.setBandwidth(6, 1);
      si4735.setSSBAudioBandwidth(1);
      si4735.setSBBSidebandCutoffFilter(1);
      BandWidth = "2.2kHz";
    }
    if (currentMode == "FM")
    {
      si4735.setFmBandwidth(3);
      BandWidth = "60kHz";
      delay (200);
    }
  }
  if (Filter == 5)
  {
    if (currentMode == "AM")
    {
      si4735.setBandwidth(1, 1);
      BandWidth = "4kHz";
      delay(200);
    }
    if (currentMode == "LSB" || currentMode == "USB")
    {
      si4735.setBandwidth(2, 1);
      si4735.setSSBAudioBandwidth(2);
      si4735.setSBBSidebandCutoffFilter(1);
      BandWidth = "3kHz";
    }
    if (currentMode == "FM")
    {
      si4735.setFmBandwidth(4);
      BandWidth = "40kHz";
      delay (200);
    }
  }
  if (Filter == 6)
  {
    if (currentMode == "AM")
    {
      si4735.setBandwidth(0, 1);
      BandWidth = "6kHz";
      delay(200);
    }
    if (currentMode == "LSB" || currentMode == "USB")
    {
      si4735.setBandwidth(1, 1);
      si4735.setSSBAudioBandwidth(3);
      si4735.setSBBSidebandCutoffFilter(1);
      BandWidth = "4kHz";
    }
  }
  si4735.setVolume (volume);
  int agc_ee = 1;
  if (agc_ee == 1)
  {
    si4735.setAutomaticGainControl(0, 0);
    agc = 1;
    delay(200);
  }
  if (agc_ee == 2)
  {
    si4735.setAutomaticGainControl(1, 0);
    agc = 0;
    delay(200);
  }
  int bfo_ee = 126;
  currentBFO = map(bfo_ee, 0, 255, -127, 127);
  currentBFO = currentBFO * 10;
  si4735.setSSBBfo(currentBFO);

  //si4735.setRefClock(32768);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), read_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), read_encoder, CHANGE);
  
 
} 
//end of setup()

 //EEPROM Save
void saveAllReceiverInformation()
{
    uint8_t addr = EEPROM_DATA_START_ADDRESS;
    EEPROM.write(EEPROM_APP_ID_ADDRESS, EEPROM_APP_ID);
    volume = si4735.getVolume();
    EEPROM.write(addr++, volume);
    EEPROM.write(addr++, Mo);
    EEPROM.writeLong(addr++, freq);
    EEPROM.commit();
}

//EEPROM Load
void readAllReceiverInformation()
{
    uint8_t addr = EEPROM_DATA_START_ADDRESS;
    volume = EEPROM.read(addr++);
    Mo = EEPROM.read(addr++);
    freq = EEPROM.readLong(addr++);
    freqold = freq;
}


void read_encoder() {
  // Encoder interrupt routine for both pins. Updates counter
  // if they are valid and have rotated a full indent
  
  static uint8_t old_AB = 3;  // Lookup table index
  static int8_t encval = 0;   // Encoder value  
  static const int8_t enc_states[]  = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}; // Lookup table
 
  old_AB <<=2;  // Remember previous state
 
  if (digitalRead(ENCODER_PIN_A)) old_AB |= 0x02; // Add current state of pin A
  if (digitalRead(ENCODER_PIN_B)) old_AB |= 0x01; // Add current state of pin B
   
  encval += enc_states[( old_AB & 0x0f )];
 
  // Update counter if encoder has rotated a full indent, that is at least 4 steps
  if( encval > 3 ) {        // Four steps forward
    encoder0Pos++;              // Increase counter
    set_frequency(1);
    encval = 0;
  }
  else if( encval < -3 ) {  // Four steps backwards
    encoder0Pos--;               // Decrease counter
    set_frequency(-1);
    encval = 0;
  }
}

void butt()
{
  countClick++;
  delay (MIN_ELAPSED_TIME);
  elapsedCommand = millis();
  //firstv = millis();
  if (digitalRead(ENCODER_BUTTON) == LOW)
  {
  up();
  }
}

void up()
{
  secondv = millis();
  if ((secondv - elapsedCommand) >= 4000) {
    firstv = secondv;
    //EEPROM.put(10, freq);
  }
  if (digitalRead(ENCODER_BUTTON) == LOW)
  {
  butt();
  }
}

void loadSSB()
{
  si4735.setI2CFastMode(); // Recommended
  //si4735_eeprom_patch_header eep;
  //si4735.reset();
  si4735.queryLibraryId(); // Is it really necessary here? I will check it.
  si4735.patchPowerUp();
  delay(50);
  si4735.downloadPatch(ssb_patch_content, size_content);
  si4735.setI2CStandardMode(); // goes back to default (100kHz)
  // Parameters
  // AUDIOBW - SSB Audio bandwidth; 0 = 1.2kHz (default); 1=2.2kHz; 2=3kHz; 3=4kHz; 4=500Hz; 5=1kHz;
  // SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)
  // AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
  // AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
  // SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
  // DSP_AFCDIS - DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.
  si4735.setSSBConfig(1, 0, 0, 1, 0, 1);
  delay(25);
}

void set_frequency(short dir) {
  //Up/Down frequency
  if (MenueEnable == 0)
  {
    if (dir == 1) freq = freq + fstep;
    if (freq >= 230000000) freq = 230000000;
    if (dir == -1) freq = freq - fstep;
    if (fstep == 1000000 && freq <= 1000000) freq = 1000000;
    else if (freq <= 100000) freq = 100000;
  }
}

void tunegen() {
  if (MenueEnable == 0)
  {
    if (currentMode == "FM")
    {
      if (freq >= 170000000)
      {
        si5351.set_freq((freq - (interfreq_FM * 1000ULL)) * 100ULL, SI5351_CLK0);
      }
      if (freq < 170000000)
      {
        si5351.set_freq((freq + (interfreq_FM * 1000ULL)) * 100ULL, SI5351_CLK0);
      }
    }
    else
    {
      si5351.set_freq((freq + (interfreq * 1000ULL)) * 100ULL, SI5351_CLK0);
    }
   storeTime = millis();
  }
}

void staticMenu() {
  display.setTextSize(1);
  display.clearDisplay();
  display.setCursor(10, 20);
  display.print("Step:");
  display.setCursor(80, 20);
  display.print(steps);

  display.setCursor(10, 30);
  display.print("BandWidth:");
  display.setCursor(80, 30);
  display.print(BandWidth);

  display.setCursor(10, 40);
  display.print("BFO:");
  display.setCursor(80, 40);
  display.print(currentBFO);
  display.setCursor(2, (menuCount * 10)+10);
  display.print(">");

  if ((millis() - elapsedCommand) > ELAPSED_COMMAND)
  {
    countClick = 0;
    MenueEnable = 0;
  }
  menuCheck();
  display.display();
}

void menuCheck() {
  if (digitalRead(AGC_BUTTON) == LOW && menuCount < 3) {
    delay(MIN_ELAPSED_TIME);
    menuCount++;
    elapsedCommand = millis();
    //EEPROM.write(menuCount - 1, encoder0Pos);
    //encoder0Pos = EEPROM.read(menuCount);
    display.setCursor(2, (menuCount) - 1);
    display.print(" ");
  }
  if (digitalRead(AGC_BUTTON) == LOW && menuCount >= 3) {
    delay(MIN_ELAPSED_TIME);
    menuCount = 1;
    elapsedCommand = millis();
    //EEPROM.write(6, encoder0Pos);
    //encoder0Pos = EEPROM.read(menuCount);
    encoder0Pos = 1;
    }
  if (menuCount == 1) {        // Steps
    if (encoder0Pos == 1)
    {
      fstep = 1;
      steps = "1Hz";
    }
    if (encoder0Pos == 2)
    {
      fstep = 10;
      steps = "10Hz";
    }    if (encoder0Pos == 3)
    {
      fstep = 100;
      steps = "100Hz";
    }    if (encoder0Pos == 4)
    {
      fstep = 1000;
      steps = "1kHz";
    }
    if (encoder0Pos == 5)
    {
      fstep = 5000;
      steps = "5kHz";
    }
    if (encoder0Pos == 6)
    {
      fstep = 10000;
      steps = "10kHz";
    }
    if (encoder0Pos == 7)
    {
      fstep = 100000;
      steps = "100kHz";
    }
    if (encoder0Pos == 8)
    {
      fstep = 1000000;
      steps = "1MHz";
    }
    if (encoder0Pos < 1)
    {
      encoder0Pos = 8;
    }
    if (encoder0Pos > 8)
    {
      encoder0Pos = 1;
    }
  }
  if (menuCount == 2) {            // Set Bandwidth
    if (encoder0Pos == 1)
    {
      if (currentMode == "AM")
      {
        if (BandWidth != "1kHz")
        {
          si4735.setBandwidth(4, 1);
          BandWidth = "1kHz";
          delay (200);
        }
      }
      if (currentMode == "LSB" || currentMode == "USB")
      {
        if (BandWidth != "0.5kHz")
        {
          si4735.setBandwidth(4, 1);
          si4735.setSSBAudioBandwidth(4);
          si4735.setSBBSidebandCutoffFilter(0);
          BandWidth = "0.5kHz";
          delay (200);
        }
      }
      if (currentMode == "FM")
      {
        if (BandWidth != "Auto")
        {
          si4735.setFmBandwidth(0);
          BandWidth = "Auto";
          delay (200);
        }
      }
    }
    if (encoder0Pos == 2)
    {
      if (currentMode == "AM")
      {
        if (BandWidth != "2kHz")
        {
          si4735.setBandwidth(3, 1);
          BandWidth = "2kHz";
          delay (200);
        }
      }
      if (currentMode == "LSB" || currentMode == "USB")
      {
        if (BandWidth != "1kHz")
        {
          si4735.setBandwidth(4, 1);
          si4735.setSSBAudioBandwidth(5);
          si4735.setSBBSidebandCutoffFilter(0);
          BandWidth = "1kHz";
          delay (200);
        }
      }
      if (currentMode == "FM")
      {
        if (BandWidth != "110kHz")
        {
          si4735.setFmBandwidth(1);
          BandWidth = "110kHz";
          delay (200);
        }
      }
    }
    if (encoder0Pos == 3)
    {
      if (currentMode == "AM")
      {
        if (BandWidth != "2.5kHz")
        {
          si4735.setBandwidth(6, 1);
          BandWidth = "2.5kHz";
          delay (200);
        }
      }
      if (currentMode == "LSB" || currentMode == "USB")
      {
        if (BandWidth != "1.2kHz")
        {
          si4735.setBandwidth(3, 1);
          si4735.setSSBAudioBandwidth(0);
          si4735.setSBBSidebandCutoffFilter(0);
          BandWidth = "1.2kHz";
          delay (200);
        }
      }
      if (currentMode == "FM")
      {
        if (BandWidth != "84kHz")
        {
          si4735.setFmBandwidth(2);
          BandWidth = "84kHz";
          delay (200);
        }
      }
    }
    if (encoder0Pos == 4)
    {
      if (currentMode == "AM")
      {
        if (BandWidth != "3kHz")
        {
          si4735.setBandwidth(2, 1);
          BandWidth = "3kHz";
          delay (200);
        }
      }
      if (currentMode == "LSB" || currentMode == "USB")
      {
        if (BandWidth != "2.2kHz")
        {
          si4735.setBandwidth(6, 1);
          si4735.setSSBAudioBandwidth(1);
          si4735.setSBBSidebandCutoffFilter(1);
          BandWidth = "2.2kHz";
          delay (200);
        }
      }
      if (currentMode == "FM")
      {
        if (BandWidth != "60kHz")
        {
          si4735.setFmBandwidth(3);
          BandWidth = "60kHz";
          delay (200);
        }
      }
    }
    if (encoder0Pos == 5)
    {
      if (currentMode == "AM")
      {
        if (BandWidth != "4kHz")
        {
          si4735.setBandwidth(1, 1);
          BandWidth = "4kHz";
          delay (200);
        }
      }
      if (currentMode == "LSB" || currentMode == "USB")
      {
        if (BandWidth != "3kHz")
        {
          si4735.setBandwidth(2, 1);
          si4735.setSSBAudioBandwidth(2);
          si4735.setSBBSidebandCutoffFilter(1);
          BandWidth = "3kHz";
          delay (200);
        }
      }
      if (currentMode == "FM")
      {
        if (BandWidth != "40kHz")
        {
          si4735.setFmBandwidth(4);
          BandWidth = "40kHz";
          delay (200);
        }
      }
    }
    if (encoder0Pos == 6)
    {
      if (currentMode == "AM")
      {
        if (BandWidth != "6kHz")
        {
          si4735.setBandwidth(0, 1);
          BandWidth = "6kHz";
          delay (200);
        }
      }
      if (currentMode == "LSB" || currentMode == "USB")
      {
        if (BandWidth != "4kHz")
        {
          si4735.setBandwidth(1, 1);
          si4735.setSSBAudioBandwidth(3);
          si4735.setSBBSidebandCutoffFilter(1);
          BandWidth = "4kHz";
          delay (200);
        }
      }
    }
    if (encoder0Pos < 1)
    {
      encoder0Pos = 6;
    }
    if (encoder0Pos > 6)
    {
      encoder0Pos = 1;
    }
  }
  if (menuCount == 3) {             // Set BFO
    if (currentMode == "LSB" || currentMode == "USB")

    {
      currentBFO = map(encoder0Pos, 0, 255, -127, 127);
      currentBFO = currentBFO * 10;
      si4735.setSSBBfo(currentBFO);
    }
  }
}

void displayfreq() {
  unsigned int m = freq / 1000000;
  unsigned int k = (freq % 1000000) / 1000;
  unsigned int h = (freq % 1000) / 1;
  display.setTextSize(2);
  display.clearDisplay();

  char buffer[15] = "";
  if (m < 1) {
    display.setCursor(0, 2); sprintf(buffer, "%003d.%003d", k, h);
  }
  else if (m < 100) {
    display.setCursor(0, 2); sprintf(buffer, "%2d.%003d.%003d", m, k, h);
  }
  else if (m >= 100) {
    unsigned int h = (freq % 1000) / 10;
    display.setCursor(0, 2); sprintf(buffer, "%2d.%003d.%02d", m, k, h);
  }
  display.print(buffer);
  display.drawFastHLine(0, 0, 128, SSD1306_WHITE);
  display.drawFastHLine(0, 17, 128, SSD1306_WHITE);
  display.drawFastVLine(127, 0, 17, SSD1306_WHITE);
  display.drawFastVLine(0, 0, 17, SSD1306_WHITE);
}

void displayset()
{
  display.setTextSize(1);
  display.setCursor(2, 21);
  display.print("BFO:");
  display.print(currentBFO);
  display.print("Hz");
  display.setCursor(78, 21);
  display.print("Mode:");
  display.print(currentMode);
  display.drawFastHLine(0, 30, 128, SSD1306_WHITE);
  display.drawFastVLine(67, 17, 13, SSD1306_WHITE);
  display.drawFastVLine(0, 17, 13, SSD1306_WHITE);
  display.drawFastVLine(127, 17, 13, SSD1306_WHITE);
  display.setCursor(2, 35);
  display.print("Step:");
  display.print(steps);
  display.setCursor(78, 35);
  display.print("Vol:");
  display.print(si4735.getVolume());
  display.setCursor(2, 46);
  display.print("BW:");
  display.print(BandWidth);
  display.setCursor(78, 46);
  if (agc == 1) {
    display.print("AGC ON");
  }
  else {
    display.print("AGC OFF");
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Bargraph as S Meter.                                                          //
  //Comment this and uncoment below to show on dispaly SNR/RSSI                    //
  ///////////////////////////////////////////////////////////////////////////////////
  int rssiAux = 0;
  si4735.getCurrentReceivedSignalQuality();
  int rssi = si4735.getCurrentRSSI();
  int val =  map(rssi, 0, 60, 0, 99);
  val = constrain(val, 0, 99);
  if (rssi == 0)
    rssiAux = 1;
  else if (rssi < 1)
    rssiAux = 2;
  else if (rssi < 2)
    rssiAux = 3;
  else if (rssi < 3)
    rssiAux = 4;
  else if (rssi < 6)
    rssiAux = 5;
  else if (rssi < 12)
    rssiAux = 6;
  else if (rssi < 25)
    rssiAux = 7;
  else if (rssi < 50)
    rssiAux = 8;
  else
    rssiAux = 9;
  display.drawRect(2, 57, 99, 7, 1); //Border of the bar chart
  display.fillRect(2, 57, val, 7, WHITE); //Draws the bar depending on the sensor value
  display.setCursor(105, 57);
  display.print("S");
  display.print(rssiAux);
  if (rssi > 60)
  {
    display.print("+");
  }

  /*
    display.setCursor(2, 57);
    si4735.getCurrentReceivedSignalQuality();
    display.print("SNR/RSSI:");
    display.print(si4735.getCurrentSNR());
    display.print("/");
    display.print(si4735.getCurrentRSSI());
  */

  display.display();
}

void loop() {
  if (digitalRead(ENCODER_BUTTON) == LOW)
  {
  butt();
  }
  if (countClick >= 2)
  {
    MenueEnable = 1;
  }
  if (MenueEnable == 1)
  {
    staticMenu();
  }

  if (MenueEnable == 0)
  {
    if (freqold != freq) {
      runState = false;
      tunegen();
      runState = true;
      freqold = freq;
    }
    //Save EEPROM if anough time passed and frequency changed
    if ((millis() - storeTime) > STORE_TIME)
    {
      if (runState == true)
       {  
       saveAllReceiverInformation();
       runState = false;
       display.setCursor(56, 46);
       display.print("*");
       display.display();
       delay(4000);
       }
    }
    bwCheck();      //check BandWidth button
    bupCheck();    //check Band Up button
    stepCheck();    //check STEP button
    modeCheck();    //check MODE button
    vupCheck();     //check V+ button
    vdownCheck();   //check V- button
    agcCheck();     //check AGC button
    displayfreq();
    displayset();
  }
  if (MenueEnable == 0)
  {
    if ((millis() - elapsedCommand) >= 1000)
    {
      countClick = 0;
    }
  }
  delay(10);
}

/* add two button for quick change STEP or MODE *********hs0nnu**/
void bwCheck() {
    if (digitalRead(BANDWIDTH_BUTTON) == LOW && (BandWidth == "1kHz")) {
      if (currentMode == "AM")
      {
       si4735.setBandwidth(2, 1);
       BandWidth = "3kHz";
       delay (300);
      }
  }
    if (digitalRead(BANDWIDTH_BUTTON) == LOW && (currentMode == "FM")) {
      {
       si4735.setFmBandwidth(0);
       BandWidth = "Auto";
       delay (300);
      }
  }
      if (digitalRead(BANDWIDTH_BUTTON) == LOW && (BandWidth == "3kHz")) {
      if (currentMode == "AM")
      {
       si4735.setBandwidth(0, 1);
       BandWidth = "6kHz";
       delay (300);
      }
  }
  else if (digitalRead(BANDWIDTH_BUTTON) == LOW && (BandWidth == "6kHz")) {
     if (currentMode == "AM")
     {
      si4735.setBandwidth(4, 1);
      BandWidth = "1kHz";
      delay (300);
     }
  }
  
}    
void bupCheck() {
  if (digitalRead(BAND_BUTTON) == LOW){
     currentMode = "AM";
	 si4735.setTuneFrequencyAntennaCapacitor(1);
     freq = 198000;
     si4735.setAM(100, 28400, freq/1000, 1);
     si4735.setBandwidth(1, 1);
     BandWidth = "4kHz";
     ssbload = 0;
     Mo = 2;
  }
  if (digitalRead(SOFTMUTE_BUTTON) == LOW) {
     currentMode = "FM";
	 si4735.setTuneFrequencyAntennaCapacitor(1);
     freq = 99400000;
     si4735.setFM(6400, 10800, freq/10000, 1);
     ssbload = 0;
     Mo = 1;
  }
}    
void vupCheck() {
  if (digitalRead(VOLUME_BUTTON) == LOW) {
        si4735.volumeUp();
        delay(40);
  }
}
void vdownCheck() {
  if (digitalRead(AVC_BUTTON) == LOW) {
        si4735.volumeDown();
        delay(40);
  }
}
void agcCheck() {
  if (digitalRead(AGC_BUTTON) == LOW) {
      if (agc != 0)
      {
        si4735.setAutomaticGainControl(1, 0);
        agc = 0;
      }
      else
      {
        si4735.setAutomaticGainControl(0, 0);
        agc = 1;
      }
      delay(300);
  }
}
void modeCheck() {
    if (digitalRead(MODE_SWITCH) == LOW && (currentMode == "FM")) {
    currentMode = "AM";
	si4735.setTuneFrequencyAntennaCapacitor(1);
    si4735.setAM(10650, 10750, IF, 1);
    //si4735.setAM(100, 28400, freq/1000, 1);
    ssbload = 0;
    Mo = 2;
  }
    if (digitalRead(MODE_SWITCH) == LOW && (currentMode == "AM")) {
    currentMode = "LSB";
    si4735.reset();
    si4735.setTuneFrequencyAntennaCapacitor(1);
    si4735.loadPatch(ssb_patch_content, size_content, 1);
    si4735.setSSB(10650, 10750, IF, 1, 2);
    //si4735.setSSB(100, 10750, freq/1000, 1, 2);
    si4735.setSSBAutomaticVolumeControl(1);
    Mo = 3;
  }
	if (digitalRead(MODE_SWITCH) == LOW && (currentMode == "LSB")) {
    currentMode = "USB";
	  si4735.reset();
    si4735.setTuneFrequencyAntennaCapacitor(1);
    si4735.loadPatch(ssb_patch_content, size_content, 2);
    si4735.setSSB(10650, 10750, IF, 1, 1);
    //si4735.setSSB(100, 10750, freq/1000, 1, 1);
    si4735.setSSBAutomaticVolumeControl(1);
    Mo = 4;
  }
  	else if (digitalRead(MODE_SWITCH) == LOW && (currentMode == "USB")) {
    currentMode = "FM";
	si4735.setTuneFrequencyAntennaCapacitor(0);
    si4735.setFM(6500, 6600, IF_FM / 10, 1);
    //si4735.setFM(6400, 10800, freq/10000, 1);
    ssbload = 0;
    Mo = 1;
  }
}
void stepCheck() {
if (digitalRead(STEP_BUTTON) == LOW && (fstep == 1)) {
	fstep = 1000000;
	steps = "1MHz";
	delay (300);
	}
  if (digitalRead(STEP_BUTTON) == LOW && (fstep == 1000000)) {
   fstep = 100000;
    steps = "100kHz";
    delay (300);
  }
    if (digitalRead(STEP_BUTTON) == LOW && (fstep == 100000)) {
   fstep = 10000;
    steps = "10kHz";
    delay (300);
  }
    if (digitalRead(STEP_BUTTON) == LOW && (fstep == 10000)) {
   fstep = 5000;
    steps = "5kHz";
    delay (300);
  }
    if (digitalRead(STEP_BUTTON) == LOW && (fstep == 5000)) {
   fstep = 1000;
    steps = "1kHz";
    delay (300);
  }
  if (digitalRead(STEP_BUTTON) == LOW && (fstep == 1000)) {
   fstep = 100;
    steps = "100Hz";
    delay (300);
  }
  if (digitalRead(STEP_BUTTON) == LOW && (fstep == 100)) {
   fstep = 10;
    steps = "10Hz";
    delay (300);
  }
  else if (digitalRead(STEP_BUTTON) == LOW && (fstep == 10)) {
   fstep = 1;
    steps = "1Hz";
    delay (300);
  }
}
/* add two button for quick change STEP or MODE *********hs0nnu**/
