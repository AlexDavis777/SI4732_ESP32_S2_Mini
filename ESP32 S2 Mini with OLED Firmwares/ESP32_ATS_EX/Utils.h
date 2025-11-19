#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

const DCfont* LastFont = DEFAULT_FONT;

void oledSetFont(const DCfont* font)
{
    if (font && LastFont != font)
    {
        LastFont = font;
        oled.setFont(font);
    }
}

void oledPrint(const char* text, int offX = -1, int offY = -1, const DCfont* font = LastFont, bool invert = false)
{
    oledSetFont(font);
    if (invert)
        oled.invertOutput(invert);
    if (offX >= 0 && offY >= 0)
        oled.setCursor(offX, offY);
    oled.print(text);
    if (invert)
        oled.invertOutput(false);
}

void oledPrint(uint16_t u, int offX = -1, int offY = -1, const DCfont* font = LastFont, bool invert = false)
{
    oledSetFont(font);
    if (invert)
        oled.invertOutput(invert);
    if (offX >= 0 && offY >= 0)
        oled.setCursor(offX, offY);
    oled.print(u);
    if (invert)
        oled.invertOutput(false);
}

//Faster alternative for convertToChar
void utoa(char* out, uint16_t num)
{
    char* p = out;
    if (num == 0)
        *p++ = '0';
    else
    {
        for (uint16_t base = 10000; base > 0; base /= 10)
        {
            if (num >= base)
            {
                *p++ = '0' + num / base;
                num %= base;
            }
            else if (p != out)
                *p++ = '0';
        }
    }

    *p = '\0';
}

//Better than sprintf which has overwhelmingly large overhead, it helps to reduce binary size
void convertToChar(char* strValue, uint16_t value, uint8_t len, uint8_t dot = 0, uint8_t separator = 0, uint8_t space = ' ')
{
    char d;
    int8_t i;
    for (i = (len - 1); i >= 0; i--)
    {
        d = value % 10;
        value = value / 10;
        strValue[i] = d + 48;
    }
    strValue[len] = '\0';

    if (dot > 0)
    {
        for (int i = len; i >= dot; i--)
        {
            strValue[i + 1] = strValue[i];
        }
        strValue[dot] = separator;
        len = dot;
    }
    i = 0;
    len--;

    while ((i < len) && ('0' == strValue[i]))
    {
        strValue[i++] = space;
    }
}

//Measure integer digit length
int ilen(uint16_t n)
{
    if (n < 10)
        return 1;
    else if (n < 100)
        return 2;
    else if (n < 1000)
        return 3;
    else if (n < 10000)
        return 4;
    else
        return 5;
}

//Split KHz frequency + BFO to KHz and .00 tail
void splitFreq(uint16_t& khz, uint16_t& tail)
{
    int32_t freq = (uint32_t(g_currentFrequency) * 1000) + g_currentBFO;
    khz = freq / 1000;
    tail = abs(freq % 1000) / 10;
}

uint8_t strlen8(const char* str)
{
    uint8_t n = 0;
    while (str[n] != '\0')
        n++;
    return n;
}

void bleep(int freq, int time) //Generate a freq tone for time duration
{
  tone(speakerPin, freq);  // Generate a tone in Hz on pin 1
  delay(time);             // Wait for number milliseconds
  noTone(speakerPin);      // Stop the tone
}

// === handleEncoder ===
void handleEncoder() {
  noInterrupts();
  dir = g_encoderCount;
  g_encoderCount = 0;
  interrupts();
  if (dir == 0) return;
  direction = (dir > 0) ? 1 : -1;
}

// === Main Function ===
void quickScan(int curr_freq, int step_khz) {
  const int barHeightMax = 50;       // Max bar height (pixels)
  const int numBars = SCREEN_WIDTH;  // Up to 128 bars (1 pixel per bar)
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  oled.clear();
  bool isFM = false;
  
  // --- Define band limits ---
  const int AM_MIN = 152;            // kHz
  const int AM_MAX = 30000;           // kHz
  const int FM_MIN = 6400;          // 64 MHz
  const int FM_MAX = 10800;         // 108 MHz

  // Detect band from current frequency
  if (g_currentMode == FM){
  isFM = 1;
  }
  else
  isFM = 0;
  int step = step_khz;
  const char* bandName = isFM ? "FM" : "AM";

  int cursorX = numBars / 2;         
  bool selected = false;
  unsigned long lastPress = 0;

  uint8_t barHeights[numBars];

  // === Step 1: Determine scan range and limits ===
  long scanRange = (numBars / 2) * step;
  long startFreq = curr_freq - scanRange;
  long endFreq   = curr_freq + scanRange;

  long bandMin = isFM ? FM_MIN : AM_MIN;
  long bandMax = isFM ? FM_MAX : AM_MAX;

  // --- Handle lower/upper band limits ---
  if (startFreq < bandMin) {
    startFreq = bandMin;
    endFreq = startFreq + numBars * step;
    if (endFreq > bandMax) endFreq = bandMax;
  } else if (endFreq > bandMax) {
    endFreq = bandMax;
    startFreq = endFreq - numBars * step;
    if (startFreq < bandMin) startFreq = bandMin;
  }

  // --- Adjust cursor based on clipped range ---
  if (curr_freq < startFreq) curr_freq = startFreq;
  if (curr_freq > endFreq)   curr_freq = endFreq;
  cursorX = (curr_freq - startFreq) / step;
  if (cursorX < 0) cursorX = 0;
  if (cursorX >= numBars) cursorX = numBars - 1;
  
      display.setCursor(90, 0);
  if (isFM)
      display.print("Mhz");
  else
      display.print("khz");
      
      display.setCursor(16, 0);
  if (isFM)
      display.print("FM");
  else
      display.print("AM");

  // === Step 2: Scan signal strengths ===
  for (int i = 0; i < numBars; i++) {
    long freq = startFreq + (i * step);
    if (freq > bandMax) break;
    g_si4735.setFrequency(freq);
    g_si4735.setSeekAmRssiThreshold(10); // default is 25
    g_si4735.setSeekAmSNRThreshold(3); // default is 5
    g_si4735.getCurrentReceivedSignalQuality();
    int rssi = g_si4735.getCurrentRSSI();
    int barHeight = map(rssi, 0, 50, 1, barHeightMax);
    if (barHeight > barHeightMax) barHeight = barHeightMax;
    barHeights[i] = barHeight;
    int barTop = SCREEN_HEIGHT - barHeights[i];
    display.drawFastVLine(i, barTop, barHeights[i], WHITE);
    display.display();    
  }
  g_si4735.setFrequency(curr_freq);

  // === Step 3: Display Loop ===
  while (!selected) {
    // Draw cursor
    display.setTextColor(WHITE);
    display.drawCircle(cursorX, 12, 2, WHITE); // small marker at top

    // Compute and display frequency
    long selectedFreq = startFreq + (cursorX * step);

    // Format frequency for FM (MHz) or AM (kHz)
    String freqText;
    if (isFM)
      freqText = String(selectedFreq / 100.0, 2);
    else
      freqText = String(selectedFreq);
    display.setCursor(46, 0);
    display.print(freqText);
    display.display();
    
              // === Handle Button Press ==
    if (digitalRead(VOLUME_BUTTON) == LOW) {
    g_si4735.volumeUp();
    delay(10);
    }
    
    if (digitalRead(AVC_BUTTON) == LOW) {
    g_si4735.volumeDown();
    delay(10);
    }
    
          // === Handle Button Press ==
    if (digitalRead(BANDWIDTH_BUTTON) == LOW) {
    display.drawCircle(cursorX, 12, 2, BLACK);// erase cursor
    cursorX --;
    cursorX = constrain(cursorX, 0, numBars - 1);
    display.fillRect(46, 0, 38, 12, BLACK);
    g_si4735.setFrequency(selectedFreq);
    delay(10);
    display.display();   
    }
    
    if (digitalRead(STEP_BUTTON) == LOW) {
    display.drawCircle(cursorX, 12, 2, BLACK);// erase cursor
    cursorX ++;
    cursorX = constrain(cursorX, 0, numBars - 1);
    display.fillRect(46, 0, 38, 12, BLACK);
    g_si4735.setFrequency(selectedFreq);
    delay(10);
    display.display();   
    }
    // Check if the encoder has moved
    if (g_encoderCount != 0)
    handleEncoder(); 
    {
       delay(60);
       if (direction == 1)
       {
            display.drawCircle(cursorX, 12, 2, BLACK);// erase cursor
            cursorX++;
            cursorX = constrain(cursorX, 0, numBars - 1);
            display.fillRect(46, 0, 38, 12, BLACK);
            g_si4735.setFrequency(selectedFreq);
            Serial.println(direction);
            direction = 0;
            display.display();
       }
       delay(60);
       if (direction == -1)
       {
            display.drawCircle(cursorX, 12, 2, BLACK);// erase cursor
            cursorX--;
            cursorX = constrain(cursorX, 0, numBars - 1);
            display.fillRect(46, 0, 38, 12, BLACK);
            g_si4735.setFrequency(selectedFreq);
            Serial.println(direction);
            direction = 0;
            display.display();
       }
    }
    
    // === Handle Button Press ===
    if (digitalRead(ENCODER_BUTTON) == LOW) {
      selected = true;
      lastPress = millis();
      long chosenFreq = startFreq + (cursorX * step);
      oled.clear();
      display.setTextSize(2);
      Serial.print("chosenFreq: ");
      Serial.println(chosenFreq);
      oledPrint("Chosen Frequency", 0, 2);
      oledPrint(chosenFreq, 24, 4);
      curr_freq = chosenFreq;
      g_currentFrequency = curr_freq;
      g_si4735.setFrequency(g_currentFrequency);
      delay(2000);
      oled.clear();
      display.setTextSize(1);
      return;
    }
   }
 }
