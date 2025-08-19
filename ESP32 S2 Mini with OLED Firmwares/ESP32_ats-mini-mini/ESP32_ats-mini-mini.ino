// ===================================================================================
// SECTION 1: LIBRARY INCLUDES AND GLOBAL DEFINITIONS
// ===================================================================================

// Include necessary libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SI4735-fixed.h>
#include <Rotary.h>
#include "patch_init.h"
#include "DSEG7_Classic_Mini_Bold_16.h"
#define RESET_PIN        35
#define PIN_AMP_EN       10
// I2C bus pin on ESP32
#define ESP32_I2C_SDA 39
#define ESP32_I2C_SCL 40
#define ENCODER_PIN_A    8
#define ENCODER_PIN_B    6
#define ENCODER_BTN      10
#define VOLUME_UP        34
#define VOLUME_DOWN      21
#define STEP_BUTTON      14
#define BAND_UP          17
#define BAND_DOWN        15
#define MODE_BUTTON       38
#define AGC_SCAN         13
#define BFO_STEP_HZ      25
#define OLED_RESET    -1  // or use the actual reset pin if connected
#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH  128
// ===================================================================================
// SECTION 2: DATA STRUCTURES AND GLOBAL VARIABLES
// ===================================================================================
// Converts signal strength (RSSI) to bar height (max 63)
int signalStrengthToHeight(int strength, int maxStrength = 100) {
  int height = map(strength, 0, maxStrength, 0, SCREEN_HEIGHT - 1);
  return constrain(height, 0, SCREEN_HEIGHT - 1);
}
// Array with text names for modulations to display on the screen
const char* mod_names[] = {"FM ", "LSB", "USB", "AM "};
enum Modulation {FM, LSB, USB, AM};

struct Band {
  const char* name;
  uint16_t min_freq, max_freq, default_freq;
  Modulation default_mod;
  bool ssb_allowed;
};

const Band bands[] = {
  {"LW", 150, 520, 198, AM, false},
  {"MW", 520, 1710, 1000, AM, false},
  {"SW 1.7-4", 1710, 4000, 3570, LSB, true},
  {"SW 4-8", 4000, 8000, 6500, AM, true},
  {"40m HAM", 6000, 7500, 7000, USB, true},
  {"AM 8-15", 8000, 15000, 10000, AM, false},
  {"AM 15-30", 15000, 30000, 21200, AM, false},
  {"FM", 6400, 10800, 9940, FM, false}
};
const int NUM_BANDS = sizeof(bands) / sizeof(bands[0]);

struct State {
  int band = 0;
  uint16_t freq = bands[0].default_freq;
  Modulation mod = bands[0].default_mod;
  int bfo = 0;
  int step = 1, stepFM = 10;
  int vol = 20;
};
State radio, menu;

enum MenuItem { M_MOD, M_EXIT};
const int MENU_ITEMS = 2;

SI4735_fixed rx;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);
Rotary encoder(ENCODER_PIN_B, ENCODER_PIN_A);
volatile int encoderChange = 0;
bool inMenu = false;
int selectedMenuItem = 0;
bool needsRedraw = true;

void ICACHE_RAM_ATTR rotaryEncoder();
void setRadio();
void showMenu(), showMain();
void handleEncoder(), handleButton();
void changeBand(int), changeVol(int), changeMod(int), changeStep(int), changeFreq(int);

// ===================================================================================
// SECTION 3: setup() FUNCTION - RUNS ONCE ON STARTUP
// ===================================================================================

void setup() {
  Serial.begin(115200);
  pinMode(ENCODER_PIN_A, INPUT_PULLUP); 
  pinMode(ENCODER_PIN_B, INPUT_PULLUP); 
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(VOLUME_UP, INPUT_PULLUP);
  pinMode(VOLUME_DOWN, INPUT_PULLUP);
  pinMode(STEP_BUTTON, INPUT_PULLUP);
  pinMode(BAND_UP, INPUT_PULLUP);
  pinMode(BAND_DOWN, INPUT_PULLUP);
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(AGC_SCAN, INPUT_PULLUP);
  //pinMode(PIN_AMP_EN, OUTPUT);
  //digitalWrite(PIN_AMP_EN, LOW);

  Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(2, 22);
  display.print("Searching for SI4732...");
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN);
  display.setCursor(2, 34);
  display.print("Si4732 addr:0x");
  display.print(si4735Addr, HEX);
  display.display();

  if (!rx.getDeviceI2CAddress(RESET_PIN)) {
    display.setCursor(2, 34);
    display.print("SI4732 not found!");
    display.display();
    while(true);
  }
  rx.setup(RESET_PIN, 0);
  delay(500);
  setRadio();
  //digitalWrite(PIN_AMP_EN, HIGH);

  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);
}

// ===================================================================================
// SECTION 4: loop() FUNCTION - RUNS CONTINUOUSLY
// ===================================================================================

void loop() {
  if (encoderChange != 0) {
    handleEncoder(); 
    needsRedraw = true;
  }
  handleButton();
  if (needsRedraw) {
    if (inMenu) showMenu(); else {showMain();showSig();}
    needsRedraw = false;
  }
  
  vupCheck();     //check V+ button
  vdownCheck();   //check V- button
  stepCheck();    //check Step button
  bandUPCheck();  //check Band+ button
  bandDOWNCheck(); //check Band- button
  modeCheck();      //check Mode button
  AGCScanCheck(); //check Scan button
  delay(10);
  
}
  void vupCheck() {
  if (digitalRead(VOLUME_UP) == LOW) {
        rx.volumeUp();
        radio.vol = rx.getVolume();
        needsRedraw = true;
        delay(40);
  }
}
  void vdownCheck() {
  if (digitalRead(VOLUME_DOWN) == LOW) {
        rx.volumeDown();
        radio.vol = rx.getVolume();
        needsRedraw = true;
        delay(40);
  }
}

  void stepCheck() {
  if (digitalRead(STEP_BUTTON) == LOW) {
        changeStep(1);
        needsRedraw = true;
        delay(100);
  }
}

 void bandUPCheck() {
  if (digitalRead(BAND_UP) == LOW) {
        changeBand(1);
        needsRedraw = true;
        delay(100);
  }
}

void bandDOWNCheck() {
  if (digitalRead(BAND_DOWN) == LOW) {
        changeBand(-1);
        needsRedraw = true;
        delay(100);
  }
}

void modeCheck() {
  if (digitalRead(MODE_BUTTON) == LOW) {
        changeMod2(1);
        setRadio();
        needsRedraw = true;
        delay(100);
  }
}

void AGCScanCheck() {
  if (digitalRead(AGC_SCAN) == LOW) {
        quickScan(radio.freq, radio.step);
        setRadio();
        needsRedraw = true;
        delay(100);
  }
}

// ===================================================================================
// SECTION 5: HANDLER FUNCTIONS
// ===================================================================================

// Interrupt Service Routine (ISR). Must be as fast as possible.
// The `ICACHE_RAM_ATTR` attribute places it in the fast RAM of the ESP32.
// Handle encoder direction
void ICACHE_RAM_ATTR rotaryEncoder()
{
  // Encoder interrupt routine for both pins. Updates counter
  // if they are valid and have rotated a full indent
  
  static uint8_t old_AB = 3;  // Lookup table index
  static int8_t encval = 0;   // Encoder value  
  static const int8_t enc_states[]  = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}; // Lookup table
 
  old_AB <<=2;  // Remember previous state
 
  if (digitalRead(ENCODER_PIN_A)) old_AB |= 0x01; // Add current state of pin A
  if (digitalRead(ENCODER_PIN_B)) old_AB |= 0x02; // Add current state of pin B
  
   
  encval += enc_states[( old_AB & 0x0f )];
 
  // Update counter if encoder has rotated a full indent, that is at least 4 steps
  if( encval > 3 ) {        // Four steps forward
    encoderChange++;              // Increase counter
    encval = 0;
  }
  else if( encval < -3 ) {  // Four steps backwards
    encoderChange--;               // Decrease counter
    encval = 0;
  }
}

void handleEncoder() {
  noInterrupts();
  int dir = encoderChange;
  encoderChange = 0;
  interrupts();
  
  if (dir == 0) return;

  if (inMenu) {
    int direction = (dir > 0) ? 1 : -1;
    switch (selectedMenuItem) {
      case M_MOD:  changeMod(direction); break;
      case M_EXIT:
        inMenu = false; radio = menu; radio.freq = bands[radio.band].default_freq; setRadio();
        break;
    }
  } else {
    changeFreq(dir);
  }
}

void handleButton() {
  static unsigned long last = 0;
  if (digitalRead(ENCODER_BTN) == LOW && millis() - last > 250) {
    if (!inMenu) {
      inMenu = true; menu = radio; selectedMenuItem = 0;
    } else {
      selectedMenuItem = (selectedMenuItem + 1) % MENU_ITEMS;
    }
    needsRedraw = true;
    last = millis();
  }
}

// ===================================================================================
// SECTION 6: STATE-CHANGING FUNCTIONS
// ===================================================================================

// Key function for frequency tuning. Contains special logic for SSB.
void changeFreq(int dir) {
  auto &r = radio;
  const Band* b = &bands[r.band];
  
  if (r.mod == LSB || r.mod == USB) {
    uint16_t old_freq = r.freq;
    r.bfo += dir * BFO_STEP_HZ;
    while (r.bfo >= 500) {
      r.freq++;
      r.bfo -= 1000;
    }
    while (r.bfo <= -500) {
      r.freq--;
      r.bfo += 1000;
    }
    if (r.freq != old_freq) {
      if (r.freq > b->max_freq) r.freq = b->min_freq;
      if (r.freq < b->min_freq) r.freq = b->max_freq;
      rx.setFrequency(r.freq);
    }
    rx.setSSBBfo(-r.bfo);
  } else {
    int step = (r.mod == FM) ? r.stepFM : r.step;
    r.freq += dir * step;
    if (r.freq > b->max_freq) r.freq = b->min_freq;
    if (r.freq < b->min_freq) r.freq = b->max_freq;
    rx.setFrequency(r.freq);
  }
}

void changeBand(int d) {
  radio.band = (radio.band + d + NUM_BANDS) % NUM_BANDS;
  radio.mod = bands[radio.band].default_mod;
  radio.freq = bands[radio.band].default_freq;
  setRadio();
}

void changeVol(int d) {
    menu.vol = constrain(menu.vol + d, 0, 63);
    rx.setVolume(menu.vol);
}

void changeMod(int d) {
  auto b = bands[menu.band];
  if (!b.ssb_allowed) return;
  int m = (int)menu.mod;
  if (d > 0) menu.mod = (m >= AM) ? LSB : (Modulation)(m + 1);
  else       menu.mod = (m <= LSB) ? AM : (Modulation)(m - 1);
}

void changeMod2(int d) {
  auto b = bands[radio.band];
  if (!b.ssb_allowed) return;
  int m = (int)radio.mod;
  if (d > 0) radio.mod = (m >= AM) ? LSB : (Modulation)(m + 1);
  else       radio.mod = (m <= LSB) ? AM : (Modulation)(m - 1);
}

void changeStep(int d) {
  if (bands[radio.band].default_mod == FM) {
    const int steps[] = {1, 5, 10, 20}; 
    int idx = 0; 
    for(int i = 0; i < 4; i++) if(radio.stepFM == steps[i]) idx = i;
    idx = (idx + d + 4) % 4; 
    radio.stepFM = steps[idx];
  } else {
    const int steps[] = {1, 5, 9, 10, 50}; 
    int idx = 0; 
    for(int i = 0; i < 5; i++) if(radio.step == steps[i]) idx = i;
    idx = (idx + d + 5) % 5; 
    radio.step = steps[idx];
  }
}

// ===================================================================================
// SECTION 7: DISPLAY FUNCTIONS
// ===================================================================================

// Renders the main screen
void showMain() {

  char freq[16], unit[5];
  if (radio.mod == FM) { sprintf(freq, "%.2f", radio.freq / 100.0); strcpy(unit, "MHz"); }
  else {
    float df = radio.freq + (radio.bfo / 1000.0);
    (radio.bfo == 0) ? sprintf(freq, "%u", radio.freq) : sprintf(freq, "%.2f", df);
    strcpy(unit, "kHz");
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.drawFastHLine(1, 1, 127, WHITE);
  display.drawFastHLine(1, 12, 127, WHITE);
  display.drawFastVLine(1, 1, 12, WHITE);
  display.drawFastVLine(127, 1, 12, WHITE);
  display.setFont(&DSEG7_Classic_Mini_Bold_16);
  display.setCursor(12, 35);
  display.print(freq);
  display.setFont(NULL);
  display.setCursor(110, 28);
  display.print(unit);
  display.setCursor(3, 4);
  display.print("Band:");
  display.print(bands[radio.band].name);
  display.drawFastHLine(1, 52, 127, WHITE);
  display.drawFastHLine(1, 63, 127, WHITE);
  display.drawFastVLine(1, 52, 12, WHITE);
  display.drawFastVLine(127, 52, 12, WHITE);
  display.setCursor(3, 55);
  display.print("Mode:");
  display.print(mod_names[radio.mod]);
  int stepNow = (radio.mod == FM) ? radio.stepFM * 10 : radio.step;
  display.setCursor(60, 55);
  display.print("Step:");
  display.print(String(stepNow) + "kHz");
  display.setCursor(89, 4);
  display.print("Vol:");
  display.print(radio.vol);
  display.display();
}

void showMenu() {
  display.clearDisplay();
  display.drawFastHLine(7, 0, 86, WHITE);
  display.drawFastHLine(7, 63, 86, WHITE);
  display.drawFastVLine(7, 0, 63, WHITE);
  display.drawFastVLine(92, 0, 63, WHITE);
  String items[MENU_ITEMS];
  items[M_MOD]  = "Mode:"   + String(mod_names[menu.mod]);
  int stepNow = (bands[menu.band].default_mod == FM) ? menu.stepFM * 10 : menu.step;
  items[M_EXIT] = "Set & Exit";
  for(int i=0;i<MENU_ITEMS;i++) {
    display.setTextColor(selectedMenuItem == i ? BLACK : WHITE, selectedMenuItem == i ? WHITE : BLACK);
    display.setCursor(10, (i*10)+2);
    display.print(items[i]);
  }
  display.display();
}

void showSig() {
int rssiAux = 0;
  rx.getCurrentReceivedSignalQuality();
  int rssi = rx.getCurrentRSSI();
  int val =  map(rssi, 0, 60, 0, 99);
  val = constrain(val, 0, 127);
  display.drawRect(1, 43, 127, 4, 1); //Border of the bar chart
  display.fillRect(2, 44, val, 2, WHITE); //Draws the bar depending on the sensor value
  display.display();  display.drawRect(1, 43, 127, 4, 1); //Border of the bar chart
}

// Function to scan around curr_freq and display signal strength bars
void quickScan(int curr_freq, int step_khz) {
  const int center_x = SCREEN_WIDTH / 2 - 1;  // 63
  int max_strength = 0;
  int strong_freq = curr_freq;

  display.clearDisplay();

  // Measure center frequency signal strength
  rx.setFrequency(curr_freq);
  delay(10);
  rx.setSeekAmRssiThreshold(10); // default is 25
  rx.setSeekAmSNRThreshold(3); // default is 5
  rx.getCurrentReceivedSignalQuality();
  int center_strength = rx.getCurrentRSSI();
  max_strength = center_strength;
  

  // Plot center frequency
  int h = signalStrengthToHeight(center_strength);
  display.setCursor(56, 0);
  display.print(curr_freq);
  display.drawLine(center_x, SCREEN_HEIGHT - 1, center_x, SCREEN_HEIGHT - 1 - h, WHITE);
  display.display();
  display.invertDisplay(false);
  Serial.printf(" Center Strength=");
  Serial.print(h);

  // LEFT side (lower freqs)
  int x = center_x - 1;
  for (int f = curr_freq - step_khz; f >= bands[radio.band].min_freq && x >= 0; f -= step_khz, x--) {
    rx.setFrequency(f);
    delay(10);
    rx.setSeekAmRssiThreshold(10); // default is 25
    rx.setSeekAmSNRThreshold(3); // default is 5
    rx.getCurrentReceivedSignalQuality();
    int strength = rx.getCurrentRSSI();
    h = signalStrengthToHeight(strength);
    display.drawLine(x, SCREEN_HEIGHT - 1, x, SCREEN_HEIGHT - 1 - h, WHITE);
    display.display();
    if (strength > max_strength) {
      max_strength = strength;
      strong_freq = f;
    }
  }
  Serial.printf(" Freq Left=");
  Serial.print(strong_freq);
  Serial.printf(" Left Strength=");
  Serial.print(h);

  // RIGHT side (higher freqs)
  x = center_x + 1;
  for (int f = curr_freq + step_khz; f <= bands[radio.band].max_freq && x < SCREEN_WIDTH; f += step_khz, x++) {
    rx.setFrequency(f);
    delay(10);
    rx.setSeekAmRssiThreshold(10); // default is 25
    rx.setSeekAmSNRThreshold(3); // default is 5
    rx.getCurrentReceivedSignalQuality();
    int strength = rx.getCurrentRSSI();
    h = signalStrengthToHeight(strength);
    display.drawLine(x, SCREEN_HEIGHT - 1, x, SCREEN_HEIGHT - 1 - h, WHITE);
    display.display();
    if (strength > max_strength) {
      max_strength = strength;
      strong_freq = f;
    }
  }
  Serial.printf(" Freq Right=");
  Serial.print(strong_freq);
  Serial.printf(" Right Strength=");
  Serial.print(h);
  
    // --- Draw marker for strongest signal ---
  if (strong_freq != curr_freq) {
    int diff = (strong_freq - curr_freq) / step_khz;
    int marker_x = center_x + diff;

    if (marker_x >= 0 && marker_x < SCREEN_WIDTH) {
      display.setTextColor(WHITE,BLACK); // Set text to plot foreground and background colours
      display.setCursor(40, 0);
      display.print("        ");
      display.setTextColor(WHITE);
      display.drawCircle(marker_x, 2, 2, WHITE); // small marker at top
    }
  }
  display.display();
  delay(3000);
  display.clearDisplay();
  display.setCursor(0, 30);
  display.printf("Freq Centre= ");
  display.print(curr_freq);
  display.print("<-->");
  display.printf(" New Freq= ");
  display.print(strong_freq);
  display.display();
  delay(3000);
  // Return frequency if it's stronger than current
   if (center_strength < max_strength)
   {
    curr_freq = strong_freq;
   }
   radio.freq = curr_freq;
   //radio.freq = strong_freq;
}

// ===================================================================================
// SECTION 8: SERVICE FUNCTION
// ===================================================================================

// Applies settings to the radio chip
void setRadio() {
  //digitalWrite(PIN_AMP_EN, LOW); delay(20);
  radio.bfo = 0; const Band* b = &bands[radio.band];
  if(radio.mod == FM) {
  rx.setFM(b->min_freq, b->max_freq, radio.freq, radio.stepFM);
  rx.setFmBandwidth(2); //new 84khz
  }
  else if(radio.mod == AM) {
  rx.setAM(b->min_freq, b->max_freq, radio.freq, radio.step);
  rx.setBandwidth(1, 1); //new 4khz
  }
  else {
    rx.setBandwidth(5, 1); //new 1.8 kHz
    rx.loadPatch(ssb_patch_content, sizeof(ssb_patch_content));
    uint8_t ssbm = (radio.mod == LSB) ? 1 : 2;
    rx.setSSB(b->min_freq, b->max_freq, radio.freq, radio.step, ssbm);
    rx.setSSBBfo(0);
  }
  rx.setVolume(radio.vol); delay(50);
  //digitalWrite(PIN_AMP_EN, HIGH);
}
